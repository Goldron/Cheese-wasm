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

#include "search.h"

#include "engine.h"
#include "util/logfile.h"
#include "moveorder.h"
#include "moveparser.h"
#include "ybwc.h"

#include "tablebases.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace Cheese {

// default value for aspiration window
constexpr int search_window = 30;

// time before sending currmove to gui (ms)
constexpr std::uint64_t wait_send_current_move = 3000;

constexpr int nullmoves_verification_depth = 12 * one_ply;

// maximum depth for razoring
constexpr int razoring_depth = 4;

constexpr int delta_pruning_margin = 200;

// min depth for internal iterative deepening
constexpr int internal_iterative_deepening_depth = 5;

constexpr int futility_pruning_depth = 4;

constexpr int extend_singleresponse = one_ply * 1;

// futility prunning margins (and reverse futility)
const std::array<int, futility_pruning_depth> futility_margin =
	{ 0, 100, 200, 300 };

// razoring margins
const std::array<int, razoring_depth> razor_margin =
	{ 0, 200, 400, 600 };

// LMR reduction table [depth][moveposition]
const std::array<std::array<std::uint32_t, lmr_max_depth>, lmr_max_moves> 
	ChessSearch::lmr_reduction = []() {
	constexpr double lmr_scale = 2.30;
	constexpr int lmr_max = 10;
	std::array<std::array<std::uint32_t, lmr_max_depth>, lmr_max_moves> 
		table {};
	for (int d=0; d<lmr_max_depth; d++) {
		for (int n=0; n<lmr_max_moves; n++) {
			int v = 0;
			if ((d > 0) && (n > 0)) {
				v = static_cast<int>(std::floor((std::log(d) * std::log(n)) / 
					lmr_scale));
				v = std::clamp(v, 0, lmr_max);
			}
			table[d][n] = v;
		}
	}
	return table;
} ();

// lmp limit [depth]
const std::array<int, lmp_max_depth> ChessSearch::lmp_count = []() {
	constexpr double lmp_base = 4.0;
	constexpr double lmp_scale = 0.9;
	std::array<int, lmp_max_depth> table {};
	for (int n=0; n<lmp_max_depth; n++) {
		table[n] = static_cast<int>(std::floor(lmp_base + (n * n) * lmp_scale));
	}
	return table;
} ();

////////////////////////////////////////////////////////////////////////////////
ChessSearch::ChessSearch(ChessSearchData &data, ChessEngine &e, 
	ChessSearchThreadList &ths, ChessEval &ev, ChessHashTable &ht, 
	ChessSearchThread *th, const ChessBoard &pos, bool chknodes) :
	engine(e), threads(ths), eval(ev), hashTable(ht), thread(th), board(pos), 
	repetition(data.repetition), searchStack(data.searchStack), pv(data.pv),
	needCheckNodes(chknodes)
{
}

////////////////////////////////////////////////////////////////////////////////
int ChessSearch::getLMRReduction(int depth, int num)
{
	int d = std::min(depth, lmr_max_depth - 1);
	int n = std::min(num, lmr_max_moves - 1);
	return static_cast<int>(lmr_reduction[d][n]);
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearch::startSearch()
{
	pv.clear(0);

	thread->initSearch();

	ChessMoveSortList &moves = engine.getMovesRoot();
	int maxmpv = std::min(engine.multipv, moves.size());
	maxmpv = std::min(maxmpv, max_multipv);

	ChessMultiPV &mpv = thread->mpv;
	mpv.clear();

	std::uint64_t hashfull = 0;	
	std::uint64_t searchtime = 0;

	ChessMove bestmove(0);
	ChessMove ponderMove(0);

	Timer timerIteration(engine.timerSearch);

	int maxdepth = (thread->isMainThread()) ? engine.ideepMaxDepth : max_depth;

	bool allowSendCurrMove = false;

	if (thread->isMainThread()) {
		engine.allowEndSearchNodes = false;
	}

	// iterative deepening
	for (int depth=1; depth<=maxdepth; ++depth) {

		thread->excludedMoves = engine.getExcludedMoves();

		mpv.updateScores();

		int value = 0;

		// multi-pv loop
		for (int ipv=0; ipv<maxmpv; ++ipv) {

			value = mpv.getInfo(ipv).score;

			int alpha = -mate_value - 1;
			int beta  =  mate_value + 1;

			// aspiration window after depth 5
			int dw = search_window;
			if (depth > 5) {
				alpha = std::max(value - dw, -mate_value - 1);
				beta  = std::min(value + dw,  mate_value + 1);
			}

			// aspiration window search
			int failtype = 0;
			do {

				failtype = 0;

				// start search at root
				int v = searchRoot(alpha, beta, depth * one_ply, 
					mpv.getInfo(ipv).getMove(0), allowSendCurrMove);

				searchtime += timerIteration.getElapsedTime();
				timerIteration.start();

				if constexpr (use_test_pv) {
					testPV(pv.getPV(0), pv.size(0));
				}

				// update multipv
				mpv.setScorePrev(ipv, v);
				if (!pv.isEmpty(0)) {
					mpv.copyPV(ipv, pv.getPV(0), pv.size(0), depth);
				}

				if (threads.mustStop()) {
					value = v;
					break;
				}

				// aspiration window
				if (v <= alpha) {
					failtype = 1;
					alpha = std::max(alpha - dw, -mate_value - 1);
					dw = dw * 2;
				} else if (v >= beta) {
					failtype = 2;
					beta  = std::min(beta + dw,  mate_value + 1);
					dw = dw * 2;
				} else {
					value = v;
				}

				// wait nefore sending currmove and hashfull to uci
				if ((thread->isMainThread()) &&
					(searchtime > wait_send_current_move)) {
					allowSendCurrMove = true;
				}

				if constexpr (use_hashfull) {
					if (allowSendCurrMove) {
						hashfull = hashTable.getHashFull();
					}
				}

				// send current line to gui if aspiration window failed
				if ((thread->isMainThread()) &&
					(failtype) &&
					(!threads.mustStop()) &&
					((allowSendCurrMove) ||
					 (engine.runtestsuite))) {

					for (int i=0; i<maxmpv; i++) {
						engine.onSendMultiPV((maxmpv > 1) ? i + 1 : 0,
							mpv.getInfo(i).depth, mpv.getInfo(i).scorePrev,
							mpv.getInfo(i).move,
							mpv.getInfo(i).nbmove,
							threads.getNodeCount(), searchtime,
							threads.getMaxPly(), failtype,
							threads.getTBHits(), hashfull);
					}
				}

			} while ((!threads.mustStop()) &&
					 (failtype));

			if (maxmpv > 1) {
				mpv.sortMultiPVLines();
			}

			// send current line to gui
			if ((thread->isMainThread()) &&
				(!pv.isEmpty(0))) {

				if ((maxmpv == 1) ||
					(ipv == (maxmpv - 1)) ||
					(allowSendCurrMove)) {
					for (int i=0; i<maxmpv; i++) {
						engine.onSendMultiPV((maxmpv > 1) ? i + 1 : 0,
							mpv.getInfo(i).depth, mpv.getInfo(i).scorePrev,
							mpv.getInfo(i).move,
							mpv.getInfo(i).nbmove,
							threads.getNodeCount(), searchtime,
							threads.getMaxPly(), 0,
							threads.getTBHits(), hashfull);
					}
				}
			}

			if (threads.mustStop()) {
				break;
			}

			// exclude the current best move in next multi-pv search
			if (((ipv + 1) < maxmpv)) {
				thread->excludedMoves.push(pv.getMove(0, 0));
			}

		} // end multipv loop

		if ((thread->isMainThread()) &&
			(mpv.getInfo(0).scorePrev >= max_score_mate) &&
			(engine.getSearchMateValue() != 0) &&
			(mate_value - mpv.getInfo(0).scorePrev <= 
				(2 * engine.getSearchMateValue()))) {
			threads.stopSearch();
		}

		// special stop conditions in tournament
		if ((thread->isMainThread()) &&
			((engine.getSearchTimeByMove() != 0) && 
			 (engine.getSearchFixedTime() == 0)) &&
			(!engine.pondering) &&
			(!threads.mustStop())) {

			// only one move possible => stop searching
			if (moves.size() == 1) {
				threads.stopSearch();
			}

			// try to guess if we have enough time to end the next search
			if ((depth > 1)
				&& (searchtime > static_cast<std::uint64_t>(
					static_cast<double>(engine.ideepTimeByMove) * 0.60))) {
					threads.stopSearch();
			}
		}

		if (mpv.getInfo(0).size() != 0) {

			thread->updateSearch(mpv.getInfo(0).getMove(0),
				mpv.getInfo(0).getMove(1), depth, mpv.getInfo(0).scorePrev);

			// update best move & ponder move
			bestmove = mpv.getInfo(0).getMove(0);

			if (mpv.getInfo(0).size() > 1) {
				ponderMove = mpv.getInfo(0).getMove(1);
			} else {
				ponderMove.clear();
			}
		}

		if (threads.mustStop()) {
			break;
		}

		// let the main thread finish the search if we reach max depth
		if ((!thread->isMainThread()) &&
			(depth == maxdepth)) {
			--depth;
		}

		if ((engine.fixedNodesValue != 0) &&
			(thread->isMainThread())) {
			// allow node limit only after finishing depth 1
			engine.allowEndSearchNodes = true;
			auto nodes = threads.getNodeCount();
			if ((nodes >= engine.fixedNodesValue)) {
				threads.stopSearch();
				break;
			}
		}

	} // end depth loop

	// wait when pondering and reaching max depth
	while ((thread->isMainThread()) &&
			(engine.pondering) &&
			(!threads.mustStop())) {
	}
	engine.pondering = false;

	threads.stopSearch();

	// no bestmove take the first move in the list (should never happens)
	if (bestmove.isEmpty()) {
		bestmove = moves.front().move;
		Logger::debug() << "no best move after search !";
	}
}

////////////////////////////////////////////////////////////////////////////////
// search at root node
int ChessSearch::searchRoot(int alpha, int beta, int depth,
	ChessMove topmove, bool sendRootMove)
{
	int	ply = 0;
	int nm = 0;
	int	value = 0;

	// real depth
	int rdepth = depth / one_ply;

	auto &searchedMoves = searchStack[ply].searchedMoves;
	int countSearched = 0;

	ChessMove bestMove(0);

	clearPV(ply);
	
	bool incheck = board.inCheck();

	ChessMoveOrder moveorder(thread->history, searchStack[ply].movelist, 
		(incheck) ? MoveOrderPhase::hash_check : MoveOrderPhase::hash, 
		topmove, ChessMove(0), ChessMove(0), ChessMove(0));

	ChessMove move;

	while ((move = moveorder.getNextMove(board)) != ChessMove(0)) {

		if (needCheckNodes && (!engine.checkNodes())) {
			return 0;
		}

		// exclude moves
		if (thread->isExcludedMove(move)) {
			continue;
		}

		// make move 
		const ChessMoveRec rec = board.makeMove(move);

		if (!board.isPositionLegal()) {
			board.unMakeMove(move, rec);
			continue;
		}

		searchStack[ply].move = move;

		// increase number of nodes searched
		++thread->countNodes;

		repetition.push(board.getHashKey());

		// send currmove to GUI
		if (sendRootMove) {
			engine.searchingRootMove(rdepth, move, nm + 1);
		}

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

		if constexpr (use_extension_single_response) {
			if (engine.getMovesRoot().size() == 1) {
				ext += extend_singleresponse;
			}
		}

		// limit extension to 1 ply
		ext = std::min(ext, (one_ply * 1));

		int newdepth = depth + ext - one_ply;

		// Principal Variation Search (PVS)
		// note : no LMR or LMP at root
		if (nm > 0) {
			value = -search<NodeType::non_pv>(-alpha - 1, -alpha, newdepth, 
				ply + 1, chk);
		}

		// pvs research, or full window search for first move
		if ((nm == 0) ||
			((value > alpha) && (value < beta))) {
			value = -search<NodeType::pv>(-beta, -alpha, newdepth, ply + 1, 
				chk);
		}

		board.unMakeMove(move, rec);

		repetition.pop();

		if ((threads.mustStop()) || (thread->cutoff())) {
			return alpha;
		}

		++nm;

		if (value > alpha) {

			alpha = value;
			bestMove = move;

			// update pv (before beta cut or we have empty pv)
			pv.insertPV(ply, move, pv);

			if (value >= beta) {
				break;
			}
		}

		if ((move != bestMove) &&
			(!move.isCaptureOrPromotion())
			&& (countSearched < max_history_quiets)) {
				searchedMoves[countSearched++] = move;
		}

		// YBWC : split the search
		if ((threads.getMaxThreads() > 1)
			&& (thread->smp == SMPMode::ybwc)
			&& (depth > engine.getMinSplitDepth())
			&& (nm < (engine.getMovesRoot().size() - 1))
			&& (thread->canSplit())
			&& (threads.isThreadAvailable(thread))) {

			if (ChessSearchYBWC::splitSearch(this, &moveorder,
					alpha, beta, depth, ply, nm, true, incheck, true,
					false, 0, &alpha, &bestMove)) {
				break;
			}
		}
	}

	// no moves possible : mate or pat
	if (nm == 0) {
		return (incheck) ? (-mate_value + ply) : engine.getDrawValue(board);
	}

	// killer move + history heuristic
	if (alpha >= beta) {
		if ((!bestMove.isCaptureOrPromotion())
			&& (!incheck)) {

			thread->history.addKillerMove(bestMove, ply);

			thread->history.add(bestMove, board.side(), rdepth);

			for (int i=0; i<countSearched; i++) {
				thread->history.sub(searchedMoves[i], board.side(), rdepth);
			}
		}
	}

	hashTable.addNode(board.getHashKey(), bestMove, alpha, depth, ply,
		(alpha >= beta) ? HashNodeType::beta :
		bestMove.isEmpty() ? HashNodeType::alpha :
							 HashNodeType::exact);

	return alpha;
}

////////////////////////////////////////////////////////////////////////////////
// alpha-beta search function
template<NodeType nodetype>
int ChessSearch::search(int alpha, int beta, int depth, int ply, bool incheck)
{
	static constexpr bool nodepv = (nodetype == NodeType::pv);

	assert(alpha < beta);
	assert((nodepv) || (alpha == beta - 1));
	assert(alpha >= -mate_value - 1);
	assert(beta <= mate_value + 1);
	assert(incheck == board.inCheck());

	if (depth < one_ply) {
		return searchQuiescent<nodetype>(alpha, beta, 0, ply, incheck);
	}

	if constexpr (nodepv) {
		clearPV(ply);
	}

	// reduce engine strength by reducing search speed
	if (thread->isMainThread() && engine.isReducingSpeed()) {
		engine.reduceSpeed();
	}

	// check time and stop conditions
	if (thread->isMainThread()) {
		if (!engine.checkSearchTime()) {
			return 0;
		}
	} else {
		if (threads.mustStop()) {
			return 0;
		}
	}

	// maximum depth reached
	if (ply >= (max_depth - 1)) {
		return eval.evaluate(board, thread->hashTablePawn, alpha, beta);
	}

	// draw by material, fifty rule, or repetition
	if (board.evaluateDraw() ||
		((board.fiftyRule() >= 100) &&
			((!incheck) || (!engine.isMate(board)))) ||
		(repetition.check(board.getHashKey(), board.fiftyRule()))) {
		return engine.getDrawValue(board);
	}

	// maximum ply for uci seldepth
	if (ply > thread->maxPly) {
		thread->maxPly = ply;
	}

	// mate distance pruning
	alpha = std::max(alpha, -mate_value + ply);
	beta = std::min(beta, mate_value - ply);

	if (alpha >= beta) {
		return alpha;
	}

	// search in hashtable
	ChessMove hashmove(0);

	int v = 0;
	HashNodeType r = hashTable.findNode(board.getHashKey(), depth, ply,
							   alpha, beta, hashmove, v);

	if (r != HashNodeType::not_found) {
		return v;
	}

	// real depth
	int rdepth = depth / one_ply;

	// probe tablebases
	if constexpr (use_tablebases) {
		if ((engine.getTBCardinality() > 0) &&
			(board.fiftyRule() == 0) &&
			(!board.canCastle(castle_init))) {

			int nbp = BitBoards::bitCount(board.bitboardAll());
			if ((nbp <= TableBases::largest()) &&
				((nbp < TableBases::largest()) ||
					(rdepth >= engine.getTBProbeDepth()))) {

				// force to check time, tablebases access is slow
				if (thread->isMainThread()) {
					engine.forceCheckTime();
				}

				int score = 0;
				int rt = TableBases::probe_wdl(board, score);

				if (rt != 0) {

					++thread->tbhits;

					int drawScore = engine.getTB50MoveRule() ? 1 : 0;

					v = (score < -drawScore) ? min_score_mate + ply + 1 :
						(score > drawScore) ? max_score_mate - ply - 1 :
						score * 2 * drawScore + engine.getDrawValue(board);

					auto ht = (score < -drawScore) ? HashNodeType::alpha :
						(score > drawScore) ? HashNodeType::beta : 
						HashNodeType::exact;

					if ((ht == HashNodeType::exact) ||
						((ht == HashNodeType::alpha) && (v <= alpha)) ||
						((ht == HashNodeType::beta) && (v >= beta))) {

						// save in hashtable
						hashTable.addNode(board.getHashKey(), ChessMove(0), v,
							(max_depth - 1) * one_ply, ply, ht);
					}

					return v;
				}
			}
		}
	}

	// evaluate called once	when needed
	int evalpos = eval_infinity;
	int	value = 0;

	// Razoring
	if constexpr (use_razoring && !nodepv) {
		if ((!incheck)
			&& (depth < (razoring_depth * one_ply))) {

			evalpos = eval.evaluate(board, thread->hashTablePawn, alpha, beta);

			if ((evalpos + razor_margin[rdepth]) < beta) {

				int rbeta = beta - razor_margin[rdepth];
				value = searchQuiescent<NodeType::non_pv>(rbeta - 1, rbeta, 0, 
					ply, false);

				if (value < rbeta) {
					return value;
				}
			}
		}
	}

	// "Reverse futility pruning" / Futility pruning at child node
	if constexpr (use_reverse_futility && !nodepv) {
		if ((!incheck)
			&& (depth < (futility_pruning_depth * one_ply))
			&& (std::abs(beta) < max_score_mate)) {

			if (evalpos == eval_infinity) {
				evalpos = eval.evaluate(board, thread->hashTablePawn, alpha, 
					beta);
			}

			if ((evalpos - futility_margin[rdepth]) >= beta) {
				return evalpos;
			}
		}
	}

	// Null moves pruning
	if constexpr (use_nullmoves && !nodepv) {
		if ((!incheck)
			&& (!searchStack[ply - 1].move.isEmpty())
			&& (board.gamePhaseCount(board.side()) != 0)
			&& (depth >= (2 * one_ply))
			&& (engine.usingNullMoves())) {

			int depthNullMoves = (3 * one_ply) + (rdepth / 8) * one_ply;

			repetition.push(board.getHashKey());

			const ChessMoveRec recn = board.makeNullMove();

			searchStack[ply].move.clear();

			int ndepth = depth - depthNullMoves - one_ply;
			value = -search<NodeType::non_pv>(-beta, -beta + 1, ndepth, 
				ply + 1, false);

			board.unMakeNullMove(recn);

			repetition.pop();

			if (threads.mustStop()) {
				return 0;
			}

			if (value >= beta) {

				if constexpr (use_nullmoves_verification) {
					// verification at high depth
					if ((depth >= nullmoves_verification_depth)) {
						int vf = search<NodeType::non_pv>(beta - 1, beta, 
							ndepth, ply, false);
						if (vf >= beta) {
							return value;
						}
					}
				} else {
					return value;
				}
			}
		}
	}

	// Internal iterative deepening (IID)
	if constexpr (use_internal_iterative_deepening && nodepv) {
		if ((!incheck)
			&& (hashmove.isEmpty())
			&& (depth >= (internal_iterative_deepening_depth * one_ply))) {

			value = search<NodeType::pv>(alpha, beta, depth - (2 * one_ply), 
				ply, false);

			if constexpr (nodepv) {
				clearPV(ply);
			}

			hashmove = hashTable.getBestMove(board.getHashKey());
		}
	}

	if ((threads.mustStop()) || (thread->cutoff())) {
		return 0;
	}

	// init futility pruning
	bool futilityPruning = false;
	int futilityMargin = 0;

	if constexpr (use_futility_pruning && !nodepv) {
		if ((!incheck)
			&& (depth < (futility_pruning_depth * one_ply))
			&& (!isGamePhasePawnEnding(board.gamePhase()))) {

			if (evalpos == eval_infinity) {
				evalpos = eval.evaluate(board, thread->hashTablePawn, alpha, 
					beta);
			}

			if ((evalpos + futility_margin[rdepth]) <= alpha) {
				futilityPruning = true;
				futilityMargin = evalpos + futility_margin[rdepth];
			}
		}
	}

	bool condLMR = ((depth >= (lmr_min_depth * one_ply))
					&& (!incheck)
					&& (engine.usingLMR()));

	auto &searchedMoves = searchStack[ply].searchedMoves;
	int countSearched = 0;

	ChessMove bestMove(0);

	ChessMove lastmove = searchStack[ply - 1].move;

	ChessMoveOrder moveorder(thread->history, searchStack[ply].movelist, 
		(incheck) ? MoveOrderPhase::hash_check : MoveOrderPhase::hash, 
		hashmove, thread->history.getKiller1(ply), 
		thread->history.getKiller2(ply),
		thread->history.getHistoryMove(board.side(), lastmove));

	const HashKey hk = board.getHashKey();

	ChessMove move;

	// legal moves count
	int num = 0;

	while ((move = moveorder.getNextMove(board)) != ChessMove(0)) {

		if (needCheckNodes && (!engine.checkNodes())) {
			return 0;
		}

		const ChessMoveRec rec = board.makeMove(move);

		if (!board.isPositionLegal()) {
			board.unMakeMove(move, rec);
			continue;
		}

		searchStack[ply].move = move;

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
				if ((num > 0)
					&& (!chk)
					&& (ext == 0)
					&& (!move.isMovePawnPast4th(~board.side()))
					&& (!move.isCaptureOrPromotion())) {

					if (futilityMargin <= alpha) {
						pruning = true;
					}
				}
			}
		}

		// late moves pruning (LMP)
		if constexpr (use_late_move_pruning && !nodepv) {
			if ((moveorder.getPhase() == MoveOrderPhase::next_quiet)
				&& (!incheck)
				&& (!chk)
				&& (ext == 0)
				&& (!move.isCastle())
				&& (!move.isMovePawnPast4th(~board.side()))
				&& (depth < lmp_max_depth * one_ply)) {

				if (num > lmp_count[rdepth]) {
					pruning = true;
				}
			}
		}

		if (pruning) {
			++num;
			board.unMakeMove(move, rec);
			continue;
		}

		// increase number of nodes searched (after pruning)
		++thread->countNodes;

		repetition.push(hk);

		// Principal Variation Search (PVS)
		if (num > 0) {

			// Late Move Reduction (LMR)
			value = alpha + 1;
			if constexpr (use_late_move_reduction) {
				if ((condLMR)
					&& (num > 1)
					&& (!chk)
					&& (moveorder.getPhase() == MoveOrderPhase::next_quiet)
					&& (!move.isCastle())
					&& (ext == 0)) {

					int lr = getLMRReduction(rdepth, num);
					if constexpr (!nodepv) {
						lr++;
					}

					if (lr) {
						// LMR search
						value = -search<NodeType::non_pv>(-alpha - 1, -alpha, 
							newdepth - lr * one_ply, ply + 1, chk);
					}
				}
			}

			if (value > alpha) {
				// PVS search
				value = -search<NodeType::non_pv>(-alpha - 1, -alpha, newdepth, 
					ply + 1, chk);
			}
		}

		// PVS re-search, or full window search for the first move
		if ((num == 0) ||
			((value > alpha) && (value < beta))) {
			value = -search<nodetype>(-beta, -alpha, newdepth, ply + 1, chk);
		}

		++num;

		board.unMakeMove(move, rec);

		repetition.pop();

		if ((threads.mustStop()) || (thread->cutoff())) {
			return 0;
		}

		if (value > alpha) {

			alpha = value;
			bestMove = move;

			if (value >= beta) {
				break;
			}

			// update pv
			if constexpr (nodepv) {
				pv.insertPV(ply, move, pv);
			}
		}

		if ((move != bestMove) &&
			(!move.isCaptureOrPromotion()) &&
			(countSearched < max_history_quiets)) {
				searchedMoves[countSearched++] = move;
		}

		// YBWC : split the search
		if ((threads.getMaxThreads() > 1)
			&& (thread->smp == SMPMode::ybwc)
			&& (depth > engine.getMinSplitDepth())
			&& (thread->canSplit())
			&& (threads.isThreadAvailable(thread))) {
			if (ChessSearchYBWC::splitSearch(this, &moveorder, alpha, beta, 
				depth, ply, num, nodepv, incheck, false, futilityPruning, 
				futilityMargin, &alpha, &bestMove)) {

				if ((threads.mustStop()) || (thread->cutoff())) {
					return 0;
				}

				break;
			}
	   }
	}

	// if no moves possible : mate or pat
	if (num == 0) {
		return (incheck) ? (-mate_value + ply) : engine.getDrawValue(board);
	}

	// update killer move + history heuristic
	if (alpha >= beta) {

		if ((!bestMove.isCaptureOrPromotion())
			&& (!incheck))
		{
			thread->history.addKillerMove(bestMove, ply);

			thread->history.add(bestMove, board.side(), rdepth);

			for (int i=0; i<countSearched; i++) {
				thread->history.sub(searchedMoves[i], board.side(), rdepth);
			}

			thread->history.setHistoryMove(board.side(), lastmove, bestMove);
		}
	}

	// save result in hash table
	hashTable.addNode(board.getHashKey(),
		bestMove.isEmpty() ? hashmove : bestMove,
		alpha, depth, ply,
		(alpha >= beta) ? HashNodeType::beta :
			bestMove.isEmpty() ? HashNodeType::alpha :
								 HashNodeType::exact);

	return alpha;
}

////////////////////////////////////////////////////////////////////////////////
// alphabeta quiescence search
template<NodeType nodetype>
int	ChessSearch::searchQuiescent(int alpha, int beta, int depth, int ply,
	bool incheck)
{
	static constexpr bool nodepv = (nodetype == NodeType::pv);

	assert(depth <= 0);
	assert(alpha < beta);
	assert((nodepv) || (alpha == beta - 1));
	assert(alpha >= -mate_value - 1);
	assert(beta <= mate_value + 1);
	assert(incheck == board.inCheck());

	if (ply >= (max_depth - 1)) {
		return engine.getDrawValue(board);
	}

	if constexpr (nodepv) {
		pv.clear(ply);
	}

	// check time and stop conditions
	if (thread->isMainThread()) {
		if (!engine.checkSearchTime()) {
			return 0;
		}
	} else {
		if (threads.mustStop()) {
			return 0;
		}
	}

	// draw by material, fifty rule, or repetition
	if (board.evaluateDraw() ||
		((board.fiftyRule() >= 100) &&
			((!incheck) || (!engine.isMate(board)))) ||
		(repetition.check(board.getHashKey(), board.fiftyRule()))) {
		return engine.getDrawValue(board);
	}

	int value = 0;

	if (!incheck) {

		// evaluate position : stand pat
		value = eval.evaluate(board, thread->hashTablePawn, alpha, beta);

		if (value > alpha) {
			if (value >= beta) {
				return value;
			}
			alpha = value;
		}
	}

	bool genchecks = (!incheck) & (depth > depth_checks_quiescence);

	ChessMoveOrder moveorder(thread->history, searchStack[ply].movelist, 
		(incheck) ? MoveOrderPhase::init_quiescence_evasion :
			(genchecks ? MoveOrderPhase::init_quiescence_capture_checks : 
					     MoveOrderPhase::init_quiescence_capture));

	bool usepruning = false;
	int value_pruning = 0;

	if constexpr (use_delta_pruning && !nodepv) {
		if ((!incheck) &&
			(isGamePhaseBeforeEnding(board.gamePhase()))
		) {
			usepruning = true;
			value_pruning = value + delta_pruning_margin;
		}
	}

	const HashKey hk = board.getHashKey();

	ChessMove move;

	int num = 0;

	// for each moves
	while ((move = moveorder.getNextMove(board)) != ChessMove(0)) {

		const ChessMoveRec rec = board.makeMove(move);
		if (!board.isPositionLegal()) {
			board.unMakeMove(move, rec);
			continue;
		}

		++num;

		++thread->countNodes;

		bool chk = board.inCheck();

		if constexpr (use_delta_pruning) {
			if ((usepruning)
				&& (num > 0)
				&& (!incheck)
				&& (!chk)
				&& (!move.isPromotion())
			) {
				int v = value_pruning + pieces_value[move.capture()];
				if (v <= alpha) {
					board.unMakeMove(move, rec);
					continue;
				}
			}
		}

		repetition.push(hk);

		value = -searchQuiescent<nodetype>(-beta, -alpha, depth - 1, ply + 1, 
			chk);

		board.unMakeMove(move, rec);

		repetition.pop();

		if (value > alpha) {

			if (value >= beta) {
				return value;
			}

			if constexpr (nodepv) {
				pv.insertPV(ply, move, pv);
			}

			alpha = value;
		}
	}

	if ((incheck) && (num == 0)) {
		return (-mate_value + ply);
	}

	return alpha;
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearch::testPV(const ChessPVMoveList &moves, int sz)
{
	std::array<ChessMoveRec, max_depth_pv> rec;
	int n;
	for (n=0; n<sz; n++) {
		if (board.validMove(moves[n])) {
			rec[n] = board.makeMove(moves[n]);
			if (!board.isPositionLegal()) {
				Logger::warning() << "illegal PV move : " <<
					ChessMoveParser::move_to_string_san(moves[n], board);
				++n;
				break;
			}
		} else {
			Logger::warning() << "invalid PV move : " <<
					ChessMoveParser::move_to_string_san(moves[n], board);
			break;
		}
	}
	--n;
	while (n >= 0) {
		board.unMakeMove(moves[n], rec[n]);
		--n;
	}
}

} // namespace Cheese
