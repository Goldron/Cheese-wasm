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

#ifndef CHEESE_TUNER_H_
#define CHEESE_TUNER_H_

#include "config.h"

#include "board.h"
#include "eval.h"
#include "fen.h"
#include "tune.h"
#include "history.h"
#include "hash.h"
#include "repetitions.h"
#include "util/configfile.h"

#include <cmath>
#include <random>
#include <vector>
#include <string>
#include <thread>

namespace Cheese {
	
////////////////////////////////////////////////////////////////////////////////
// tuning parameter with middle/endgame value
struct Parameter {

	double mid {0.0};

	double end {0.0};
	
	Parameter() = default;
	
	Parameter(double m, double e) : mid(m), end(e)
	{}
};

////////////////////////////////////////////////////////////////////////////////
struct EvalCoef {
	
	// parameter index
	std::size_t index {0};
	
	int coef {0};
	
	EvalCoef() = default;
	
	EvalCoef(std::size_t i, int c) : index(i), coef(c)
	{}
};

////////////////////////////////////////////////////////////////////////////////
// training position
struct Position {
	
	// start position of eval coefs in the global vector
	std::size_t index_start {0};
	
	// end position of eval coefs in the global vector
	std::size_t index_end {0};
	
	// game result from epd
	// (0.0 = black win, 0.5 = draw, 1.0 = white win)
	double result {0.0};
	
	// default evaluation from white perspective 
	double eval {0.0};
	
	// precalc phase factors (optimization)
	double fmid {0.0};

	double fend {0.0};
	
	// game phase in this position
	int phase {0};
	
	// playing side
	Side side {black};
	
	Position() = default;
	
	Position(std::size_t st, std::size_t en, double r, double ev, double fm, 
		double fe, int p, Side s) : index_start(st), index_end(en), result(r), 
		eval(ev), fmid(fm), fend(fe), phase(p), side(s)
		{}
};

////////////////////////////////////////////////////////////////////////////////
class ChessTunerThreadData {
	
	public:
	
		// temporary array used by updateGradients function
		std::vector<Parameter> local {};
	
		// total error
		double error {0.0};
		
		ChessTunerThreadData() = default;
};

////////////////////////////////////////////////////////////////////////////////
// Texel's tuning method with Gradient descent
class ChessTuner {

		ChessEval eval;

		ChessPawnHashTable pawnHash;
	
		std::vector<std::thread> threads;
		
		std::vector<ChessTunerThreadData> threadsdata;
			
		// fen strings
		std::vector<std::string> positions_fen {};
		
		// parameters to tune default value
		std::vector<Parameter> parameters_default {};

		// current parameters
		std::vector<Parameter> parameters {};
	
		// training positions
		std::vector<Position> positions {};
	
		// all coefficients
		std::vector<EvalCoef> coefs {};
		
		// gradients for each parameters
		std::vector<Parameter> gradients {};

		std::vector<EvalScore> parameters_current {};
	
		// temporary array used by updateGradients function
		std::vector<Parameter> local {};

		std::string filePositions;

		std::size_t maxThreads;
		
		std::size_t batch_size;

		// display results interval
		std::size_t display_interval;

		// stop after n iteration
		std::size_t stop_iteration;

		// Gradient descent learning rate
		double learning_rate;
		
		double reduce_learning_rate;

		std::vector<std::string> parametersNames;
	
	public:
	
		ChessTuner(const std::string &fileepd, std::size_t nbthreads, 
			std::size_t nbbatch, std::size_t display, std::size_t stop, 
			double rate, double reduce) : filePositions(fileepd), 
			maxThreads(nbthreads), batch_size(nbbatch), 
			display_interval(display), stop_iteration(stop), 
			learning_rate(rate), reduce_learning_rate(reduce)
		{}
	
		bool run();
		
	private:
	
		// load all positions and initialize all parameters
		[[nodiscard]]
		bool init();
		
		void loadConfigFile(const std::string &filename);

		[[nodiscard]]
		bool loadPositions(const std::string &filename, 
			std::vector<std::string> &fens);
		
		void initParameters(const ChessEval &eval);
		
		// get game result from fen position
		// 0.0 = lost, 1.0 = win, 0.5 = draw
		[[nodiscard]]
		double readGameResult(const std::string &fen) const;
			
		// save parameters in CSV format
		void saveIterations(const std::string &filename,
			std::size_t iteration, double error);
			
		void saveParameters(const std::string &filename);
	
		// optimize parameters with gradient descent
		void optimize();
		
		// recalculate all gradients
		void updateGradients(std::size_t offset, std::size_t count, double k);
		
		// update current parameters with gradients
		void updateParameters(std::size_t nbpositions, double rate);		
		
		[[nodiscard]]
		double singleLinearError(const Position &pos, double k);
	
		[[nodiscard]]	
		double completeLinearError(double k);
		
		[[nodiscard]]		
		double linearEvaluation(const Position &pos);
		
		[[nodiscard]]
		double completeEvaluationError(double k);
		
		// calculate k constant
		[[nodiscard]]
		double calculateK();
};

} // namespace Cheese

#endif // CHEESE_TUNER_H_

