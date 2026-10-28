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

#ifndef CHEESE_SEARCH_H_
#define CHEESE_SEARCH_H_

#include "config.h"

#include "board.h"
#include "eval.h"
#include "history.h"
#include "moveorder.h"
#include "pv.h"
#include "repetitions.h"
#include "searchthread.h"

#include <array>

namespace Cheese {

// use null moves pruning
inline constexpr bool use_nullmoves = true;

// null move pruning verification
inline constexpr bool use_nullmoves_verification = false;

// use Razoring
inline constexpr bool use_razoring = true;

// reverse futility prunning, or static null moves pruning
inline constexpr bool use_reverse_futility = true;

// use Internal Iterative Deepening
inline constexpr bool use_internal_iterative_deepening = true;

// extend search when in check
inline constexpr bool use_extension_check = true;

// extend search when a pawn is on 7th rank
inline constexpr bool use_extension_pawn_on_7th = true;

// used only in root
inline constexpr bool use_extension_single_response = true;

// use late move reduction (LMR)
inline constexpr bool use_late_move_reduction = true;

// Late move pruning (LMP)
inline constexpr bool use_late_move_pruning = true;

// use Futility Pruning
inline constexpr bool use_futility_pruning = true;

// "delta pruning" = futility pruning in quiescence search
inline constexpr bool use_delta_pruning = false;

// test if PV moves are valid
inline constexpr bool use_test_pv = false;

// maximum depth the engine can search
inline constexpr int max_depth = 128;

// fractional ply resolution
// note : fractional ply is not used, now all extensions are 1 ply
inline constexpr int one_ply = 2;

// depth for checks in quiescence search
// -1 => 1 ply , -2 => 2 ply ...
inline constexpr int depth_checks_quiescence = -1;

inline constexpr int extend_check = one_ply * 1;

inline constexpr int extend_pawnon7th = one_ply * 1;

// Late Move Reduction : minimum depth
inline constexpr int lmr_min_depth = 3;

inline constexpr int lmr_max_depth = 64;

inline constexpr int lmr_max_moves = 64;

inline constexpr int lmp_max_depth = 16;

// maximum search depth
inline constexpr int max_depth_search = 128;

////////////////////////////////////////////////////////////////////////////////
enum class NodeType {
	non_pv,
	pv
};

////////////////////////////////////////////////////////////////////////////////
// search stack, used by search to access previous ply information
// note : movelist and searchedMoves are declared here to reduce stack memory 
// usage and because it's slow to declare them in search function with 
// ChessMove constructor.
struct ChessSearchStackEntry {

	// current move in search
	ChessMove move;

	// move list used by move generation and ordering in search
	ChessMoveSortList movelist;

	// quiet moves searched
	std::array<ChessMove, max_history_quiets> searchedMoves;

};

using ChessSearchStack = std::array<ChessSearchStackEntry, max_depth_search>;

////////////////////////////////////////////////////////////////////////////////
// search repetitions, search stack, movelist, and pv, allocated in thread
// to reduce stack memory usage
struct ChessSearchData {

	// repetition table
	ChessRepetitions repetition;

	// searched moves stack
	ChessSearchStack searchStack;

	// principal variation
	ChessPV pv;
};

class ChessEngine;

////////////////////////////////////////////////////////////////////////////////
// search functions
class ChessSearch {

	friend class ChessSearchYBWC;

	// pointer to chess engine
	ChessEngine &engine;

	// reference to thread list
	ChessSearchThreadList &threads;

	// reference to engine evaluation
	ChessEval &eval;

	// reference to engine main hashtable
	ChessHashTable &hashTable;

	// thread associated with this search
	ChessSearchThread *thread;

	// chess board current position in search
	ChessBoard board;

	// repetition table
	ChessRepetitions &repetition;

	// searched moves stack
	ChessSearchStack &searchStack;

	// principal variation
	ChessPV &pv;

	// if we need to test nodes in search move loop
	// (only in main thread)
	bool needCheckNodes;

	// lmr reduction [depth][movecount]
	static const std::array<std::array<std::uint32_t, lmr_max_depth>, 
		lmr_max_moves> lmr_reduction;

	// lmp limit [depth]
	static const std::array<int, lmp_max_depth> lmp_count;

public:

	ChessSearch(ChessSearchData &data, ChessEngine &e, 
		ChessSearchThreadList &ths,	ChessEval &ev, ChessHashTable &ht, 
		ChessSearchThread *th, const ChessBoard &pos, bool chknodes);

	void clearPV(int ply);

	ChessPV &getPV();

	// note : only used by move order at root when using quiescence
	ChessRepetitions &getRepetitions();

	// evaluate current position
	[[nodiscard]]
	int evaluate(int alpha, int beta);

	// start iterative deepening search (main thread)
	void startSearch();

	// alpha-beta search at root
	[[nodiscard]]
	int searchRoot(int alpha, int beta, int depth, ChessMove topmove, 
		bool sendRootMove);

	// alpha-beta search
	template<NodeType nodetype>
	[[nodiscard]]
	int search(int alpha, int beta, int depth, int ply, bool incheck);

	// alpha-beta quiescence search
	template<NodeType NodeType>
	[[nodiscard]]
	int searchQuiescent(int alpha, int beta, int depth, int ply, bool incheck);

	void testPV(const ChessPVMoveList &moves, int sz);

	[[nodiscard]]
	static int getLMRReduction(int depth, int num);
};

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearch::clearPV(int ply)
{
	pv.clear(ply);
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessSearch::evaluate(int alpha, int beta)
{
	return eval.evaluate(board, thread->hashTablePawn, alpha, beta);
}

////////////////////////////////////////////////////////////////////////////////
inline ChessPV &ChessSearch::getPV()
{
	return pv;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessRepetitions &ChessSearch::getRepetitions()
{
	return repetition;
}

} // namespace Cheese

#endif // CHEESE_SEARCH_H_
