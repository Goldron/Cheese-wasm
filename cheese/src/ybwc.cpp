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

#include "ybwc.h"

#include "search.h"
#include "moveorder.h"
#include "repetitions.h"
#include "engine.h"

#include <algorithm>
#include <cassert>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
ChessSearchYBWC::ChessSearchYBWC(ChessSearchData &data, ChessEngine &e, ChessSearchThreadList &ths,
	ChessEval &ev, ChessHashTable &ht, ChessSearchThread *th, const
	ChessBoard &pos, bool chknodes) :
	ChessSearch(data, e, ths, ev, ht,th, pos, chknodes)
{
}

////////////////////////////////////////////////////////////////////////////////
// a thread start the search at a splitpoint, at Root
void ChessSearchYBWC::searchSMPRoot(int alpha, int beta, int depth)
{
	assert(thread->smp == SMPMode::ybwc);
	auto *th = static_cast<ChessSearchThreadYBWC *>(thread);
	ChessSearchSplitPoint *sp = th->splitPoint;
	ChessMoveOrder &moveorder = *sp->moveorder;

	int	ply = 0;

	alpha = sp->bestValue;

	// legal moves count
	int nm = sp->numMove;

	clearPV(ply);

	ChessMove move;

	while ((move = moveorder.getNextMove(board)) != ChessMove(0)) {

		// exclude moves
		if (thread->isExcludedMove(move)) {
			continue;
		}

		// note : legality test already done in orderRoot
		const ChessMoveRec rec = board.makeMove(move);

		if (!board.isPositionLegal()) {
			board.unMakeMove(move, rec);
			continue;
		}

		searchStack[ply].move = move;

		++thread->countNodes;

		repetition.push(board.getHashKey());

		// remember current move num to send to gui
		int cnum = nm;

		++sp->numMove;

		if (sp->bestValue > alpha) {
			alpha = sp->bestValue;
		}

		// send currmove to GUI
		engine.onSearchRootMove(depth, move, cnum + 1);

		sp->mutex.unlock();

		bool chk = board.inCheck();

		int ext = 0;

		if constexpr (use_extension_check) {
			if (chk) {
				ext += extend_check;
			}
		}

		if constexpr (use_extension_pawn_on_7th) {
			if (move.isMovePawnOn7th()) {
				ext += extend_pawnon7th;
			}
		}

		// limit extension to 1 ply
		ext = std::min(ext, (one_ply * 1));

		int newdepth = depth + ext - one_ply;

		int	value = 0;

		// Principal Variation Search (PVS)

		if (sp->bestValue > alpha) {
			alpha = sp->bestValue;
		}

		// PVS search
		value = -search<NodeType::non_pv>(-alpha - 1, -alpha, newdepth, 
			ply + 1, chk);

		// pvs failed : research
		if ((value > alpha) && (value < beta)) {

			if (sp->bestValue > alpha) {
				alpha = sp->bestValue;
			}

			// PVS re-search
			value = -search<NodeType::pv>(-beta, -alpha, newdepth, ply + 1, 
				chk);
		}

		board.unMakeMove(move, rec);

		repetition.pop();

		sp->mutex.lock();

		if ((threads.mustStop()) || (thread->cutoff())) {
			return;
		}

		// get alpha & bestmove modified by other threads
		if (sp->bestValue > alpha) {
			alpha = sp->bestValue;
		}

		// num may change between the unlock / lock
		nm = sp->numMove;

		if (value > alpha) {

			sp->bestMove = move.data();

			// update pv
			sp->pv->insertPV(ply, move, pv);

			if (value >= beta) {
				sp->bestValue = value;
				sp->cutoff = true;
				return;
			}

			alpha = value;
			sp->bestValue = alpha;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// a thread start the search at a splitpoint
template<NodeType nodetype>
void ChessSearchYBWC::searchSMP(int alpha, int beta, int depth, int ply,
	bool incheck)
{
	static constexpr bool nodepv = (nodetype == NodeType::pv);

	assert(alpha < beta);
	assert((nodepv) || (alpha == beta - 1));
	assert(depth < max_depth * one_ply);
	assert(alpha >= -mate_value - 1);
	assert(beta <= mate_value + 1);
	assert(incheck == board.inCheck());

	assert(thread->smp == SMPMode::ybwc);
	auto *th = static_cast<ChessSearchThreadYBWC *>(thread);
	ChessSearchSplitPoint *sp = th->splitPoint;
	ChessMoveOrder &moveorder = *sp->moveorder;

	// real depth
	int rdepth = depth / one_ply;

	if constexpr (nodepv) {
		clearPV(ply);
	}

	// init futility pruning
	bool futilityPruning = sp->fpruning;
	int futilityMargin = sp->fmargin;

	bool condLMR = ((depth >= (lmr_min_depth * one_ply))
					&& (!incheck)
					&& (engine.usingLMR()));

	alpha = sp->bestValue;

	int num = sp->numMove;

	const HashKey hk = board.getHashKey();

	ChessMove move;

	int value = 0;

	// TODO : fix data race : child threads are using the main thread 
	// moveorder.getNextMove() here under lock, but it's using history to 
	// sort moves, while the main thread change history values in search()
	while ((move = moveorder.getNextMove(board)) != ChessMove(0)) {

		const auto mphase = moveorder.getPhase();

		const ChessMoveRec rec = board.makeMove(move);
		if (!board.isPositionLegal()) {
			board.unMakeMove(move, rec);
			continue;
		}

		searchStack[ply].move = move;

		++thread->countNodes;

		int cnum = num;

		num = ++sp->numMove;

		if (sp->bestValue > alpha) {
			alpha = sp->bestValue;
		}

		sp->mutex.unlock();

		bool chk = board.inCheck();

		// search extensions
		int ext = 0;

		if constexpr (use_extension_check) {
			if (chk) {
				ext += extend_check;
			}
		}

		if constexpr (use_extension_pawn_on_7th) {
			if (move.isMovePawnOn7th()) {
				ext += extend_pawnon7th;
			}
		}

		// limit extension to 1 ply
		ext = std::min(ext, (one_ply * 1));

		int newdepth = depth + ext - one_ply;

		bool pruning = false;

		// Futility Prunning
		if constexpr (use_futility_pruning) {
			if (futilityPruning) {
				if ((cnum > 0)
					&& (!chk)
					&& (ext == 0)
					&& (!move.isMovePawnPast4th(~board.side()))
					&& (!move.isCaptureOrPromotion())
					) {

					if (futilityMargin <= alpha) {
						pruning = true;
					}
				}
			}
		}

		// late moves pruning (LMP)
		if constexpr (use_late_move_pruning && !nodepv) {
			if ((mphase == MoveOrderPhase::next_quiet)
				&& (!incheck)
				&& (!chk)
				&& (ext == 0)
				&& (!move.isCastle())
				&& (!move.isMovePawnPast4th(~board.side()))
				&& (depth < lmp_max_depth * one_ply)
			) {
				if (cnum > lmp_count[rdepth]) {
					pruning = true;
				}
			}
		}

		if (pruning) {
			board.unMakeMove(move, rec);
			sp->mutex.lock();
			continue;
		}

		repetition.push(hk);

		// Principal Variation Search (PVS)

		if (sp->bestValue > alpha) {
			alpha = sp->bestValue;
		}

		// Late Move Reduction (LMR)
		value = alpha + 1;
		if constexpr (use_late_move_reduction) {
			if ((condLMR)
				&& (cnum > 1)
				&& (!chk)
				&& (mphase == MoveOrderPhase::next_quiet)
				&& (!move.isCastle())
				&& (ext == 0)) {

				int lr = getLMRReduction(rdepth, num);
				if constexpr (!nodepv) {
					lr++;
				}

				if (lr) {
					// LMR search
					value = -search<NodeType::non_pv>(-alpha - 1, -alpha, 
						newdepth - lr * one_ply, ply + 1, false);
				}	
			}
		}

		if (value > alpha) {

			// PVS search
			value = -search<NodeType::non_pv>(-alpha - 1, -alpha, newdepth, 
				ply + 1, chk);

			if ((value > alpha) && (value < beta)) {

				if ((sp->bestValue > alpha) && (sp->bestValue < beta)) {
					alpha = sp->bestValue;
				}

				// PVS re-search
				value = -search<NodeType::pv>(-beta, -alpha, newdepth, ply + 1, 
					chk);
			}
		}

		board.unMakeMove(move, rec);

		repetition.pop();

		sp->mutex.lock();

		if ((threads.mustStop()) || (thread->cutoff())) {
			return;
		}

		// get alpha modified by other threads
		if (sp->bestValue > alpha) {
			alpha = sp->bestValue;
		}

		// smp : num may change between the unlock / lock
		num = sp->numMove;

		if (value > alpha) {

			sp->bestMove = move.data();

			if (value >= beta) {
				sp->bestValue = value;
				sp->cutoff = true;
				return;
			}

			// update pv
			if constexpr (nodepv) {
				sp->pv->insertPV(ply, move, pv);
			}

			alpha = value;
			sp->bestValue = alpha;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
bool ChessSearchYBWC::splitSearch(ChessSearch *search, 
	ChessMoveOrder *moveorder, int alpha, int beta, int depth, int ply, 
	int nummove, bool nodepv, bool incheck, bool rootnode, bool fpruning, 
	int fmargin, int *bestvalue, ChessMove *bestmove)
{
	assert(search->thread->smp == SMPMode::ybwc);
	auto *threadybwc = static_cast<ChessSearchThreadYBWC *>(search->thread);
	auto &threads = search->threads;

	// create a splitpoint
	ChessSearchSplitPoint *sp =
		&threadybwc->listSplitPoint[threadybwc->countSplit];

	sp->parentSplitPoint = threadybwc->splitPoint;
	sp->parentThread = threadybwc;
	sp->board = &search->board;
	sp->moveorder = moveorder;
	sp->repetition = &search->repetition;
	sp->searchStack = &search->searchStack;
	sp->pv = &search->pv;
	sp->depth = depth;
	sp->ply = ply;
	sp->fmargin = fmargin;
	sp->alpha = alpha;
	sp->beta = beta;
	sp->nodepv = nodepv;
	sp->incheck = incheck;
	sp->rootnode = rootnode;
	sp->fpruning = fpruning;
	sp->cutoff = false;
	sp->numMove = nummove;
	sp->bestMove = bestmove->data();
	sp->bestValue = *bestvalue;
	sp->childMask = 1ULL << threadybwc->getSearchId();
	sp->countChildren = 1;

	threads.lock();
	sp->mutex.lock();

	// we also search at this splitpoint
	threadybwc->splitPoint = sp;
	++threadybwc->countSplit;

	// if threads are available, associate splitpoint and start searching
	ChessSearchThreadYBWC *th = nullptr;
	while ((th = static_cast<ChessSearchThreadYBWC *>(
			threads.findAvailableThread(threadybwc))) != nullptr) {
		sp->childMask |= (1ULL << th->getSearchId());
		++sp->countChildren;
		th->splitPoint = sp;
		th->searching = true;
		th->notify();
	}

	sp->mutex.unlock();
	threads.unlock();

	// start searching
	threadybwc->runChild(sp);

	threads.lock();
	sp->mutex.lock();

	threadybwc->searching = true;

	// remove splitpoint
	--threadybwc->countSplit;
	threadybwc->splitPoint = sp->parentSplitPoint;

	*bestvalue = sp->bestValue;
	bestmove->set(sp->bestMove);

	sp->mutex.unlock();
	threads.unlock();

	// return to the search that called split, and all moves was searched
	return true;
}

////////////////////////////////////////////////////////////////////////////////
ChessSearchThreadYBWC::ChessSearchThreadYBWC(int id, ChessSearchThreadList &th,
	ChessEngine &e) : ChessSearchThread(id, SMPMode::ybwc, th, e)
{
	hashTablePawn.clear();
}

////////////////////////////////////////////////////////////////////////////////
bool ChessSearchThreadYBWC::cutoff() const
{
	ChessSearchSplitPoint *sp = splitPoint;
	while (sp != nullptr) {
		if (sp->cutoff) {
			return true;
		}
		sp = sp->parentSplitPoint;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessSearchThreadYBWC::isAvailable(ChessSearchThread *mainthread)
{
	// the thread is already searching
	if (searching) {
		return false;
	}

	const int count = countSplit;

	// we have no splitpoints, we wan help other threads
	if (count == 0) {
		return true;
	}

	// the helpful parent concept :
	// we must only help the threads working on the last splitpoint
	// we created because we want them to finish as fast as possible
	return ((listSplitPoint[count - 1].childMask &
			 (1ULL << mainthread->getSearchId())) != 0ULL);
}

////////////////////////////////////////////////////////////////////////////////
// thread main function
void ChessSearchThreadYBWC::func()
{
	if (isMainThread()) {
		runMain();
	} else {
		runChild(nullptr);
	}
}

////////////////////////////////////////////////////////////////////////////////
// split != 0 when this function is called manually
void ChessSearchThreadYBWC::runMain()
{	
	while (!terminate) {

		std::unique_lock<std::mutex> lk(mutex);
		thinking = false;

		threads.notify();
		cond.wait(lk, [&]{ return thinking || terminate; });
		lk.unlock();

		if (!terminate) {

			searching = true;

			threads.resetNodeCount();
			threads.clearHistory();

			for (auto *th : threads) {
				if (!th->isMainThread()) {
					th->excludedMoves = excludedMoves;
				}
			}

			listSearchData[searchId].repetition = engine.getRepetitions();

			ChessSearch searchMain(listSearchData[searchId], engine, 
				threads, engine.getEval(), engine.getHashTable(), this, 
				engine.getBoardRef(), engine.mustCheckNodes());

			++searchId;

			// start iterative deepening search
			searchMain.startSearch();

			--searchId;

			// send result
			engine.onEndSearch(bestMove, ponderMove, searchDepth, 
				threads.getMaxPly(), threads.getNodeCount(), 
				engine.getSearchTime());

			searching = false;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// split != 0 when this function is called manually
void ChessSearchThreadYBWC::runChild(ChessSearchSplitPoint *split)
{
	while (!terminate) {

		while (searching) {

			threads.lock();
			ChessSearchSplitPoint *sp = splitPoint;
			threads.unlock();

			listSearchData[searchId].repetition = *sp->repetition;

			// create a new search for this split
			ChessSearchYBWC search(listSearchData[searchId], engine, threads, 
				engine.getEval(), engine.getHashTable(), this, *(sp->board), 
				false);

			++searchId;

			search.copySearchStack(*(sp->searchStack), sp->ply);

			sp->mutex.lock();

			// start searching at the splitpoint
			if (sp->rootnode) {
				search.searchSMPRoot(sp->alpha, sp->beta, sp->depth);
			} else {
				if (sp->nodepv) {
					search.searchSMP<NodeType::pv>(sp->alpha, sp->beta, 
						sp->depth, sp->ply, sp->incheck);
				} else {
					search.searchSMP<NodeType::non_pv>(sp->alpha, sp->beta, 
						sp->depth, sp->ply, sp->incheck);
				}
			}

			searching = false;

			sp->childMask &= ~(1ULL << stid);
			--sp->countChildren;

			// if the main thread finished before the children
			// and all children have finish, wake up the parent thread
			if ((sp->parentThread != this) &&
				(sp->childMask == 0ULL)) {
				sp->parentThread->notify();
			}

			sp->mutex.unlock();

			--searchId;

			// TODO : "late join"
			// after search, try to find another available splitpoint to help

		}

		std::unique_lock<std::mutex> lk(mutex);

		// if we created the splitpoint and all children have finished, return
		// (will return to splitsearch to end the split)
		// else wait until another thread need our help
		if ((split != nullptr) &&
			(split->parentThread == this) &&
			(split->childMask == 0ULL)) {
			// break will release the mutex and free unique_lock
			break;
		}

		// wait until a thread need our help at splitpoint
		// or when the function is called manually
		cond.wait(lk, [&]{
			if ((split != nullptr) &&
				(split->parentThread == this) &&
				(split->childMask == 0ULL)) {
				return true;
			}
			return (searching || terminate);
		});
	}
}

} // namespace Cheese

