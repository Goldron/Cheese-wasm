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

#ifndef CHEESE_YBWC_H_
#define CHEESE_YBWC_H_

#include "config.h"

#include "search.h"
#include "searchthread.h"

#include <array>
#include <mutex>
#include <atomic>

namespace Cheese {

// Young Brother Wait Concept (YBWC)
// inspired by Stockfish implementation

class ChessMoveOrder;
class ChessRepetitions;
class ChessBoard;
class ChessPV;
class ChessEngine;
class ChessHashTable;

// maximum of split points by threads
inline constexpr int max_split_by_thread = 8;

////////////////////////////////////////////////////////////////////////////////
// YBWC split point : a position in the search where multiple threads
// are working
struct ChessSearchSplitPoint {

	// mutex to lock modification of splitpoint data
	std::mutex mutex;

	ChessSearchSplitPoint *parentSplitPoint;

	// the thread that created this splitpoint
	ChessSearchThread *parentThread;

	ChessBoard *board;

	ChessMoveOrder *moveorder;

	ChessRepetitions *repetition;

	ChessSearchStack *searchStack;

	ChessPV *pv;

	int depth;

	int ply;

	int fmargin;

	int alpha;

	int beta;

	bool nodepv;

	bool incheck;

	bool rootnode;

	bool fpruning;

	std::atomic_bool cutoff;

	// current move number
	std::atomic_int numMove;

	// best move
	std::atomic_uint32_t bestMove;

	// best value
	std::atomic_int bestValue;

	// threads working at this splitpoint
	// 1 bit / thread, limit the number of thread to 64
	std::atomic_uint64_t childMask;

	// count children threads working at this splitpoint
	std::atomic_int countChildren;

};

////////////////////////////////////////////////////////////////////////////////
class ChessSearchYBWC final : public ChessSearch {

public:

	ChessSearchYBWC(ChessSearchData &data, ChessEngine &e, 
		ChessSearchThreadList &ths,	ChessEval &ev, ChessHashTable &ht, 
		ChessSearchThread *th, const ChessBoard &pos, bool chknodes);

	// a thread start the search at a splitpoint
	template<NodeType nodetype>
	void searchSMP(int alpha, int beta, int depth, int ply, bool incheck);

	// a thread start the search at a splitpoint, at Root
	void searchSMPRoot(int alpha, int beta, int depth);

	void copySearchStack(ChessSearchStack &lmv, int ply);

	// split the search, called by ChessSearch
	[[nodiscard]]
	static bool splitSearch(ChessSearch *search, ChessMoveOrder *moveorder,
		int alpha, int beta, int depth, int ply, int nummove, bool nodepv,
		bool incheck, bool rootnode, bool fpruning, int fmargin, int *bestvalue,
		ChessMove *bestmove);
};

////////////////////////////////////////////////////////////////////////////////
class ChessSearchThreadYBWC final : public ChessSearchThread {

	friend class ChessSearchYBWC;

	// splitpoint where the thread is searching now
	ChessSearchSplitPoint *splitPoint {nullptr};

	// number of splitpoint created by this thread
	std::atomic_int countSplit {0};

	// count used search data
	int searchId {0};

	// split points created by this thread
	std::array<ChessSearchSplitPoint, max_split_by_thread> listSplitPoint;

	std::array<ChessSearchData, max_split_by_thread + 1> listSearchData;

public:

	ChessSearchThreadYBWC(int id, ChessSearchThreadList &th, ChessEngine &e);

	~ChessSearchThreadYBWC() override = default;

	// thread main function, called by thread
	void func() override;

	// code for main thread
	void runMain();

	// code for child thread
	void runChild(ChessSearchSplitPoint *split);

	// test if we have not reached the maximum splitpoint for this thread
	[[nodiscard]]
	bool canSplit() const override;

	// if a cutoff happens in splitpoint or parent splitpoints
	[[nodiscard]]
	bool cutoff() const override;

	// test if the thread is available to help another thread
	[[nodiscard]]
	bool isAvailable(ChessSearchThread *mainthread) override;
};

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchYBWC::copySearchStack(ChessSearchStack &lmv,
	int ply)
{
	// need only last ply
	if (ply > 0) {
		searchStack[ply - 1].move = lmv[ply - 1].move;
	}
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThreadYBWC::canSplit() const
{
	return (countSplit < max_split_by_thread);
}

} // namespace Cheese

#endif //CHEESE_YBWC_H_
