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

#include "engine.h"

#include "bitbase.h"
#include "fen.h"
#include "moveorder.h"
#include "moveparser.h"
#include "tablebases.h"

#include "util/logfile.h"
#include "util/tokenizer.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace Cheese {

// default maximum number of moves in a game
constexpr int max_moves = 256;

// maximum number of moves in an opening book
constexpr int max_book_moves = 20;

constexpr std::size_t nb_elo_samples = 13;
using ELOArray = std::array<int, nb_elo_samples>;

const ELOArray elo_speed_values = { 50, 100, 500, 1000, 5000, 10000, 50000, 
100000, 250000, 500000, 1000000, 2000000, 3000000 };

const ELOArray elo_speed_ratings = { 974, 1026, 1397, 1616, 1947, 2113, 
2446, 2539, 2697, 2771, 2823, 2937, 2986 };

const ELOArray elo_nodes_values = { 50, 100, 500, 1000, 5000, 10000, 50000, 
100000, 250000, 500000, 1000000, 2000000, 3000000 };

const ELOArray elo_nodes_ratings = { 752, 753, 959, 1149, 1623, 1831, 2228, 
2404, 2598, 2744, 2850, 2853, 2868 };

// fix classic chess castling before sending to GUI
// the engine moves king to rook square for castling,
// but GUI wants normal castling moves
constexpr std::array<Square, nb_squares> castlefix = {
	c1, b1, c1, d1, e1, f1, g1, g1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	c8, b8, c8, d8, e8, f8, g8, g8
};

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::precalcTables()
{
	if constexpr (BitBases::generate_bitbase) {
		BitBases::generateKPK();
	}

	// precalc bitboards tables
	BitBoards::init();
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::init(int hashsize, int nt)
{
	Logger::debug() << "Engine init : " << hashsize;

	setMaxThreads(nt);

	hashTable.init(hashsize, nt);

	Logger::info() << "Pawns hash table size : " << hashtable_pawn_size
				   << " Kb by thread";

	eval.initEvalData();

	repetition.clear();

	setStartPosition();

	gameMoves.reserve(max_moves);
	undoMoves.reserve(max_moves);
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::free()
{
	Logger::debug() << "Engine free";

	// stop search
	pondering = false;

	if (isThinking()) {
		stopThinking();
	}

	threads.destroy();
	hashTable.release();
}

////////////////////////////////////////////////////////////////////////////////
// linear interpolation between ratings/value samples
int calcStrengthLimit(int elo, const ELOArray &ratings, const ELOArray &values)
{
	const int vmin = values[0];
	const int vmax = values[values.size() - 1];

	if (elo <= ratings[0]) {
		return vmin;
	}
	if (elo >= ratings[ratings.size() - 1]) {
		return vmax;
	}

	std::size_t p2 = 0;
	for (auto p=1U; p<ratings.size(); ++p) {
		if ((elo > ratings[p - 1]) && (elo <= ratings[p])) {
			p2 = p;
			break;
		}
	}

	auto p1 = p2 - 1;

	double a = static_cast<double>(values[p2] - values[p1]) /
			   static_cast<double>(ratings[p2] - ratings[p1]);
	double b = static_cast<double>(values[p1]) - a * 
			   static_cast<double>(ratings[p1]);

	double v = std::clamp(a * elo + b, static_cast<double>(vmin), 
		static_cast<double>(vmax));

	return static_cast<int>(std::floor(v));
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::setStrengthELO(int e)
{
	strengthELO = std::clamp(e, min_strength_elo, max_strength_elo);
	useCustomStrength = (strengthELO < max_strength_elo);

	eloNodesLimit = 0;
	reduceSpeedNodes = 0;

	if (strengthELO < max_strength_elo) {

		if (eloLimitMode == ELOLimitMode::nodes) {

			// limit nodes
			eloNodesLimit = calcStrengthLimit(e, elo_nodes_ratings, 
				elo_nodes_values);

			Logger::info() << "set engine strength to " << strengthELO
					       << " ELO (" << eloNodesLimit << " nodes)";
		} else {

			// init reduce speed
			reduceSpeedNodes = calcStrengthLimit(e, elo_speed_ratings, 
				elo_speed_values);

			Logger::info() << "set engine strength to " << strengthELO
					       << " ELO (" << reduceSpeedNodes << " node/s)";
		}

	} else {
		Logger::info() << "set engine strength to maximum";
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::setStrengthLevel(int l)
{
	Logger::info() << "use engine level instead of ELO settings";
	setStrengthELO(min_strength_elo + (l - 1) * 100);
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::newGame()
{
	const int defaultGameLength = 5 * 60 * 1000;

	movesOutOfBook = 0;
	pondering = false;
	thinkDepth = 0;
	ideepNodeCountTime = 0;
	remainingmoves = 30;
	timeIncrement[white] = 0;
	timeIncrement[black] = 0;
	ideepTimeByMove = defaultGameLength / remainingmoves;
	fixedNodesValue = 0;
	fixedTimeValue = 0;
	timeGame[black] = defaultGameLength;
	timeGame[white] = defaultGameLength;
	infiniteSearch = false;

	board.clear();
	gameMoves.clear();
	undoMoves.clear();
	repetition.clear();

	setStartPosition();
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::errorMoveFromGUI(const std::string &text)
{
	std::string str = "Illegal move: " + text;
	onSendInfoMessage(str);
	Logger::error() << str;
	Logger::info() << getFENPosition(board);
	Logger::info() << board;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEngine::playMovesFromString(const std::string_view &text)
{
	Tokenizer tk(text);
	ChessMove move;
	while (tk.hasMoreTokens()) {

		auto token = tk.nextToken();

		bool r = ChessMoveParser::move_from_string(token, board, move);

		if ((!r) ||
			(!playMove(move))) {
			errorMoveFromGUI(std::string(token));
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEngine::playMove(ChessMove move)
{
	const HashKey key = board.getHashKey();

	if (!board.validMove(move)) {
		return false;
	}

	const ChessMoveRec rec = board.makeMove(move);

	if (board.isPositionLegal()) {
		repetition.push(key);
		if (!move.isReversible()) {
			repetition.clear();
		}
		gameMoves.push_back(move);
		undoMoves.push_back(rec);
	} else {
		board.unMakeMove(move, rec);
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEngine::takeBack()
{
	if (getGameMoveCount() > 0) {
		ChessMove &move = gameMoves.back();
		ChessMoveRec &rec = undoMoves.back();

		board.unMakeMove(move, rec);

		gameMoves.pop_back();
		repetition.pop();

		// rebuild repetitions
		repetition.clear();

		for (int n=0; n<getGameMoveCount(); n++) {
			repetition.push(undoMoves[n].hashKey);
			if (!gameMoves[n].isReversible()) {
				repetition.clear();
			}
		}

		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
// calculate mate value in number of full moves
int ChessEngine::calcMateValue(int value)
{
	if (value <= min_score_mate) {
		return (mate_value + value) / 2;
	}
	if (value >= max_score_mate) {
		return (mate_value - value) / 2 + 1;
	}
	return value;
}

////////////////////////////////////////////////////////////////////////////////
// test : extract PV from hash table
void ChessEngine::extractPV(int depth)
{
	std::array<ChessMove, 32> pvmove;
	std::array<ChessMoveRec, max_depth> moveUndo;
	ChessMove cm;
	int pvdepth = depth - 1;
	int nbpv = 0;

	while (pvdepth >= 0) {

		cm.clear();

		if (!board.validMove(cm)) {
			break;
		}

		moveUndo[nbpv] = board.makeMove(cm);
		if (!board.isPositionLegal()) {
			break;
		}

		pvmove[nbpv++] = cm;

		--pvdepth;
	}

	for (pvdepth = (nbpv - 1); pvdepth>=0; pvdepth--) {
		ChessMoveRec &rec = moveUndo[pvdepth];
		board.unMakeMove(pvmove[pvdepth], rec);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::initSearch()
{
	ideepMaxDepth = (thinkDepth == 0) ? (max_depth - 1) : thinkDepth;

	ideepNodeCountTime = 0;
	ideepNodeCheckTime = check_time_interval;
	if (fixedNodesValue != 0) {
		ideepNodeCheckTime = std::min(check_time_interval, 
			fixedNodesValue);
	}
	ideepTimeByMove = 0;

	if (!infiniteSearch) {

		if ((thinkDepth != 0) &&
			(fixedTimeValue == 0) && 
			(getTime(board.side()) == 0)) {
			return;
		}

		if (fixedTimeValue != 0) {
			ideepTimeByMove = fixedTimeValue;
		} else {
			int tm = getTime(board.side());
			if (remainingmoves != 0) {
				ideepTimeByMove = tm / std::max(remainingmoves, 2);
			} else {
				ideepTimeByMove = tm / 30;
			}

			ideepTimeByMove += timeIncrement[board.side()];

			// give more time for the 10 first moves out of book
			// (first move = x2 time)
			if (movesOutOfBook <= 9) {
				double r = 2.0 - movesOutOfBook / 10.0;
				ideepTimeByMove = static_cast<std::uint64_t>(
					static_cast<double>(ideepTimeByMove) * r);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::startSearch(bool ponder)
{
	// search start time
	timerSearch.start();

	threads.waitEndSearch();

	// engine side
	engineSide = board.side();
	eval.setEngineSide(engineSide);

	// generate legal moves in current position
	ChessMoveGen::generateLegalMoves(board, movesRoot);

	// no legal moves : should never happen
	if (movesRoot.empty()) {
		onEndSearch(ChessMove(0), ChessMove(0), 0, 0, 0ULL, 0);
		return;
	}

	// search in opening book
	if (useOpeningBook &&
		(!pondering) &&
		(getGameMoveCount() < max_book_moves) &&
		(!infiniteSearch)) {

		ChessMove bestmove = openingBook.findMove(board);

		if (!bestmove.isEmpty()) {

			// test if move is valid
			bool tstbook = false;
			for (const auto &m : movesRoot) {
				if (m.move == bestmove) {
					tstbook = true;
					break;
				}
			}

			if (tstbook) {
				Logger::debug() << "opening book move : " 
					<< ChessMoveParser::move_to_string_lan(bestmove);
				onEndSearch(bestmove, ChessMove(0), 1, 1, 0ULL, 0);
				pondering = false;
				return;
			} else {
				Logger::warning() << "invalid book move : " 
					<< ChessMoveParser::move_to_string_lan(bestmove);
				bestmove.clear();
			}
		}
	}

	TBCardinality = std::min(TBProbeLimit, TableBases::largest());
	if (TBCardinality > 0) {

		int nbp = BitBoards::bitCount(board.bitboardAll());
		if ((nbp <= TBCardinality) &&
			(!board.canCastle(castle_init))) {

			int score = 0;
			int r = TableBases::probe_root(board, score, movesRoot);
	
			if (r != 0) {
				movesRoot.sort(0, movesRoot.size());				
				threads.getMainThread()->tbhits += movesRoot.size();
				TBCardinality = 0;
			}
		}
	}

	if (!pondering) {
		++movesOutOfBook;
	}

	// init time management and stop conditions
	initSearch();

	// update main hashtable age
	hashTable.initSearch();

	threads.initSearch();
	threads.resetNodeCount();

	pondering = ponder;

	// wake up the main thread to start the search
	threads.getMainThread()->thinking = true;
	threads.getMainThread()->notify();
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::setSearchExcludedMoves(const ChessMoveList &moves)
{
	excludedMoves = moves;
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::setSearchSearchedMoves(const ChessMoveList &moves)
{
	ChessMoveSortList movelist;
	ChessMoveGen::generateLegalMoves(board, movelist);

	excludedMoves.clear();
	for (const auto &m : movelist) {
		if (std::find(moves.begin(), moves.end(), m.move) == moves.end()) {
			excludedMoves.push(m.move);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::setSearchSearchedMovesFromString(const std::string_view &str)
{
	Tokenizer tk(str);

	ChessMoveList moves;
	ChessMove move;

	while (tk.hasMoreTokens()) {
		auto token = tk.nextToken();
		bool r = ChessMoveParser::move_from_string(token, board, move);
		if (r) {
			moves.push(move);
		}
	}	

	setSearchSearchedMoves(moves);
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::reduceSpeed()
{
	if (threads.mustStop()) {
		return;
	}

	std::uint64_t elapsed = getSearchElapsedTime();
	std::uint64_t sp = (getSearchNodeCount() * 1000) / (elapsed + 1);

	if (sp > getReduceSpeedNodes()) {

		// wait if we have enough time
		if ((ideepTimeByMove == 0) ||
			((elapsed + (100 + 16)) < ideepTimeByMove)) {
			Timer::sleep(100);
		}

		// force to check time
		ideepNodeCountTime = ideepNodeCheckTime;
	}
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEngine::checkSearchTime()
{
	if (threads.mustStop()) {
		return false;
	}

	++ideepNodeCountTime;
	if (ideepNodeCountTime < ideepNodeCheckTime) {
		return true;
	}

	ideepNodeCountTime = 0;

	// in pondering : don't stop searching
	if (!pondering) {

		if (fixedNodesValue != 0) {			
			if (allowEndSearchNodes) {
				auto nodes = threads.getNodeCount();
				if ((nodes >= fixedNodesValue)) {
					threads.stopSearch();
					return false;
				}
				if ((nodes + ideepNodeCheckTime) > fixedNodesValue) {
					ideepNodeCheckTime = fixedNodesValue - nodes;
				}
			}
		}

		// compare to time by move
		if (ideepTimeByMove != 0) {
			auto diff = timerSearch.getElapsedTime();
			if (diff >= ideepTimeByMove) {
				threads.stopSearch();
				return false;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::setPersonality(const std::string &fname)
{
	if (eval.loadPersonality(fname)) {
		//...
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::stopThinking()
{
	threads.stopSearch();

	// force to check time
	ideepNodeCountTime = ideepNodeCheckTime;

	threads.waitEndSearch();
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::ponderHit()
{
	pondering = false;
	ideepNodeCountTime = ideepNodeCheckTime;
	checkSearchTime();
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEngine::checkNodes()
{
	std::uint64_t nodes = threads.getNodeCount();
	std::uint64_t maxnodes = (eloNodesLimit != 0) ? eloNodesLimit : 
		fixedNodesValue;
	// don't stop before reaching depth 1
	int depth = threads.getMainThread()->searchDepth;
	if ((depth > 0) && (nodes >= maxnodes)) {
		threads.stopSearch();
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::searchingRootMove(int depth, ChessMove &mv, int num)
{
	onSearchRootMove(depth, mv, num);
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::setStartPosition()
{
	gameMoves.clear();
	undoMoves.clear();
	repetition.clear();
	board.setStartPosition();
}

////////////////////////////////////////////////////////////////////////////////
// set FEN (Forsythe-Edwards Notation)
bool ChessEngine::setFENPosition(const std::string_view &str)
{
	gameMoves.clear();
	undoMoves.clear();
	repetition.clear();

	return ChessFENParser::set(str, board);
}

////////////////////////////////////////////////////////////////////////////////
// get FEN (Forsythe-Edwards Notation)
std::string ChessEngine::getFENPosition(ChessBoard &pos) const
{
	return ChessFENParser::get(pos);
}

////////////////////////////////////////////////////////////////////////////////
std::string ChessEngine::getTextMove(const ChessMove &move) const
{
	if ((move.isCastle()) &&
		(board.getChessRules() == ChessRules::classic)) {
		Square src = move.src();
		Square dst = castlefix[move.dst()];
		ChessMove m = ChessMove(src, dst, king, no_piece, no_piece, 1, 0);
		return ChessMoveParser::move_to_string(m);
	}

	return ChessMoveParser::move_to_string(move);
}

////////////////////////////////////////////////////////////////////////////////
std::string ChessEngine::getTextPV(const ChessPVMoveList &mlist, 
	int count) const
{
	std::string str;
	for (int n=0; n<count; n++) {

		str += getTextMove(mlist[n]);

		if (n < (count - 1)) {
			str += ' ';
		}
	}

	return str;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEngine::loadOpeningBook(const std::string &filebook)
{
	useOpeningBook = openingBook.open(filebook);
	return useOpeningBook;
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::unloadOpeningBook()
{
    useOpeningBook = false;
	openingBook.close();
}

////////////////////////////////////////////////////////////////////////////////
int ChessEngine::countLegalMoves(ChessBoard &pos) const
{
	return ChessMoveGen::countLegalMoves(pos);
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::setSMPMode(SMPMode m)
{
	smpmode = m;
	switch (smpmode) {
		case SMPMode::lazy:
			Logger::info() << "using Lazy SMP";
			break;
		case SMPMode::ybwc:
			Logger::info() << "using YBWC";
			break;
		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngine::showBookMoves()
{
	if (useOpeningBook) {
		openingBook.showMoves(board);
	} else {
		std::cout << "book not loaded" << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEngine::isPositionQuiet(ChessBoard &pos)
{
	ChessMoveSortList movelist;
	ChessMoveGen::generateLegalMoves(pos, movelist);

	for (const auto &move : movelist) {
		const auto &m = move.move;
		if ((m.isCaptureOrPromotion()) &&
			(pos.staticExchangeEvaluation(m) > 0)) {
			return false;
		}
	}
	
	return true;
}

} // namespace Cheese
