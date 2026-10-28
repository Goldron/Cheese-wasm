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

#include "playgames.h"
#include "moveparser.h"
#include "tablebases.h"

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>

#include <thread>
#include <mutex>
#include <condition_variable>

using namespace Cheese;

////////////////////////////////////////////////////////////////////////////////
PGNBook::PGNBook()
{
	engine.init(1);	
}

////////////////////////////////////////////////////////////////////////////////
bool PGNBook::loadPGNIndex(const std::string &filename)
{
	Logger::info() << "indexing";

	pgnfile = filename;

	if (!loadIndex(filename, pgnindex)) {
		Logger::info() << "book not found";
		return false;
	}

	Logger::info() << "found " << pgnindex.size() << " games.";

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool PGNBook::loadPGNGame(std::size_t id)
{
	count = id;
	eco = "";
	if (!load(pgnfile, pgnindex[id].position, pgnindex[id].size)) {
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void PGNBook::onStartGame([[maybe_unused]] ChessPGN::PGNParser &parser,
			[[maybe_unused]] std::size_t id) 
{
	ChessBoard &board = engine.getBoardRef();
	board.setStartPosition();
	board.setPly(0);

	moves.clear();
}

////////////////////////////////////////////////////////////////////////////////
void PGNBook::onEndGame([[maybe_unused]] ChessPGN::PGNParser &parser,
	[[maybe_unused]] std::size_t id) 
{
}

////////////////////////////////////////////////////////////////////////////////	
void PGNBook::onParseDeclaration([[maybe_unused]] ChessPGN::PGNParser &parser,
	[[maybe_unused]] const std::string_view &name,
	[[maybe_unused]] const std::string_view &value) 
{
	if ((name == "ECO") || (name == "Eco")) {
		eco = value;
	}
}

////////////////////////////////////////////////////////////////////////////////
void PGNBook::logErrorMove(ChessPGN::PGNParser &parser, 
	const std::string_view &move)
{
	ChessBoard &board = engine.getBoardRef();
	auto mc = (parser.getMoveCount() / 2) + 1;
	auto pc = parser.getMoveCount() + 1;
	Logger::info() << "illegal position : " 
		<< mc
		<< ((pc & 1) ? ". " : "..")
		<< move;
	Logger::info() << "Game " << count + 1;
	Logger::info() << "Line " << parser.getTokenLine()
		<< ", Column " << parser.getTokenColumn();
	Logger::info() << board;
	Logger::info() << ChessFENParser::get(board);
}

////////////////////////////////////////////////////////////////////////////////
void PGNBook::onParseMove([[maybe_unused]] ChessPGN::PGNParser &parser,
	[[maybe_unused]] const std::string_view &move) 
{
	ChessBoard &board = engine.getBoardRef();

	ChessMove mv;
	bool r = ChessMoveParser::move_from_string_san(move, board, mv);
	if (r) {
		if (board.validMove(mv)) {

			board.makeMove(mv);
			if (!board.isPositionLegal()) {
				Logger::info() << "illegal move";
				parser.cancelParsing();
				logErrorMove(parser, move);
			}	

			moves.push_back(mv);

		} else {
			Logger::info() << "invalid move";
			parser.cancelParsing();
			logErrorMove(parser, move);
		}

	} else {
		Logger::info() << "illegal move parsed";
		parser.cancelParsing();
		logErrorMove(parser, move);
	}
}

////////////////////////////////////////////////////////////////////////////////		
void PGNBook::onError([[maybe_unused]] ChessPGN::PGNParser &parser,
	[[maybe_unused]] const std::string &str, 
	[[maybe_unused]] std::size_t line, 
	[[maybe_unused]] std::size_t col,
	[[maybe_unused]] char c) 
{
}

////////////////////////////////////////////////////////////////////////////////
ChessGameThread::ChessGameThread(ChessGamePlayer &gp) : gamePlayer(gp)
{
	engine.registerListener(this);

	engine.init(gamePlayer.hash_size);
	engine.setMaxThreads(1);
	engine.setSMPMode(SMPMode::ybwc);	

	engine.setTBProbeDepth(1);
	engine.setTB50MoveRule(true);
	engine.setTBProbeLimit(0);

	std::random_device rd;	
	std::seed_seq sd { rd(), rd(), rd(), rd() };
	mtrnd.seed(sd);
}

////////////////////////////////////////////////////////////////////////////////
ChessGameThread::~ChessGameThread()
{
	engine.unregisterListener(this);
}

////////////////////////////////////////////////////////////////////////////////
ChessGamePlayer::ChessGamePlayer()
{
	std::random_device rd;	
	std::seed_seq sd { rd(), rd(), rd(), rd() };
	mtrnd.seed(sd);
}

////////////////////////////////////////////////////////////////////////////////
void ChessGamePlayer::init(std::size_t nbthreads, const std::string &file, 
	const std::string &fileepdout, 	
	const std::string &filebook, std::size_t nbbook, const std::string &tc,
	int maxdepth, bool fullrandom, bool minpgn, bool quietpos, int hashsize, 
	const std::string &tbpath, int mingameply, int minply, int maxply,
	int maxscore, std::size_t nbsamples, int depthrand, int maxrand, 
	int distrand, int multipvrand)
{
	nb_threads = nbthreads;
	full_random = fullrandom;
	min_pgn = minpgn;
	quiet = quietpos;
	hash_size = hashsize;
	tc_depth = maxdepth;
	min_game_ply = mingameply;
	min_ply = minply;
	max_ply = maxply;
	max_score = maxscore;
	samples = nbsamples;

	tc_depth_random = depthrand;
	randommoves = maxrand;
	randomdist = distrand;
	multipv = multipvrand;

	filepgn = file;
	fileepd = fileepdout;
	fileopenings = filebook;
	tb_path = tbpath;
	bookmoves = nbbook;

	TableBases::init(tb_path);

	if (!fileopenings.empty()) {
		if (!pgnBook.loadPGNIndex(fileopenings)) {
			fileopenings = "";
		}
	}

	for (std::size_t n=0U; n<nb_threads; n++) {
		games.emplace_back(std::make_unique<ChessGameThread>(*this));
	}

	// TODO : 40/60:0+1 
	if (tc.empty()) {
		tc_moves = 0;
		tc_time = 0;
		tc_inc = 0;
	} else {
		auto p1 = tc.find_first_of('/');
		if (p1 != std::string::npos) {
			auto s1 = tc.substr(0, p1);
			tc_moves = std::stoi(s1);

			auto p2 = tc.find_first_of('+', p1);
			if (p2 != std::string::npos) {
				auto s2 = tc.substr(p1 + 1, p2);
				auto s3 = tc.substr(p2 + 1);
				tc_time = std::stoi(s2);
				tc_inc = static_cast<int>(std::floor(std::stod(s3) * 1000.0));
			} else {
				tc_time = std::stoi(tc);
			}

		} else {
			auto p2 = tc.find_first_of('+');
			if (p2 != std::string::npos) {
				auto s2 = tc.substr(0, p2);
				auto s3 = tc.substr(p2 + 1);
				tc_time = std::stoi(s2);
				tc_inc = static_cast<int>(std::floor(std::stod(s3) * 1000.0));
			} else {
				tc_time = std::stoi(tc);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessGamePlayer::playGames(int count)
{
	if (!filepgn.empty()) {
		ofs.open(filepgn, std::ofstream::out);			
	}

	if (!fileepd.empty()) {
		ofsepd.open(fileepd, std::ofstream::out);			
	}

	Timer timer;
	timer.start();

	game_id = 0;
	game_count = count;

	for (std::size_t n=0U; n<nb_threads; n++) {

		Logger::info() << "start thread " << n + 1;

		threads.emplace_back([this](std::size_t id) {

			while (game_count > 0) {

				--game_count;

				auto &game = games[id];
				game->game_id = game_id++;
				game->tc_moves = tc_moves;
				game->tc_time = tc_time;
				game->tc_inc = tc_inc;
				game->tc_depth = tc_depth;
				game->tc_depth_random = tc_depth_random;
		
				game->playGame();
			}

		}, n);

	}

	Logger::info() << "end threads : ";
	for (std::size_t n=0U; n<nb_threads; n++) {
		threads[n].join();
		Logger::info() << "stop thread " << n + 1;
	}

	auto timegame = timer.getElapsedTime();
	auto str = Timer::timeToString(timegame);

	std::cout << "\nFinished in " << str << '\n';
	std::cout << count << " games, " << countPositions << " positions ("
		<< ((static_cast<double>(count) * 1000.0) / 
			static_cast<double>(timegame)) * 3600
		<< " games/h) ("
		<< ((static_cast<double>(countPositions) * 1000.0) / 
			static_cast<double>(timegame)) * 3600
		<< " positions/h)"
		<< std::endl;

	std::cout << "\nScore : " << count_win << " - "
		<< count_loss << " - "
		<< count_draw 
		<< " (" << (count_draw * 100) / count << " % draw)\n";

	if (count_error != 0) {
		std::cout << "\nUnfinished games : " << count_error << '\n';
	}
}

////////////////////////////////////////////////////////////////////////////////
bool ChessGameThread::testGameTermination(std::string &strtermination, 
	std::string &strend, std::string &strresult)
{
	ChessBoard &board = engine.getBoardRef();
	ChessRepetitions repetitions = engine.getRepetitions();

	if (board.inCheck() && engine.isMate(board)) {
		strtermination = "";
		if (board.side() == black) {
			strresult = "1-0";
			strend = "White wins";
			++gamePlayer.count_win;
		} else {
			strresult = "0-1";
			strend = "Black wins";
			++gamePlayer.count_loss;
		}
		return true;
	}

	if (tc_depth == 0) {
		if (timew < 0) {
			strtermination = "adjudication";
			strresult = "0-1";
			strend = "White forfeits on time";
			++gamePlayer.count_loss;
			return true;
		}

		if (timeb < 0) {
			strtermination = "adjudication";
			strresult = "1-0";
			strend = "Black forfeits on time";
			++gamePlayer.count_win;
			return true;
		}
	}

	if (board.evaluateDraw()) {
		strtermination = "";
		strresult = "1/2-1/2";
		strend = "Draw by insuficient material";
		++gamePlayer.count_draw;
		return true;
	}

	if (board.fiftyRule() >= 100) {
		strtermination = "";
		strend = "Draw by 50 moves rule";
		strresult = "1/2-1/2";
		++gamePlayer.count_draw;
		return true;
	}

	if (repetitions.getCount(board.getHashKey(), board.fiftyRule()) > 2) {
		strtermination = "";
		strresult = "1/2-1/2";
		strend = "Draw by 3-fold repetition";
		++gamePlayer.count_draw;
		return true;
	}

	moves.clear();
	ChessMoveGen::generateLegalMoves(board, moves);
	if (moves.empty()) {
		strtermination = "";
		strresult = "1/2-1/2";
		strend = "Draw by stalemate";
		++gamePlayer.count_draw;
		return true;
	}

	if ((gamePly > gamePlayer.draw_ply) && 
		(std::abs(bestScore) < gamePlayer.draw_score)) {
		++countDraw;
		if (countDraw > gamePlayer.draw_count * 2) {
			strtermination = "adjudication";
			strresult = "1/2-1/2";
			strend = "Draw by adjudication";
			++gamePlayer.count_draw;
			return true;
		}
	} else {
		countDraw = 0;
	}
	
	if ((std::abs(bestScore) > gamePlayer.resign_score)) {
		++countScore;
		if (countScore > gamePlayer.resign_count * 2) {
			strtermination = "adjudication";
			if (board.side() == black) {
				if (bestScore < 0) {
					strresult = "0-1";
					strend = "Black wins by adjudication";
					++gamePlayer.count_loss;
				} else {
					strresult = "1-0";
					strend = "White wins by adjudication";
					++gamePlayer.count_win;
				}
			} else {
				if (bestScore < 0) {
					strresult = "1-0";
					strend = "White wins by adjudication";
					++gamePlayer.count_win;
				} else {
					strresult = "0-1";
					strend = "Black wins by adjudication";
					++gamePlayer.count_loss;
				}
			}
			return true;
		}
	} else {
		countScore = 0;
	}

	if (TableBases::largest() != 0) {
		int score = 0;
		int r = TableBases::probe_wdl(board, score);	

		if (r != 0) {
			if (score == 0) {
				strtermination = "tablebase adjudication";
				strresult = "1/2-1/2";
				strend = "Draw by adjudication";
				++gamePlayer.count_draw;
				return true;			
			}
			if (score == -2) {
				strtermination = "tablebase adjudication";
				strresult = "1-0";
				strend = "White wins by adjudication";
				++gamePlayer.count_win;
				return true;
			}
			if (score == 2) {
				strtermination = "tablebase adjudication";
				strresult = "0-1";
				strend = "Black wins by adjudication";
				++gamePlayer.count_loss;
				return true;
			}
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////
void ChessGamePlayer::loadOpeningMoves(std::size_t num, 
	ChessMoveSortList &moves)
{
	std::lock_guard<std::mutex> lk(mutex_write);

	pgnBook.loadPGNGame(num);
	moves.clear();
	for (auto n=0U; n<pgnBook.countMoves(); n++) {
		moves.push(pgnBook.getMove(n));
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessGameThread::samplePositions(std::vector<ChessGamePosition> &positions)
{
	std::uniform_int_distribution<std::size_t> dist(0, positions.size() - 1);

	const int dmin = 4;
	const std::size_t maxretry = 8;

	std::size_t nb = std::min(gamePlayer.samples, positions.size() / dmin);

	std::vector<int> samples;
	samples.reserve(nb);

	// try to limit distance between sampled moves
	for (std::size_t n=0U; n<nb; ++n) {
		std::size_t index = 0;
		bool ok = false;
		std::size_t ret = 0;
		while ((!ok) && (ret < maxretry)) {
			index = dist(mtrnd);
			auto i = positions[index].ply;
			ok = true;
			for (const auto &s : samples) {
				auto d = (s > i) ? (s - i) : (i - s);
				if (d < dmin) {
					ok = false;
					++ret;
					break;
				}
			}
		}
		if (ok) {
			samples.push_back(positions[index].ply);
			positions[index].sampled = true;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
ChessMove ChessGameThread::getFullRandomMove()
{
	moves.clear();
	ChessBoard &board = engine.getBoardRef();
	ChessMoveGen::generateLegalMoves(board, moves);
	auto count = moves.size();
	int i = 0;
	if (count > 1) {
		std::uniform_int_distribution<int> dist(0, count - 1);
		i = dist(mtrnd);
	}
	return moves[i].move;
}

////////////////////////////////////////////////////////////////////////////////
ChessMove ChessGameThread::getRandomMove()
{
	moves.clear();
	ChessBoard &board = engine.getBoardRef();
	ChessMoveGen::generateLegalMoves(board, moves);
	auto count = moves.size();

	if (count == 0) {
		return ChessMove(0);
	}

	if (count == 1) {
		return moves.front().move;
	}

	std::vector<ChessMove> searchedmoves;
	ChessMoveList excludedmoves;

	int nbm = std::min(gamePlayer.multipv, count);
	int margin = gamePlayer.randomscoremargin;
	int topscore = 0;

	engine.setSearchFixedDepth(tc_depth_random);
	engine.setSearchFixedNodes(0);
	engine.setSearchFixedTime(0);
	engine.setPlayerTime(black, 0);
	engine.setPlayerTime(white, 0);
	engine.setTimeIncrement(white, 0);
	engine.setTimeIncrement(black, 0);

	// multipv
	for (int n=0; n<nbm; n++) {
		engine.setSearchExcludedMoves(excludedmoves);

		engine.startSearch(false);
		engine.waitEndSearch();

		if (n == 0) {
			topscore = bestScore;
		}

		if ((n == 0) ||
			(std::abs(topscore - bestScore) <= margin)) {
			searchedmoves.push_back(bestMove);
			excludedmoves.push(bestMove);
		}
	}

	engine.clearSearchExcludedMoves();

	if (searchedmoves.size() > 1) {
		std::uniform_int_distribution<int> dist(0, 
			static_cast<int>(searchedmoves.size() - 1));
		auto i = dist(mtrnd);
		return searchedmoves[i];
	}

	return searchedmoves.front();
}

////////////////////////////////////////////////////////////////////////////////
void ChessGameThread::playGame()
{
	gamePlayer.output("Started game " + std::to_string(game_id));	

	ChessBoard &board = engine.getBoardRef();

	engine.newGame();
	gameRandomPos = 0;
	gameRandomMoves = 0;

	std::string strtc;
	if (tc_depth == 0) {
		std::stringstream sstr;
		if (tc_moves != 0) {
			sstr << tc_moves << '/';
		}
		sstr << tc_time;
		if (tc_inc != 0) {
			sstr << '+' << tc_inc;
		}
		strtc = sstr.str();
	} else {
		strtc = "inf";
	}

	std::vector<ChessGamePosition> positions;

	ChessPGN::PGNFileData game;

	std::string strmove;
	std::string strresult;
	std::string strend;
	std::string strtermination;

	gamePly = 1;
	countDraw = 0;
	countScore = 0;

	timew = tc_time * 1000;
	timeb = tc_time * 1000;

	int tinc = tc_inc;	
	int dt = 0;

	if (!gamePlayer.fileopenings.empty()) {
		auto i = gamePlayer.getRandomOpeningId();
		gamePlayer.loadOpeningMoves(i, moves);
	}

	engine.setSearchFixedNodes(0);
	engine.setSearchFixedTime(0);

	Timer timer;
	timer.start();

	bool rndmove = false;

	while (true) {

		rndmove = false;

		bool bookmove = (gamePlayer.full_random) || 
			(gamePly < static_cast<int>(gamePlayer.bookmoves + 1));

		if (!gamePlayer.fileopenings.empty()) {
			if ((gamePly - 1) >= static_cast<int>(
				gamePlayer.pgnBook.countMoves())) {
				bookmove = false;
			}
		}

		bestMove.clear();

		if (bookmove) {

			if (!gamePlayer.fileopenings.empty()) {
				bestMove = moves[gamePly - 1].move;
			} else {
				// random book move
				bestMove = getFullRandomMove();
			}

		} else {
	
			if ((gamePlayer.randommoves != 0) &&
				(gameRandomMoves < gamePlayer.randommoves) &&
				((gamePly - gameRandomPos) >= gamePlayer.randomdist)) {

				// random move using multi pv
				bestMove = getRandomMove();
				rndmove = true;

				++gameRandomMoves;	
				gameRandomPos = gamePly;

			} else {
			
				// normal search
				auto t1 = timer.getElapsedTime();

				if (tc_depth == 0) {
					engine.setSearchFixedDepth(0);
					engine.setPlayerTime(black, timeb);
					engine.setPlayerTime(white, timew);
					engine.setTimeIncrement(white, tinc);
					engine.setTimeIncrement(black, tinc);
				} else {
					engine.setSearchFixedDepth(tc_depth);
					engine.setPlayerTime(black, 0);
					engine.setPlayerTime(white, 0);
					engine.setTimeIncrement(white, 0);
					engine.setTimeIncrement(black, 0);
				}

				engine.startSearch(false);
				engine.waitEndSearch();

				if (tc_depth == 0) {
					auto t2 = timer.getElapsedTime();	
					dt = static_cast<int>(t2 - t1);

					if (board.side() == white) {
						timew = timew - dt + tinc;
					} else {
						timeb = timeb - dt + tinc;
					}
				}
			}
		}

		strmove = ChessMoveParser::move_to_string_san(bestMove, board);

		if (!engine.playMove(bestMove)) {
			Logger::info() << "game " << game_id << ", ply = " << gamePly;
			Logger::info() << "Illegal move : " << strmove;
			Logger::info() << board;
			Logger::info() << ChessFENParser::get(board);

			strresult = "*";
			strtermination = "*";
			positions.clear();
			++gamePlayer.count_error;
			break;
		}

		if (board.inCheck()) {			
			if (engine.isMate(board)) {
				strmove += '#';
			} else {
				strmove += '+';
			}
		}

		if (bookmove) {
			game.moves.emplace_back(strmove, "book");
		} else {
			if (rndmove) {
				game.moves.emplace_back(strmove, "random");
			} else {
				if (gamePlayer.min_pgn) {
					game.moves.emplace_back(strmove, "");
				} else {
					std::stringstream ss;
					ss << static_cast<double>(bestScore) / 100.0 << '/'
					   << std::to_string(searchDepth) << ' '
					   << static_cast<double>(searchTime) / 1000.0 << 's';
					game.moves.emplace_back(strmove, ss.str());
				}
			}
		}
		
		if ((!bookmove) && 
			(testGameTermination(strtermination, strend, strresult))) {
			if (gamePlayer.min_pgn) {
				game.moves.back().comment = strend;
			} else {
				game.moves.back().comment += ", " + strend;
			}
			break;
		}

		// filter positions
//		int nbp = BitBoards::bitCount(board.bitboardAll());
		if ((!bookmove) &&
			(std::abs(bestScore) < gamePlayer.max_score) &&
			(gamePly >= gamePlayer.min_ply) && 
			(gamePly <= gamePlayer.max_ply) &&
//			(nbp > 3) &&
			(!board.inCheck()) &&
			((!gamePlayer.quiet) || ChessEngine::isPositionQuiet(board))) {

			positions.emplace_back(engine.getFENPosition(board),
				gamePly, false);
		}
	
		++gamePly;
	}

	auto timegame = timer.getElapsedTime();

	gamePlayer.output("Finished game " + std::to_string(game_id));	

	gamePlayer.output("  Score : " + 
		std::to_string(gamePlayer.count_win) + " - " +
		std::to_string(gamePlayer.count_loss) + " - " +
		std::to_string(gamePlayer.count_draw));

	std::string strdate, strtime;

	auto now = std::chrono::system_clock::now();
	auto tc = std::chrono::system_clock::to_time_t(now);

	std::tm ltm{};
	#ifdef WIN32
	localtime_s(&ltm, &tc);
	#else
	localtime_r(&tc, &ltm);
	#endif
	
	std::stringstream sstr1;
	sstr1 << std::put_time(&ltm, "%Y/%m/%d");
	strdate = sstr1.str();
	std::stringstream sstr2;
	sstr2 << std::put_time(&ltm, "%H:%M:%S");
	strtime = sstr2.str();

	game.headers.emplace_back("Event", "Cheese Tuning");
	game.headers.emplace_back("Site", "cheese_tuner");
	game.headers.emplace_back("White", "Cheese " + std::string(project_ver) + 
		"_a");
	game.headers.emplace_back("Black", "Cheese " + std::string(project_ver) + 
		"_b");
	game.headers.emplace_back("PlyCount", std::to_string(gamePly));
	game.headers.emplace_back("Date", strdate);
	game.headers.emplace_back("Time", strtime);
	game.headers.emplace_back("GameDuration", Timer::timeToString(timegame));
	game.headers.emplace_back("Round", std::to_string(game_id + 1));
	game.headers.emplace_back("TimeControl", strtc);
	if (!strtermination.empty()) {
		game.headers.emplace_back("Termination", strtermination);
	}
	game.headers.emplace_back("Result", strresult);
	if ((!gamePlayer.fileopenings.empty()) && 
		(!gamePlayer.pgnBook.getECO().empty())) {
		game.headers.emplace_back("ECO", gamePlayer.pgnBook.getECO());		
	}

	game.result = strresult;

	ChessPGN::PGNLoader pgn;
	std::string str = pgn.create(game);

	// save game
	if (!gamePlayer.filepgn.empty()) {
		gamePlayer.writePGN(str);
	}

	// save positons
	if ((!gamePlayer.fileepd.empty()) &&
		(gamePly >= gamePlayer.min_game_ply)) {

		samplePositions(positions);
		gamePlayer.writeEPD(positions, strresult);
	}
}

////////////////////////////////////////////////////////////////////////////////
std::size_t ChessGamePlayer::getRandomOpeningId()
{
	std::lock_guard<std::mutex> lk(mutex_write);

	std::uniform_int_distribution<std::size_t> dist(0, 
		pgnBook.countGames() - 1);
	return dist(mtrnd);	
}

////////////////////////////////////////////////////////////////////////////////
void ChessGamePlayer::writePGN(const std::string &str)
{
	std::lock_guard<std::mutex> lk(mutex_write);
	ofs << str << '\n';
}

////////////////////////////////////////////////////////////////////////////////
void ChessGamePlayer::writeEPD(const std::vector<ChessGamePosition> &positions, 
	const std::string  &strresult)
{
	std::lock_guard<std::mutex> lk(mutex_write);

	for (const auto &p : positions) {
		if ((samples == 0) || p.sampled) {
			ofsepd << p.fen << " c9 \"" << strresult << "\";\n";
			++countPositions;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessGamePlayer::output(const std::string &str)
{
	std::lock_guard<std::mutex> lk(mutex_write);
	std::cout << str << '\n';	
}

