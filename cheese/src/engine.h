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

#ifndef CHEESE_ENGINE_H_
#define CHEESE_ENGINE_H_

#include "config.h"

#include "board.h"
#include "polyglot/book.h"
#include "eval.h"
#include "hash.h"
#include "listener.h"
#include "pv.h"
#include "repetitions.h"
#include "search.h"
#include "util/timer.h"
#include "tune.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <string>
#include <vector>

namespace Cheese {

// maximum strength
inline constexpr int max_strength_elo = 3000;

// minimum strength
inline constexpr int min_strength_elo = 800;

// maximum engine level (when used to replace ELO settings)
inline constexpr int max_engine_level = 23;

// check time every N nodes
inline constexpr std::uint64_t check_time_interval = 4096;

// reduce ELO by limiting speed or nodes
enum class ELOLimitMode {
	speed,
	nodes
};

////////////////////////////////////////////////////////////////////////////////
// Chess engine main class
class ChessEngine : public ChessEngineNotifier {

	friend class ChessSearch;

	// chess board current position (not in search)
	ChessBoard board;

	// evaluation functions
	ChessEval eval;

	// main hash table
	ChessHashTable hashTable;

	// repetition table	for current game (not in search)
	ChessRepetitions repetition;

	// move list for current game
	std::vector<ChessMove> gameMoves;

	// undo move list, to be able to take back
	std::vector<ChessMoveRec> undoMoves;

	ChessMoveSortList movesRoot;

	// moves excluded by the search
	ChessMoveList excludedMoves;

	// threads used by the search
	ChessSearchThreadList threads;

	// opening book
	ChessOpeningBook openingBook;

	// timer
	Timer timerSearch;

	// engine playing side
	Side engineSide {white};

	// multipv number of moves (1 = disable)
	int multipv {1};

	// search time by moves
	std::uint64_t ideepTimeByMove {0};

	// max depth to search
	int ideepMaxDepth {max_depth - 1};

	// number of nodes between time checks
	std::uint64_t ideepNodeCheckTime {check_time_interval};

	// node count for time checks
	std::uint64_t ideepNodeCountTime {0};

	// how many nodes before sleep to reduce speed
	// 0 = disable
	std::uint64_t reduceSpeedNodes {0};

	// limit number of nodes to reduce strength
	std::uint64_t eloNodesLimit {0};

	ELOLimitMode eloLimitMode {ELOLimitMode::speed};

	std::uint64_t fixedNodesValue {0};

	std::uint64_t fixedTimeValue {0};

	int searchMate {0};

	// remaining moves (for tournament mode, if used)
	int remainingmoves {30};

	// time increment for each side (should be the same ?)
	std::array<int, nb_sides> timeIncrement {};

	// number of moves of the game out of book moves
	int movesOutOfBook {0};

	// max depth for current search
	int thinkDepth {0};

	// SMP algorithm
	SMPMode smpmode {SMPMode::ybwc};

	// minimum split depth for YBWC
	int minSplitDepth {4 * one_ply};

	// time for white & black (ms)
	std::array<int, nb_sides> timeGame {};

	// pondering is in use for the current search
	std::atomic_bool pondering {false};

	bool infiniteSearch {false};

	// enable null moves prunning
	bool useNullMoves {true};

	// enable Late moves reduction
	bool useLMR {true};

	// set when running testsuites
	bool runtestsuite {false};

	// use opening book
	bool useOpeningBook {true};

	int strengthELO {max_strength_elo};

	bool useCustomStrength {false};

	int TBProbeDepth {1};

	int TBProbeLimit {7};

	int TBCardinality {7};

	bool TB50MoveRule {true};

	bool allowEndSearchNodes {false};

public:

	ChessEngine() = default;

	~ChessEngine() override = default;

	// precalc tables for bitboards, search, ...
	// must be called once
	static void precalcTables();

	void init(int hashsize, int nt = 1);

	void free();

	ChessBoard &getBoardRef();

	ChessEval &getEval();

	ChessHashTable &getHashTable();

	ChessRepetitions &getRepetitions();

	ChessMoveList &getExcludedMoves();

	ChessMoveSortList &getMovesRoot();

	void setSearchExcludedMoves(const ChessMoveList &moves);

	void setSearchSearchedMoves(const ChessMoveList &moves);

	void setSearchSearchedMovesFromString(const std::string_view &str);

	void clearSearchExcludedMoves();

	ChessSearchThreadList &getThreads();

	[[nodiscard]]
	bool mustCheckNodes() const;

	[[nodiscard]]
	bool isRunningTestSuite() const;

	void setModeRunTestSuite(bool b);

	[[nodiscard]]
	bool usingLMR() const;

	[[nodiscard]]
	bool usingNullMoves() const;

	[[nodiscard]]
	SMPMode getSMPMode() const;

	// set algorithm for SMP (use it before creating threads)
	void setSMPMode(SMPMode m);

	// start a new game
	// note : not clearing hash table
	void newGame();

	void setRemainingMoves(int r);

	void setTimeIncrement(Side side, int increment);

	void enableNullMoves(bool b);

	void enableLMR(bool b);

	void setMultiPV(int n);

	[[nodiscard]]
	bool isPondering() const;

	// return if custom strength is used
	[[nodiscard]]
	bool usingCustomStrength() const;

	// return current ELO strength
	[[nodiscard]]
	int getStrengthELO() const;

	// set custom ELO strength
	void setStrengthELO(int e);

	// set custom Level instead of using ELO (+100 ELO by level)
	void setStrengthLevel(int l);

	// get current search time
	[[nodiscard]]
	auto getSearchTime() const;

	// return the number of legal moves on current position
	[[nodiscard]]
	int countLegalMoves(ChessBoard &pos) const;

	// test if the player is mate (slow)
	[[nodiscard]]
	bool isMate(ChessBoard &pos) const;

	// return draw value for current player
	[[nodiscard]]
	int getDrawValue(const ChessBoard &pos) const;

	// set contempt factor, returned as draw value
	void setDrawValue(int v);

	// init time management and stop conditions
	void initSearch();

	// start a new search for current position
	void startSearch(bool ponder);

	void waitEndSearch();

	// calculate mate value
	[[nodiscard]]
	static int calcMateValue(int value);

	// extract pv from hash
	void extractPV(int depth);

	// test if the engine must end current search
	[[nodiscard]]
	bool mustStopSearch() const;

	// when start seearching a move at root
	void searchingRootMove(int depth, ChessMove &mv, int num);

	// evaluate current position
	[[nodiscard]]
	int evaluate(int alpha, int beta);

	// clear the main hash table
	void clearHashTable(int nt = 1);

	void setHashTableSize(int sz, int nt = 1);

	void reduceSpeed();

	[[nodiscard]]
	bool isReducingSpeed() const;

	// get current search nodes count
	// smp : for all threads
	[[nodiscard]]
	std::uint64_t getSearchNodeCount();

	[[nodiscard]]
	std::uint64_t getSearchElapsedTime() const;

	// find if the current search must end
	bool checkSearchTime();

	// get current playing side
	[[nodiscard]]
	Side side() const;

	void errorMoveFromGUI(const std::string &text);

	// set move with no piece information (used by UCI engines)
	// ex: "e2e4"
	[[nodiscard]]
	bool playMovesFromString(const std::string_view &text);

	// play a move (must be legal)
	bool playMove(ChessMove move);

	// take back one move
	bool takeBack();

	// init board with start position
	void setStartPosition();

	// init board with FEN position
	[[nodiscard]]
	bool setFENPosition(const std::string_view &str);

	// get FEN position from board
	[[nodiscard]]
	std::string getFENPosition(ChessBoard &pos) const;

	void setSearchFixedDepth(int depth);

	void setSearchFixedNodes(std::uint64_t nodes);

	void setSearchFixedTime(int t);

	void setInfiniteSearch(bool s);

	std::uint64_t getSearchTimeByMove() const;

	std::uint64_t getSearchFixedTime() const;

	int getSearchMateValue() const;

	void setSearchMateValue(int v);

	void setMaxThreads(int m);

	void setMinSplitDepth(int d);

	[[nodiscard]]
	int getMinSplitDepth() const;

	[[nodiscard]]
	std::uint64_t getReduceSpeedNodes() const;

	void setPersonality(const std::string &fname);

	[[nodiscard]]
	bool isSearchInfinite() const;

	// return true if the engine is searching
	[[nodiscard]]
	bool isThinking();

	// force the current search to stop
	void stopThinking();

	// the opponent played the expected move,
	// continue thinking in normal mode
	void ponderHit();

	// test nodes searched
	// must be called in the move loop for exact node count
	[[nodiscard]]
	bool checkNodes();

	void setELOLimitMode(ELOLimitMode m);

	// get number of moves in current game
	[[nodiscard]]
	int getGameMoveCount() const;

	// get move in current game
	[[nodiscard]]
	ChessMove getGameMove(int n) const;

	[[nodiscard]]
	ChessMove getGameLastMove() const;

	// load opening book
	[[nodiscard]]
	bool loadOpeningBook(const std::string &filebook);

	// unload opening book
	void unloadOpeningBook();

	// show available opening book moves for current position
	void showBookMoves();

	static bool isPositionQuiet(ChessBoard &pos);

	// get current time
	[[nodiscard]]
	int getTime(Side p) const;

	[[nodiscard]]
	std::string getTextMove(const ChessMove &move) const;

	[[nodiscard]]
	std::string getTextPV(const ChessPVMoveList &mlist, int count) const;

	void setPlayerTime(Side p, int tm);

	void forceCheckTime();

	[[nodiscard]]
	int getTBProbeDepth() const;

	void setTBProbeDepth(int d);

	[[nodiscard]]
	int getTBProbeLimit() const;

	void setTBProbeLimit(int d);

	[[nodiscard]]
	int getTBCardinality() const;

	[[nodiscard]]
	bool getTB50MoveRule() const;

	void setTB50MoveRule(bool b);

};

////////////////////////////////////////////////////////////////////////////////
inline ChessBoard &ChessEngine::getBoardRef()
{
	return board;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessEval &ChessEngine::getEval()
{
	return eval;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessHashTable &ChessEngine::getHashTable()
{
	return hashTable;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::isPondering() const
{
	return pondering;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::waitEndSearch()
{
	threads.waitEndSearch();
}

////////////////////////////////////////////////////////////////////////////////
inline ChessRepetitions &ChessEngine::getRepetitions()
{
	return repetition;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMoveList &ChessEngine::getExcludedMoves()
{
	return excludedMoves;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::clearSearchExcludedMoves()
{
	excludedMoves.clear();
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMoveSortList &ChessEngine::getMovesRoot()
{
	return movesRoot;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessSearchThreadList &ChessEngine::getThreads()
{
	return threads;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::mustCheckNodes() const
{
	return ((fixedNodesValue != 0) || (eloNodesLimit != 0));
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::isRunningTestSuite() const
{
	return runtestsuite;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setModeRunTestSuite(bool b)
{
	runtestsuite = b;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::usingLMR() const
{
	return useLMR;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::usingNullMoves() const
{
	return useNullMoves;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setRemainingMoves(int r)
{
	remainingmoves = r;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setTimeIncrement(Side side, int increment)
{
	timeIncrement[side] = increment;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setSearchFixedDepth(int depth)
{
	if (depth == 0) {
		thinkDepth = max_depth - 1;
	} else {
		thinkDepth = std::clamp(depth, 0, max_depth - 1);
	}
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setSearchFixedNodes(std::uint64_t nodes)
{
	fixedNodesValue = std::max(static_cast<std::uint64_t>(0), nodes);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setSearchFixedTime(int t)
{
	fixedTimeValue = static_cast<std::uint64_t>(std::max(0, t));
}

////////////////////////////////////////////////////////////////////////////////
inline std::uint64_t ChessEngine::getSearchTimeByMove() const
{
	return ideepTimeByMove;
}

////////////////////////////////////////////////////////////////////////////////
inline std::uint64_t ChessEngine::getSearchFixedTime() const
{
	return fixedTimeValue;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getSearchMateValue() const
{
	return searchMate;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setSearchMateValue(int v)
{
	searchMate = v;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::enableNullMoves(bool b)
{
	useNullMoves = b;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::enableLMR(bool b)
{
	useLMR = b;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setMultiPV(int n)
{
	multipv = std::clamp(n, 1, max_multipv);
	threads.initMultiPV(multipv);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::usingCustomStrength() const
{
	return useCustomStrength;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getStrengthELO() const
{
	return strengthELO;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getDrawValue(const ChessBoard &pos) const
{
	return (pos.side() == engineSide) ? -eval.getDrawValue() :
		eval.getDrawValue();
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setDrawValue(int v)
{
	eval.setDrawValue(v);
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::evaluate(int alpha, int beta)
{
	return eval.evaluate(board, threads.getMainThread()->hashTablePawn, alpha,
		beta);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::clearHashTable(int nt)
{
	hashTable.clear(nt);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setHashTableSize(int sz, int nt)
{
	hashTable.init(sz, nt);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::isReducingSpeed() const
{
	return (reduceSpeedNodes != 0);
}

////////////////////////////////////////////////////////////////////////////////
inline std::uint64_t ChessEngine::getSearchNodeCount()
{
	return threads.getNodeCount();
}

////////////////////////////////////////////////////////////////////////////////
inline std::uint64_t ChessEngine::getSearchElapsedTime() const
{
	return timerSearch.getElapsedTime();
}

////////////////////////////////////////////////////////////////////////////////
inline Side ChessEngine::side() const
{
	return board.side();
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setMaxThreads(int m)
{
	threads.setMaxThreads(m, *this);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setMinSplitDepth(int d)
{
	minSplitDepth = std::clamp(d, 1, max_depth) * one_ply;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getMinSplitDepth() const
{
	return minSplitDepth;
}

////////////////////////////////////////////////////////////////////////////////
inline std::uint64_t ChessEngine::getReduceSpeedNodes() const
{
	return reduceSpeedNodes;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::isThinking()
{
	return threads.isThinking();
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::isSearchInfinite() const
{
	return infiniteSearch;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setInfiniteSearch(bool s)
{
	infiniteSearch = s;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setELOLimitMode(ELOLimitMode m)
{
	eloLimitMode = m;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getGameMoveCount() const
{
	return static_cast<int>(gameMoves.size());
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove ChessEngine::getGameMove(int n) const
{
	return (n < getGameMoveCount()) ? gameMoves[n] : ChessMove(0);
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove ChessEngine::getGameLastMove() const
{
	return gameMoves.back();
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getTime(Side p) const
{
	return timeGame[p];
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setPlayerTime(Side p, int tm)
{
	timeGame[p] = tm;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::isMate(ChessBoard &pos) const
{
	return (pos.inCheck() &&
		    (countLegalMoves(pos) == 0));
}

////////////////////////////////////////////////////////////////////////////////
inline SMPMode ChessEngine::getSMPMode() const
{
	return smpmode;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::forceCheckTime()
{
	ideepNodeCountTime = ideepNodeCheckTime;
}

////////////////////////////////////////////////////////////////////////////////
inline auto ChessEngine::getSearchTime() const
{
	return timerSearch.getElapsedTime();
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getTBProbeDepth() const
{
	return TBProbeDepth;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setTBProbeDepth(int d)
{
	TBProbeDepth = d;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getTBProbeLimit() const
{
	return TBProbeLimit;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setTBProbeLimit(int d)
{
	TBProbeLimit = d;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEngine::getTBCardinality() const
{
	return TBCardinality;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessEngine::getTB50MoveRule() const
{
	return TB50MoveRule;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEngine::setTB50MoveRule(bool b)
{
	TB50MoveRule = b;
}

} // namespace Cheese

#endif //CHEESE_ENGINE_H_
