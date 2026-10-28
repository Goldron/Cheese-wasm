////////////////////////////////////////////////////////////////////////////////
//
// Cheese, Chess engine.
//
// Copyright (C) 2006-2025 Patrice Duhamel. All rights reserved.
//
// Cheese is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Cheese is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include "tuner.h"

#include "engine.h"
#include "bitboard.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace Cheese {

const std::string result_filename = "result.csv";

////////////////////////////////////////////////////////////////////////////////
// evaluate probability of winning based on evaluation score
inline double sigmoid(double k, double s)
{
	return 1.0 / (1.0 + std::exp((-k * s) / 400.0));
}

////////////////////////////////////////////////////////////////////////////////
double ChessTuner::linearEvaluation(const Position &pos)
{
	auto m = static_cast<double>(max_game_phase);

	Parameter v;
	for (auto i=pos.index_start; i<pos.index_end; i++) {
		
		auto index = coefs[i].index;
		auto coef = static_cast<double>(coefs[i].coef);
		
		v.mid += coef * parameters[index].mid;
		v.end += coef * parameters[index].end;
	}

	// add position eval because parameters start from zero
	auto p = static_cast<double>(pos.phase);

	auto ev = pos.eval + (v.mid * p + v.end * (m - p)) / m;

	return ev + static_cast<double>((pos.side == white) ? eval_tempo : 
		-eval_tempo);
}

////////////////////////////////////////////////////////////////////////////////
double ChessTuner::singleLinearError(const Position &pos, double k)
{
	double s = sigmoid(k, linearEvaluation(pos));
	double sp = s * (1.0 - s);
	return (pos.result - s) * sp;
}

////////////////////////////////////////////////////////////////////////////////
double ChessTuner::completeLinearError(double k)
{
	double e = 0.0;
		
	threads.clear();
	for (auto n=0U; n<maxThreads; n++) {
		
		auto cnt = positions.size() / maxThreads;
		auto pos1 = n * cnt;
		auto pos2 = (n == maxThreads - 1) ? positions.size() : (n + 1) * cnt;
			
		threads.emplace_back([this](std::size_t id, double k2, 
			std::size_t p1, std::size_t p2) {
			
			ChessTunerThreadData &data = threadsdata[id];
			data.error = 0.0;
			for (auto p=p1; p<p2; p++) {
				double s = sigmoid(k2, linearEvaluation(positions[p]));
				data.error += pow(positions[p].result - s, 2.0);
			}
			
		}, n, k, pos1, pos2);
		
	}
	
	for (auto n=0U; n<maxThreads; n++) {
		threads[n].join();
		e += threadsdata[n].error;
	}

	return e / static_cast<double>(positions.size());
}

////////////////////////////////////////////////////////////////////////////////
double ChessTuner::calculateK()
{
	constexpr int precision = 10;
	
	double k1 = -10.0;
	double k2 = 10.0;
	double delta = 1.0;
	
	double bestk = k1;
	double beste = completeEvaluationError(k1);
	
	for (int i=0; i<precision; i++) {		
		double k = k1;
		while (k < k2) {			
			double e = completeEvaluationError(k);
			if (e < beste) {
				beste = e;
				bestk = k;
			}
			k += delta;
		}
		
		k1 = bestk - delta;
		k2 = bestk + delta;
		delta = delta / 10.0;
	}
	
	return bestk;
}

////////////////////////////////////////////////////////////////////////////////
double ChessTuner::completeEvaluationError(double k) 
{	
	threads.clear();
	for (auto n=0U; n<maxThreads; n++) {
		
		auto cnt = positions.size() / maxThreads;
		auto pos1 = n * cnt;
		auto pos2 = (n == maxThreads - 1) ? positions.size() : (n + 1) * cnt;
			
		threads.emplace_back([this](std::size_t id, double k2,
			std::size_t p1, std::size_t p2) {
			
			ChessTunerThreadData &data = threadsdata[id];
			data.error = 0.0;
			for (auto p=p1; p<p2; p++) {
				double s = sigmoid(k2, positions[p].eval);
				data.error += pow(positions[p].result - s, 2.0);
			}	
		}, n, k, pos1, pos2);
			
	}

	double e = 0.0;	
	for (auto n=0U; n<maxThreads; n++) {
		threads[n].join();
		e += threadsdata[n].error;
	}

	return e / static_cast<double>(positions.size());
}

////////////////////////////////////////////////////////////////////////////////	
void ChessTuner::updateGradients(std::size_t offset, std::size_t count, 
	double k)
{
	for (auto i=0U; i<parameters.size(); i++) {
		gradients[i].mid = 0.0;
		gradients[i].end = 0.0;
	}
		
	threads.clear();
	for (auto n=0U; n<maxThreads; n++) {
		
		auto cnt = count / maxThreads;
		auto pos1 = offset + n * cnt;
		auto pos2 = (n != maxThreads - 1) ? (offset + (n + 1) * cnt) : 
			(offset + count);
			
		threads.emplace_back([this](std::size_t id, double k2, 
			std::size_t p1, std::size_t p2) {
			
			ChessTunerThreadData &data = threadsdata[id];
			
			for (auto i=0U; i<parameters.size(); i++) {
				data.local[i].mid = 0.0;
				data.local[i].end = 0.0;
			}
			
			for (auto p=p1; p<p2; p++) {
		
				const auto &pos = positions[p];
					
				double e = singleLinearError(pos, k2);
				
				double e_mid = e * pos.fmid;
				double e_end = e * pos.fend;
				
				// calculate changes only for parameters used in evaluation
				for (auto i=pos.index_start; i<pos.index_end; i++) {
					
					auto index = coefs[i].index;
					auto coef = static_cast<double>(coefs[i].coef);
					
					data.local[index].mid += e_mid * coef;
					data.local[index].end += e_end * coef;
				}
			}
			
		}, n, k, pos1, pos2);			
	}
	
	for (auto n=0U; n<maxThreads; n++) {
		threads[n].join();
		
		// update gradients
		for (auto i=0U; i<parameters.size(); i++) {		
			gradients[i].mid += threadsdata[n].local[i].mid;
			gradients[i].end += threadsdata[n].local[i].end;
		}
	}	
}

////////////////////////////////////////////////////////////////////////////////
void ChessTuner::updateParameters(std::size_t nbpositions, double rate)
{
	for (auto i=0U; i<parameters.size(); i++) {
		parameters[i].mid += (2.0 / static_cast<double>(nbpositions)) * 
			rate * gradients[i].mid;
		parameters[i].end += (2.0 / static_cast<double>(nbpositions)) * 
			rate * gradients[i].end;
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessTuner::loadConfigFile(const std::string &filename)
{
	ConfigFile conf;
	if (!conf.load(filename)) {
		Logger::warning() << "missing configuration file : " << filename;
	} else {	
		filePositions = conf.get("FileEPD");
		maxThreads = conf.getInt("MaxThreads", 1);
		batch_size = conf.getInt("BatchSize", 16384);
		display_interval = conf.getInt("DisplayInterval", 1);
		stop_iteration = conf.getInt("StopIteration", 10000);
		learning_rate = conf.getDouble("LearningRate", 20.0);
		reduce_learning_rate = conf.getDouble("ReduceLearningRate", 1.25);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessTuner::initParameters(const ChessEval &ev)
{
	auto evp = ev.getTuningParameters();
	
	for (const auto &p : evp) {

		parameters_default.emplace_back(static_cast<double>(p.value.mid),
			static_cast<double>(p.value.end));

		parameters.emplace_back(0.0, 0.0);

		parametersNames.push_back(p.name);
	}	
	
	Logger::info() << "number of parameters = " << parameters.size();
}

////////////////////////////////////////////////////////////////////////////////
double ChessTuner::readGameResult(const std::string &fen) const
{
	if (fen.find("0-1") != std::string::npos) {
		return 0.0;
	} else 
	if (fen.find("1-0") != std::string::npos) {
		return 1.0;
	} else 
	if (fen.find("1/2-1/2") != std::string::npos) {
		return 0.5;
	}
	return -1.0;
}

////////////////////////////////////////////////////////////////////////////////		
bool ChessTuner::loadPositions(const std::string &filename, 
	std::vector<std::string> &fens)
{
	return ChessFENParser::load(filename, fens);
}

////////////////////////////////////////////////////////////////////////////////
bool ChessTuner::init()
{
	// init threads
	threads.reserve(maxThreads);
	threadsdata.resize(maxThreads);
	
	// load positions
	if (!loadPositions(filePositions, positions_fen)) {
		Logger::error() << "file not found : " << filePositions;
		return false;
	}
	
	Logger::info() << "loaded " << positions_fen.size() << " positions";
	std::cout << "loaded " << positions_fen.size() << " positions" << std::endl;

	// precalc tables
	ChessEngine::precalcTables();
		
	pawnHash.clear();
	eval.clearEvalData();
	eval.initTuning();
	
	auto evp = eval.getTuningParameters();

	// init parameters
	initParameters(eval);
	
	gradients.resize(parameters.size());
	local.resize(parameters.size());
	
	std::cout << "number of parameters = " << parameters.size() << std::endl;

	for (auto i=0U; i<maxThreads; i++) {
		threadsdata[i].local.resize(parameters.size());
	}
	
	ChessBoard board;

	std::vector<std::size_t> pcountsw;
	std::vector<std::size_t> pcountsb;
	for (auto n=0U; n<parameters.size(); n++) {
		pcountsw.push_back(0);
		pcountsb.push_back(0);
	}

	std::size_t count_win = 0ULL;
	std::size_t count_lost = 0ULL;
	std::size_t count_draw = 0ULL;

	for (const auto &fen : positions_fen) {
			
		// set position
		if (!ChessFENParser::set(fen, board)) {
			Logger::error() << "bad fen : " << fen;
			return false;
		}

		// get game result
		auto result = readGameResult(fen);
		if (result == -1.0) {
			Logger::warning() << "unknown game result : " << fen;
			continue;
		}

		if (result == 1.0) {
			++count_win;
		}
		if (result == 0.0) {
			++count_lost;
		}
		if (result == 0.5) {
			++count_draw;
		}
		
		// precalc game phase factors
		auto gp = std::min(board.gamePhase(), max_game_phase);
		auto m = static_cast<double>(max_game_phase);
		auto p = static_cast<double>(gp);
		auto fmid = p / m;
		auto fend = (m - p) / m;
			   
		// evaluate position and terms
		auto value = static_cast<double>(eval.evaluateTuning(board, pawnHash));
		value = (board.side() == white) ? value : -value;

		// init parameters coefs		
		// only take the terms used in the evaluation 	
		auto pstart = coefs.size();
		for (auto n=0U; n<parameters.size(); n++) {
			int dc = evp[n].term[white] - evp[n].term[black];
			if  (dc != 0) {
				coefs.emplace_back(n, dc);
			}

			if (evp[n].term[white] != 0) {
				pcountsw[n]++;
			}
			if (evp[n].term[black] != 0) {
				pcountsb[n]++;
			}
		}
		auto pend = coefs.size();

		positions.emplace_back(pstart, pend, result, value, fmid, fend,
			static_cast<int>(gp), board.side());		
	}

	Logger::info() << "count win = " << count_win 
		<< " (" << (count_win * 100) / positions.size() << " %";
	Logger::info() << "count lost = " << count_lost 
		<< " (" << (count_lost * 100) / positions.size() << " %";
	Logger::info() << "count draw = " << count_draw 
		<< " (" << (count_draw * 100) / positions.size() << " %";

	for (auto n=0U; n<parameters.size(); n++) {
		Logger::info() << parametersNames[n] << " : " 
			<< (pcountsb[n] * 100) / positions.size()
			<< " % (" << pcountsb[n] << ") , " 
			<< (pcountsw[n] * 100) / positions.size()
			<< " % (" << pcountsw[n] << ")";
	}

	Logger::info() << "number of coefs = " << coefs.size();
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
void ChessTuner::saveParameters(const std::string &filename)
{
	std::ofstream ofs;
	ofs.open(filename, std::ofstream::out);	

	auto evp = eval.getTuningParameters();
	
	for (auto i=0U; i<parameters.size(); i++) {
		auto vmid = static_cast<int>(
			std::round(parameters_default[i].mid + parameters[i].mid));
		auto vend = static_cast<int>(
			std::round(parameters_default[i].end + parameters[i].end));
	
		evp[i].value = EvalScore(vmid, vend);
	}

	ofs << "// generated from " 
		<< std::filesystem::path(filePositions).filename()
		<< '\n' << '\n';
	
	ofs << eval.printEval() << std::endl;
	ofs.close();
}

////////////////////////////////////////////////////////////////////////////////
// save parameters in CSV format
void ChessTuner::saveIterations(const std::string &filename,
	std::size_t iteration, double error)
{
	std::ofstream ofs;
	if (iteration == 1) {
		ofs.open(filename, std::ofstream::out);
		ofs << "iteration;error;";
		for (auto i=0U; i<parameters.size(); i++) {
			ofs << parametersNames[i] << " mid;"
				<< parametersNames[i] << " end;";
		}
		ofs << std::endl;
	} else {
		ofs.open(filename, std::ofstream::app);
	}
	ofs << iteration << ';'
		<< error << ';';
	for (auto i=0U; i<parameters.size(); i++) {
		auto vmid = static_cast<int>(
			std::round(parameters_default[i].mid + parameters[i].mid));
		auto vend = static_cast<int>(
			std::round(parameters_default[i].end + parameters[i].end));
		ofs << vmid << ';' << vend << ';';
	}
	ofs << std::endl;
	ofs.close();
}

////////////////////////////////////////////////////////////////////////////////
void ChessTuner::optimize()
{
	// calculate K
	double k = calculateK();
	double rate = learning_rate;
	double error = completeLinearError(k);
	double besterror = error;
	
	std::size_t iteration = 1;
	
	std::random_device rd;	
	std::seed_seq sd { rd(), rd(), rd(), rd() };
	std::mt19937_64 mtrnd(sd);

	std::cout << "k = " << k << std::endl;
	std::cout << "start" << " : error = " << error << std::endl;
//	saveIterations(result_filename, iteration, error);

	parameters_current.resize(parameters.size());
	
	while (true) {

		std::shuffle(positions.begin(), positions.end(), mtrnd);
		
		if (((iteration % display_interval) == 0) ||
			(iteration == stop_iteration))  {

			error = completeLinearError(k);
			
			if (error > besterror) {
				rate = rate / reduce_learning_rate;
				std::cout << "  reduce learning rate to " << rate << std::endl;
			}
			besterror = error;

			std::size_t changes = 0ULL;
			for (auto i=0U; i<parameters.size(); i++) {
				auto vmid = static_cast<int>(
					std::round(parameters_default[i].mid + parameters[i].mid));
				auto vend = static_cast<int>(
					std::round(parameters_default[i].end + parameters[i].end));

				if ((parameters_current[i].mid != vmid) ||
					(parameters_current[i].end != vend)) {
					++changes;
				}
				parameters_current[i].mid = vmid;
				parameters_current[i].end = vend;
			}
			
			std::cout << "iteration " << iteration
					  << " : error = " << error 
					  << " changes = " << changes
					  << std::endl;
//			saveIterations(result_filename, iteration, error);			
			saveParameters("parameters.txt");
			if (!eval.saveEval("eval.hce")) {
				Logger::error() << "unable to save eval data";
			}
		}
		
		if (iteration == stop_iteration) {
			break;
		}

		for (auto n=0U; n<positions.size() / batch_size; n++) {
	
			// calculate gradients
			updateGradients(n * batch_size, batch_size, k);
		
			// update parameters
			updateParameters(batch_size, rate);
		}	

		++iteration;
	}
}

////////////////////////////////////////////////////////////////////////////////
bool ChessTuner::run()
{
	if (!init()) {
		return false;
	}
	
	optimize();
	
	return true;
}
	
} // namespace Cheese

