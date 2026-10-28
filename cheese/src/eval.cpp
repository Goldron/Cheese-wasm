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

#include "eval.h"

#include "evalparams.h"
#include "board.h"
#include "moveparser.h"
#include "util/configfile.h"
#include "endgame.h"
#include "hash.h"
#include "util/logfile.h"
#include "search.h"

#include <cassert>

#include <iomanip>
#include <sstream>

#include <iostream>

namespace Cheese {

// middle game material values
std::array<std::int32_t, nb_pieces_type> pieces_value;

// end game material values
std::array<std::int32_t, nb_pieces_type> pieces_value_end;

// maximum minor piece value in middle game
std::int32_t minor_piece_max_value = 0;

////////////////////////////////////////////////////////////////////////////////
// convert 64 square index to 32 ssquare index (a1..d8)
template<Side side>
inline Square squareIndexHalf(Square s)
{
	auto f = static_cast<int>((squareFile(s) < file_e) ? squareFile(s) : 
		(file_h - squareFile(s)));
	auto r = static_cast<int>(relRank<side>(squareRank(s)));
	return static_cast<Square>(r * nb_files_half + f);
}

////////////////////////////////////////////////////////////////////////////////
// update tuning term count
template<ChessEvalMode mode, Side s>
void add_term(EvalTerm &et, std::int32_t v)
{
	if constexpr (mode == ChessEvalMode::tuning) {
		et[s] += v;
	}
}

////////////////////////////////////////////////////////////////////////////////
// update tuning term count for square table
template<ChessEvalMode mode, Side s>
void add_term_square(std::array<EvalTerm, nb_squares_half> &et, Square src, 
	std::int32_t v)
{
	if constexpr (mode == ChessEvalMode::tuning) {
		auto sq = squareIndexHalf<s>(src);
		et[sq][s] += v;
	}
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluateKnights(const ChessBoard &board,
	ChessEvalParam &ev)
{
	EvalScore score;

	const BitBoard bbAttacks = ev.bbAttackByPiece[~side][pawn];

	ev.bbAttackByPiece[side][knight] = 0;

	BitBoard bbSrc = board.bitboardPiece(side, knight);

	BitBoards::scan(bbSrc, [&](Square src) {

		// psqt
		score += psqt[side][knight][src];

		add_term_square<evalmode, side>(terms.psqt[knight], src, 1);

		// material
		score += pieces_score[knight];

		add_term<evalmode, side>(terms.pieces_score[knight], 1);

		BitBoard bAtt = BitBoards::attackKnight(src);

		ev.bbAttackByPiece[side][knight] |= bAtt;

		// mobility
		bAtt = bAtt & ~board.bitboardColor(side) & ~bbAttacks;
		int mb = BitBoards::bitCount(bAtt);
		ev.scoremob[side] += mobility_knight[mb];

		add_term<evalmode, side>(terms.mobility_knight[mb], 1);

		// outpost
		if ((!(BitBoards::attackPawn(side, src) &
				board.bitboardPiece(~side, pawn))) &&
			  (BitBoards::attackPawn(~side, src) &
				board.bitboardPiece(side, pawn))) {
			score += outpost_knight[side][src];

			add_term_square<evalmode, side>(terms.outpost_knight, src, 1);
		}

		// trapped knight
		bool trapped = false;
		if constexpr (side == white) {
			if ((src == a7) &&
				(board.bitboardPiece(black, pawn) & BitBoards::square[b7]) &&
				(board.bitboardPiece(black, pawn) & BitBoards::square[a6])) {
				trapped = true;
			}
			if ((src == h7) &&
				(board.bitboardPiece(black, pawn) & BitBoards::square[g7]) &&
				(board.bitboardPiece(black, pawn) & BitBoards::square[h6])) {
				trapped = true;
			}
		} else {
			if ((src == a2) &&
				(board.bitboardPiece(white, pawn) & BitBoards::square[b2]) &&
				(board.bitboardPiece(white, pawn) & BitBoards::square[a3])) {
				trapped = true;
			}
			if ((src == h2) &&
				(board.bitboardPiece(white, pawn) & BitBoards::square[g2]) &&
				(board.bitboardPiece(white, pawn) & BitBoards::square[h3])) {
				trapped = true;
			}
		}

		if (trapped) {
			score += trapped_knight;

			add_term<evalmode, side>(terms.trapped_knight, 1);
		}
	});
	return score;
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluateBishops(const ChessBoard &board,
			ChessEvalParam &ev)
{
	EvalScore score;

	const BitBoard bbAttacks = ev.bbAttackByPiece[~side][pawn];

	ev.bbAttackByPiece[side][bishop] = 0;

	BitBoard bbSrc = board.bitboardPiece(side, bishop);

	BitBoards::scan(bbSrc, [&](Square src) {

		// psqt
		score += psqt[side][bishop][src];

		add_term_square<evalmode, side>(terms.psqt[bishop], src, 1);

		// material
		score += pieces_score[bishop];

		add_term<evalmode, side>(terms.pieces_score[bishop], 1);

		BitBoard bAtt = BitBoards::attackBishop(src, board.bitboardAll());

		ev.bbAttackByPiece[side][bishop] |= bAtt;

		// mobility
		bAtt = bAtt & ~board.bitboardColor(side) & ~bbAttacks;
		int mb = BitBoards::bitCount(bAtt);
		ev.scoremob[side] += mobility_bishop[mb];

		add_term<evalmode, side>(terms.mobility_bishop[mb], 1);

		// outpost
		if ((!(BitBoards::attackPawn(side, src) &
				board.bitboardPiece(~side, pawn))) &&
			  (BitBoards::attackPawn(~side, src) &
				board.bitboardPiece(side, pawn))) {
			score += outpost_bishop[side][src];

			add_term_square<evalmode, side>(terms.outpost_bishop, src, 1);
		}

		// trapped bishop
		bool trapped = false;
		if constexpr (side == white) {
			if ((src == a7) &&
				(board.bitboardPiece(black, pawn) & BitBoards::square[b6])) {
				trapped = true;
			}
			if ((src == h7) &&
				(board.bitboardPiece(black, pawn) & BitBoards::square[g6])) {
				trapped = true;
			}
		} else {
			if ((src == a2) &&
				(board.bitboardPiece(white, pawn) & BitBoards::square[b3])) {
				trapped = true;
			}
			if ((src == h2) &&
				(board.bitboardPiece(white, pawn) & BitBoards::square[g3])) {
				trapped = true;
			}
		}
		
		if (trapped) {
			score += trapped_bishop;

			add_term<evalmode, side>(terms.trapped_bishop, 1);
		}
	});

	return score;
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluateRooks(const ChessBoard &board,
			ChessEvalParam &ev)
{
	static constexpr Rank r7 = relRank<side>(rank_7);
	static constexpr Rank r8 = relRank<side>(rank_8);

	EvalScore score;

	const BitBoard bbAttacks = ev.bbAttackByPiece[~side][pawn];

	ev.bbAttackByPiece[side][rook] = 0;

	BitBoard bbSrc = board.bitboardPiece(side, rook);

	BitBoards::scan(bbSrc, [&](Square src) {

		// psqt
		score += psqt[side][rook][src];

		add_term_square<evalmode, side>(terms.psqt[rook], src, 1);

		// material
		score += pieces_score[rook];

		add_term<evalmode, side>(terms.pieces_score[rook], 1);

		BitBoard bAtt = BitBoards::attackRook(src, board.bitboardAll());

		ev.bbAttackByPiece[side][rook] |= bAtt;

		// mobility
		bAtt = bAtt & ~board.bitboardColor(side) & ~bbAttacks;
		int mb = BitBoards::bitCount(bAtt);
		ev.scoremob[side] += mobility_rook[mb];

		add_term<evalmode, side>(terms.mobility_rook[mb], 1);

		File f = squareFile(src);

		// rook on an open/semi-open file
		if (!(board.bitboardPiece(side, pawn) & BitBoards::file[f])) {
			if (!(board.bitboardPiece(~side, pawn) & BitBoards::file[f])) {
				score += rook_open_file;

				add_term<evalmode, side>(terms.rook_open_file, 1);

			} else {
				score += rook_semi_open_file;

				add_term<evalmode, side>(terms.rook_semi_open_file, 1);
			}
		}

		// rook on 7th rank, with king on 7th or 8th rank
		if (squareRank(src) == r7) {
			Rank rk = squareRank(board.kingPosition(~side));
			if ((rk == r7) || (rk == r8)) {
				score += rook_on_7th;
				add_term<evalmode, side>(terms.rook_on_7th, 1);
			}
		}

		// blocked rook
		bool blocked = false;
		if constexpr (side == white) {
			Square pk = board.kingPosition(white);
			if (((src == g1) || (src == h1)) &&
				(pk <= h1) && (pk > e1) && (pk < src)) {
				blocked = true;
			}
			if (((src == a1) || (src == b1) || (src == c1)) &&
				(pk <= h1) && (pk < d1) && (pk > src)) {
				blocked = true;
			}
		} else {
			Square pk = board.kingPosition(black);
			if (((src == g8) || (src == h8)) &&
				(pk >= a8) && (pk > e8) && (pk < src)) {
				blocked = true;
			}
			if (((src == a8) || (src == b8) || (src == c8)) &&
				(pk >= a8) && (pk < d8) && (pk > src)) {
				blocked = true;
			}
		}
		
		if (blocked) {
			score += blocked_rook;

			add_term<evalmode, side>(terms.blocked_rook, 1);
		}
	});

	return score;
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluateQueens(const ChessBoard &board,
			ChessEvalParam &ev)
{
	EvalScore score;

	const BitBoard bbAttacks = ev.bbAttackByPiece[~side][pawn];

	ev.bbAttackByPiece[side][queen] = 0;

	BitBoard bbSrc = board.bitboardPiece(side, queen);

	BitBoards::scan(bbSrc, [&](Square src) {

		// psqt
		score += psqt[side][queen][src];

		add_term_square<evalmode, side>(terms.psqt[queen], src, 1);

		// material
		score += pieces_score[queen];

		add_term<evalmode, side>(terms.pieces_score[queen], 1);

		BitBoard bAtt = BitBoards::attackQueen(src, board.bitboardAll());

		ev.bbAttackByPiece[side][queen] |= bAtt;

		// mobility
		bAtt = bAtt & ~board.bitboardColor(side) & ~bbAttacks;
		int mb = BitBoards::bitCount(bAtt);
		ev.scoremob[side] += mobility_queen[mb];

		add_term<evalmode, side>(terms.mobility_queen[mb], 1);
	});
	return score;
}

////////////////////////////////////////////////////////////////////////////////
// evaluate a King file (pawn shield + pawn storm)
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluateKingFile(const ChessBoard &board, BitBoard bm, 
	File f)
{
	EvalScore score;

	// pawn shield
	BitBoard bbp = board.bitboardPiece(side, pawn) & bm & BitBoards::file[f];
	Rank r = (bbp != 0ULL) ?
		relSquareRank<side>(BitBoards::getNearestSquare(side, bbp)) : rank_1;

	int fs = (f < file_e) ? f : (file_h - f);

	score += pawn_shield[fs][r];

	add_term<evalmode, side>(terms.pawn_shield[fs][r], 1);

	// pawn storm
	bbp = board.bitboardPiece(~side, pawn) & bm & BitBoards::file[f];
	Rank r2 = (bbp != 0ULL) ? 
		relSquareRank<side>(BitBoards::getNearestSquare(side, bbp)) : rank_1;

	int fs2 = (f < file_e) ? f : (file_h - f);

	int op = ((r != rank_1) && (r2 == r + 1)) ? 1 : 0;

	score += pawn_storm[op][fs2][r2];

	add_term<evalmode, side>(terms.pawn_storm[op][fs2][r2], 1);

	return score;
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluatePawnShield(const ChessBoard &board)
{
	EvalScore score;

	const Square pk = board.kingPosition(side);

	// pawn shield + pawn storm
	File f = std::min(std::max(squareFile(pk), file_b), file_g);

	Square sk = makeSquare(f, squareRank(pk));
	BitBoard bbk = BitBoards::square[sk - 1] |
				   BitBoards::square[sk] |
				   BitBoards::square[sk + 1];

	BitBoard bm = BitBoards::fillFront<side>(bbk);

	for (File n=f - 1; n<=f + 1; ++n) {
		score += evaluateKingFile<evalmode, side>(board, bm, n);
	}

	return score;
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluateKing(const ChessBoard &board, ChessEvalParam &ev)
{
	const Square pk = board.kingPosition(side);

	BitBoard bbAtt = BitBoards::attackKing(pk) &
		ev.bbAttackByPiece[~side][knight];
	std::int32_t v = BitBoards::bitCount(bbAtt) * king_attackers_weight[knight];
	bbAtt = BitBoards::attackKing(pk) & ev.bbAttackByPiece[~side][bishop];
	v += BitBoards::bitCount(bbAtt) * king_attackers_weight[bishop];
	bbAtt = BitBoards::attackKing(pk) & ev.bbAttackByPiece[~side][rook];
	v += BitBoards::bitCount(bbAtt) * king_attackers_weight[rook];
	bbAtt = BitBoards::attackKing(pk) & ev.bbAttackByPiece[~side][queen];
	v += BitBoards::bitCount(bbAtt) * king_attackers_weight[queen];

	ev.kingSafety[side] = EvalScore(-(v * v) / 2, -v / 16);

	add_term_square<evalmode, side>(terms.psqt[king], pk, 1);

	return psqt[side][king][pk];
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluatePawns(const ChessBoard &board,
			ChessEvalParam &ev)
{
	EvalScore score;

	ev.bbPassed[side] = 0;

	BitBoard bbSrc = board.bitboardPiece(side, pawn);

	BitBoards::scan(bbSrc, [&](Square src) {

		// psqt
		score += psqt[side][pawn][src];

		add_term_square<evalmode, side>(terms.psqt[pawn], src, 1);

		// material
		score += pieces_score[pawn];

		add_term<evalmode, side>(terms.pieces_score[pawn], 1);

		Square dst = relSquareUp<side>(src);

		// doubled pawn 
		bool doubled = false;
		BitBoard bd = BitBoards::fillFront<side>(BitBoards::square[dst]) &
			board.bitboardPiece(side, pawn);
		if (bd) {
			doubled = true;
		}

		// isolated pawn
		bool isolated = false;
		File f = squareFile(src);
		if (!(board.bitboardPiece(side, pawn) & BitBoards::files_around[f])) {
			isolated = true;
		}

		// passed pawn
		bd = BitBoards::fillBack<side>(board.bitboardPiece(~side, pawn)) &
			(BitBoards::square[dst] | BitBoards::attackPawn(side, src));

		bool backward = false;
		if (!bd) {
			if (!doubled) {
				// passed pawn
				ev.bbPassed[side] |= BitBoards::square[src];
			}
		} else {
			// backward pawn
			if (!isolated) {
				BitBoard bi = BitBoards::fillBack<side>(
					BitBoards::attackPawn(~side, dst));

				if ((!(bi & board.bitboardPiece(side, pawn))) &&
					((BitBoards::attackPawn(side, dst) &
					board.bitboardPiece(~side, pawn)))) {
					backward = true;
				}
			}
		}

		if (doubled) {
			score += doubled_pawn;

			add_term<evalmode, side>(terms.doubled_pawn, 1);
		}
		if (isolated) {
			score += isolated_pawn;

			add_term<evalmode, side>(terms.isolated_pawn, 1);
		}
		if (backward) {
			score += backward_pawn;

			add_term<evalmode, side>(terms.backward_pawn, 1);
		}
	});
	return score;
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode, Side side>
EvalScore ChessEval::evaluatePassedPawns(const ChessBoard &board,
	ChessEvalParam &ev)
{
	EvalScore score;

	BitBoard bbatt = 
		ev.bbAttackByPiece[~side][knight] |
		ev.bbAttackByPiece[~side][bishop] |
		ev.bbAttackByPiece[~side][rook] |
		ev.bbAttackByPiece[~side][queen] |
		BitBoards::attackKing(board.kingPosition(~side));

	BitBoard bbSrc = ev.bbPassed[side];

	BitBoards::scan(bbSrc, [&](Square src) {

		// distance to promotion
		Rank r = relSquareRank<side>(src);

		// base score
		score += passed_base[r];

		add_term<evalmode, side>(terms.passed_base[r], 1);

		// file bonus
		File f = squareFile(src);
		File sf = (f < file_e) ? f : static_cast<File>(file_h - f);

		score += passed_file[sf];

		add_term<evalmode, side>(terms.passed_file[sf], 1);

		if (r > rank_3) {

			// front square
			Square dst = relSquareUp<side>(src);

			// bonus if the front square is empty
			if (board.piece(dst) == no_piece) {
				BitBoard bf = BitBoards::fillFront<side>(BitBoards::square[src]);
				if ((bbatt & bf) == 0ULL) {

				//if ((bbatt & BitBoards::square[dst]) == 0ULL) {
					score += passed_free[r];

					add_term<evalmode, side>(terms.passed_free[r], 1);
				} else {
					score += passed_free_attacked[r];

					add_term<evalmode, side>(terms.passed_free_attacked[r], 1);
				}
			}

			// distance to ennemy king
			int dopp = squareDistance(dst, board.kingPosition(~side));
			score += passed_king_opp[r] * dopp;

			add_term<evalmode, side>(terms.passed_king_opp[r], dopp);

			// protected by king
			int down = squareDistance(dst, board.kingPosition(side));
			score += passed_king_own[r] * down;

			add_term<evalmode, side>(terms.passed_king_own[r], down);
		}
	});

	return score;
}

////////////////////////////////////////////////////////////////////////////////
int ChessEval::evaluate(const ChessBoard &board, ChessPawnHashTable &htp, 
	int alpha, int beta)
{
	return (!usePerso) ? 
		evaluatePosition<ChessEvalMode::normal>(board, htp, alpha, beta) :
		evaluatePosition<ChessEvalMode::personality>(board, htp, alpha, beta);
}

////////////////////////////////////////////////////////////////////////////////
int ChessEval::evaluateTuning(const ChessBoard &board, ChessPawnHashTable &htp)
{
	for (auto &p : parameters) {
		p.term[black] = 0;
		p.term[white] = 0;
	}

	return evaluatePosition<ChessEvalMode::tuning>(board, htp, 
		-eval_infinity, eval_infinity);
}

////////////////////////////////////////////////////////////////////////////////
template<ChessEvalMode evalmode>
int ChessEval::evaluatePosition(const ChessBoard &board, 
	ChessPawnHashTable &htp, int alpha, int beta)
{
	// disable lazy evaluation for tuning
	if constexpr (use_lazy_eval && (evalmode != ChessEvalMode::tuning)) {

		// Lazy evaluation
		int lazyeval = board.materialScore(white) - board.materialScore(black);
		if (board.side() == black) {
			lazyeval = -lazyeval;
		}
		
		if (((lazyeval + lazyEvalMargin) < alpha) ||
			((lazyeval - lazyEvalMargin) > beta)) {
			return lazyeval;
		}
	}

	int drawScale = ChessEndGames::max_draw_scale;

	// evaluate known endgames
	if constexpr (evalmode != ChessEvalMode::tuning) {
		if (isGamePhaseAfterMiddle(board.gamePhase())) {
			int v = 0;
			if (ChessEndGames::evalEndGames(board, v, drawScale)) {
				return v;
			}
		}
	}

	EvalScore score;

	// bishop pair
	if (countMaterial(board.materialCount(), white, bishop) >= 2) {
		score += bishop_pair;

		add_term<evalmode, white>(terms.bishop_pair, 1);
	}
	if (countMaterial(board.materialCount(), black, bishop) >= 2) {
		score -= bishop_pair;

		add_term<evalmode, black>(terms.bishop_pair, 1);
	}

	// blocked pawns
	if ((board.bitboardPiece(white, pawn) & BitBoards::square[d2]) &&
		(board.piece(d3) != no_piece)) {
		score += blocked_pawn;

		add_term<evalmode, white>(terms.blocked_pawn, 1);
	}

	if ((board.bitboardPiece(white, pawn) & BitBoards::square[e2]) &&
		(board.piece(e3) != no_piece)) {
		score += blocked_pawn;

		add_term<evalmode, white>(terms.blocked_pawn, 1);
	}

	if ((board.bitboardPiece(black, pawn) & BitBoards::square[d7]) &&
		(board.piece(d6) != no_piece)) {
		score -= blocked_pawn;

		add_term<evalmode, black>(terms.blocked_pawn, 1);
	}

	if ((board.bitboardPiece(black, pawn) & BitBoards::square[e7]) &&
		(board.piece(e6) != no_piece)) {
		score -= blocked_pawn;

		add_term<evalmode, black>(terms.blocked_pawn, 1);
	}

	ChessEvalParam ev;

	// evaluate pawns
	if constexpr (evalmode == ChessEvalMode::tuning) {
		score += evaluatePawns<evalmode, white>(board, ev) -
				 evaluatePawns<evalmode, black>(board, ev);
	} else {

		if (board.getHashKeyPawns() != 0ULL) {

			EvalScore vp;

			// read pawn hash table
			auto *hashp = htp.getHashNode(board.getHashKeyPawns());

			if (hashp->key == board.getHashKeyPawns()) {

				hashp->get(ev.bbPassed, vp.mid, vp.end);

			} else {

				vp = evaluatePawns<evalmode, white>(board, ev) - 
					 evaluatePawns<evalmode, black>(board, ev);
		
				hashp->set(board.getHashKeyPawns(), ev.bbPassed, vp.mid, 
					vp.end);
			}
			score += vp;
		} else {
			ev.bbPassed[black] = 0;
			ev.bbPassed[white] = 0;
		}
	}

	// generate pawn attacks bitboards
	ev.bbAttackByPiece[black][pawn] =
		BitBoards::allPawnsAttacks<black>(board.bitboardPiece(black, pawn));

	ev.bbAttackByPiece[white][pawn] =
		BitBoards::allPawnsAttacks<white>(board.bitboardPiece(white, pawn));

	ev.scoremob[black] = 0;
	ev.scoremob[white] = 0;

	// evaluate knights
	score += evaluateKnights<evalmode, white>(board, ev) -
	         evaluateKnights<evalmode, black>(board, ev);

	// evaluate bishops
	score += evaluateBishops<evalmode, white>(board, ev) -
			 evaluateBishops<evalmode, black>(board, ev);

	// evaluate rooks
	score += evaluateRooks<evalmode, white>(board, ev) - 
			 evaluateRooks<evalmode, black>(board, ev);

	// evaluate queens
	score += evaluateQueens<evalmode, white>(board, ev) -
			 evaluateQueens<evalmode, black>(board, ev);

	// evaluate kings
	score += evaluateKing<evalmode, white>(board, ev) - 
			 evaluateKing<evalmode, black>(board, ev);

	if constexpr (evalmode != ChessEvalMode::personality) {

		// evaluate pawn shield
		score += evaluatePawnShield<evalmode, white>(board) - 
			     evaluatePawnShield<evalmode, black>(board);

		// evaluate passed pawns
		score += evaluatePassedPawns<evalmode, white>(board, ev) - 
				 evaluatePassedPawns<evalmode, black>(board, ev);	

		// add mobility and king safety
		score += ev.scoremob[white] - ev.scoremob[black];

		score += ev.kingSafety[white] - ev.kingSafety[black];

	} else {
	
		// evaluate pawn shield
		score += (evaluatePawnShield<evalmode, white>(board) * 
			perso.pawnShield) / 100;
		score -= (evaluatePawnShield<evalmode, black>(board) * 
			perso.pawnShield) / 100;

		// evaluate passed pawns
		score += (evaluatePassedPawns<evalmode, white>(board, ev) * 
			perso.passedPawns) / 100;
		score -= (evaluatePassedPawns<evalmode, black>(board, ev) * 
			perso.passedPawns) / 100;

		// add mobility and king safety
		int pw = (engineSide == white) ? perso.kingSafetyOwn : 
			perso.kingSafetyOpp;
		int pb = (engineSide == white) ? perso.kingSafetyOpp : 
			perso.kingSafetyOwn;

		score += (ev.scoremob[white] * pw - ev.scoremob[black] * pb) / 100;

		score += (ev.kingSafety[white] * pw -
				  ev.kingSafety[black] * pb) / 100;
	}

	// scale drawish endgames
	score.end = (score.end * drawScale) / ChessEndGames::max_draw_scale;

	// tappered evaluation
	auto p = static_cast<int>(std::min(board.gamePhase(), max_game_phase));
	int v = score.interpolate(p, max_game_phase);

	if constexpr (evalmode == ChessEvalMode::personality) {
		if (perso.randomEval != 0) {
			// r = 1.0 => maximum strength, r = 0.0 => random eval
			int r = (100 - perso.randomEval) / 100;
			// TODO : use real random numbers + seed
			auto rval = static_cast<int>(board.getHashKey() % 200) - 100;
			v = ((v * r) + rval * (100 - r)) / 100;
		}
	}

	return ((board.side() == white) ? v : -v) + eval_tempo;
}

////////////////////////////////////////////////////////////////////////////////
void ChessEval::clearEvalData()
{
	psqt = {};
	outpost_knight = {};
	outpost_bishop = {};
	mobility_knight = {};
	mobility_bishop = {};
	mobility_rook = {};
	mobility_queen = {};
	passed_base = {};
	passed_free = {};
	passed_file = {};
	passed_king_own = {};
	passed_king_opp = {};
	pawn_shield = {};
	pawn_storm = {};
	doubled_pawn = {};
	isolated_pawn = {};
	backward_pawn = {};
	rook_open_file = {};
	rook_semi_open_file = {};
	rook_on_7th = {};
	blocked_pawn = {};
	trapped_knight = {};
	trapped_bishop = {};
	blocked_rook = {};

	pieces_score = {{
		{ 0, 0 }, { 100, 100 }, { 325, 325 }, { 325, 325 }, { 500, 500 }, 
		{ 975, 975 }, { 0, 0 }, { 0, 0 }
	}};
	bishop_pair = { 50, 50 };

	king_attackers_weight = Params::king_attackers_weight;

	initEval();
}

////////////////////////////////////////////////////////////////////////////////
void ChessEval::initEvalData()
{
	for (int p=0; p<nb_pieces_type; p++) {
		for (int y=0; y<nb_ranks; y++) {
			for (int x=0; x<nb_files; x++) {
				auto s = y * nb_files + x;
				auto sr = (nb_ranks - 1 - y) * nb_files + x;
				auto sp = y * nb_files_half + 
					((x < nb_files_half) ? x : (nb_files - 1 - x));
				psqt[black][p][sr] = Params::psqt[p][sp];
				psqt[white][p][s] = Params::psqt[p][sp];
			}
		}
	}

	for (int y=0; y<nb_ranks; y++) {
		for (int x=0; x<nb_files; x++) {
			auto s = y * nb_files + x;
			auto sr = (nb_ranks - 1 - y) * nb_files + x;
			auto sp = y * nb_files_half + 
				((x < nb_files_half) ? x : (nb_files - 1 - x));
			outpost_knight[black][sr] = Params::outpost_knight[sp];
			outpost_knight[white][s] = Params::outpost_knight[sp];
			outpost_bishop[black][sr] = Params::outpost_bishop[sp];
			outpost_bishop[white][s] = Params::outpost_bishop[sp];
		}
	}

	mobility_knight = Params::mobility_knight;
	mobility_bishop = Params::mobility_bishop;
	mobility_rook = Params::mobility_rook;
	mobility_queen = Params::mobility_queen;
	passed_base = Params::passed_base;
	passed_free = Params::passed_free;
	passed_free_attacked = Params::passed_free_attacked;
	passed_file = Params::passed_file;
	passed_king_own = Params::passed_king_own;
	passed_king_opp = Params::passed_king_opp;
	pawn_shield = Params::pawn_shield;
	pawn_storm = Params::pawn_storm;
	pieces_score = Params::pieces_score;
	bishop_pair = Params::bishop_pair;
	doubled_pawn = Params::doubled_pawn;
	isolated_pawn = Params::isolated_pawn;
	backward_pawn = Params::backward_pawn;
	rook_open_file = Params::rook_open_file;
	rook_semi_open_file = Params::rook_semi_open_file;
	rook_on_7th = Params::rook_on_7th;
	blocked_pawn = Params::blocked_pawn;
	trapped_knight = Params::trapped_knight;
	trapped_bishop = Params::trapped_bishop;
	blocked_rook = Params::blocked_rook;
	king_attackers_weight = Params::king_attackers_weight;

	initEval();
	initTuning();
}

////////////////////////////////////////////////////////////////////////////////
void ChessEval::initEval()
{
	for (int i=0; i<nb_pieces_type; i++) {
		pieces_value[i] = pieces_score[i].mid;
		pieces_value_end[i] = pieces_score[i].end;
	}

	minor_piece_max_value = std::max(pieces_score[knight].mid, 
		pieces_score[bishop].mid);

	startMaterialValue = pieces_score[pawn].mid * 8 +
						 pieces_score[knight].mid * 2 +
						 pieces_score[bishop].mid * 2 +
						 pieces_score[rook].mid * 2 +
						 pieces_score[queen].mid;

	lazyEvalMargin = std::max({
		pieces_score[knight].mid, pieces_score[knight].end, 
		pieces_score[bishop].mid, pieces_score[bishop].end});
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEval::loadPersonality(const std::string &fname)
{
	ConfigFile conf;
	usePerso = false;
	if (conf.load(fname)) {

		Logger::info() << "load personality " << fname;

		clearEvalData();

		auto strEvalData = conf.get("EvalData");
		bool newdata = false;
		if (!strEvalData.empty()) {
			if (!loadEval(strEvalData)) {
				Logger::error() << "error loading eval data file";
			} else {
				newdata = true;
			}
		}

		if (!newdata) {
			initEvalData();
		}

		perso.materialPawn = conf.getInt("PawnValueMid", 
			pieces_score[pawn].mid);
		perso.materialKnight = conf.getInt("KnightValueMid", 
			pieces_score[knight].mid);
		perso.materialBishop = conf.getInt("BishopValueMid", 
			pieces_score[bishop].mid);
		perso.materialRook = conf.getInt("RookValueMid", 
			pieces_score[rook].mid);
		perso.materialQueen = conf.getInt("QueenValueMid", 
			pieces_score[queen].mid);
		perso.materialPawnEnd = conf.getInt("PawnValueEnd", 
			pieces_score[pawn].end);
		perso.materialKnightEnd = conf.getInt("KnightValueEnd", 
			pieces_score[knight].end);
		perso.materialBishopEnd = conf.getInt("BishopValueEnd", 
			pieces_score[bishop].end);
		perso.materialRookEnd = conf.getInt("RookValueEnd", 
			pieces_score[rook].end);
		perso.materialQueenEnd = conf.getInt("QueenValueEnd", 
			pieces_score[queen].end);

		perso.bishopPairMid = conf.getInt("BishopPairMid", 
			bishop_pair.mid);
		perso.bishopPairEnd = conf.getInt("BishopPairEnd", 
			bishop_pair.end);
		perso.pawnDoubledMid = conf.getInt("PawnDoubledMid", 
			doubled_pawn.mid);
		perso.pawnDoubledEnd = conf.getInt("PawnDoubledEnd", 
			doubled_pawn.end);
		perso.pawnIsolatedMid = conf.getInt("PawnIsolatedMid", 
			isolated_pawn.mid);
		perso.pawnIsolatedEnd = conf.getInt("PawnIsolatedEnd", 
			isolated_pawn.end);
		perso.pawnBackwardMid = conf.getInt("PawnBackwardMid", 
			backward_pawn.mid);
		perso.pawnBackwardEnd = conf.getInt("PawnBackwardEnd", 
			backward_pawn.end);
		perso.rookOpenFileMid = conf.getInt("RookOpenFileMid", 
			rook_open_file.mid);
		perso.rookOpenFileEnd = conf.getInt("RookOpenFileEnd", 
			rook_open_file.end);
		perso.rookSemiOpenFileMid = conf.getInt("RookSemiOpenFileMid", 
			rook_semi_open_file.mid);
		perso.rookSemiOpenFileEnd = conf.getInt("RookSemiOpenFileEnd", 
			rook_semi_open_file.end);
		perso.rook7thRankMid = conf.getInt("Rook7thRankMid", 
			rook_on_7th.mid);
		perso.rook7thRankEnd = conf.getInt("Rook7thRankEnd", 
			rook_on_7th.end);

		perso.outpostKnight = conf.getInt("OutpostKnight", 100);
		perso.outpostBishop = conf.getInt("OutpostBishop", 100);
		perso.passedPawns = conf.getInt("PassedPawns", 100);
		perso.pawnShield = conf.getInt("PawnShield", 100);
		perso.kingSafetyOwn = conf.getInt("KingSafetyOwn", 100);
		perso.kingSafetyOpp = conf.getInt("KingSafetyOpp", 100);
		perso.mobilityOwn = conf.getInt("MobilityOwn", 100);
		perso.mobilityOpp = conf.getInt("MobilityOpp", 100);
		perso.randomEval = conf.getInt("Randomness", 0);
		
		usePerso = true;

		initPersonality();
		initEval();
		
	} else {
		Logger::warning() << "error loading personality file";

		initEvalData();
	}

	return usePerso;
}

////////////////////////////////////////////////////////////////////////////////
void ChessEval::initPersonality()
{
	Logger::info() << "init personality";
	
	pieces_score[pawn].mid = perso.materialPawn;
	pieces_score[knight].mid = perso.materialKnight;
	pieces_score[bishop].mid = perso.materialBishop;
	pieces_score[rook].mid = perso.materialRook;
	pieces_score[queen].mid = perso.materialQueen;
	pieces_score[pawn].end = perso.materialPawnEnd;
	pieces_score[knight].end = perso.materialKnightEnd;
	pieces_score[bishop].end = perso.materialBishopEnd;
	pieces_score[rook].end = perso.materialRookEnd;
	pieces_score[queen].end = perso.materialQueenEnd;
	bishop_pair.mid = perso.bishopPairMid;
	bishop_pair.end = perso.bishopPairEnd;
	doubled_pawn.mid = perso.pawnDoubledMid;
	doubled_pawn.end = perso.pawnDoubledEnd;
	isolated_pawn.mid = perso.pawnIsolatedMid;
	isolated_pawn.end = perso.pawnIsolatedEnd;
	backward_pawn.mid = perso.pawnBackwardMid;
	backward_pawn.end = perso.pawnBackwardEnd;
	rook_open_file.mid = perso.rookOpenFileMid;
	rook_open_file.end = perso.rookOpenFileEnd;
	rook_semi_open_file.mid = perso.rookSemiOpenFileMid;
	rook_semi_open_file.end = perso.rookSemiOpenFileEnd;
	rook_on_7th.mid = perso.rook7thRankMid;
	rook_on_7th.end = perso.rook7thRankEnd;

	for (int i=0; i<nb_squares; i++) {
		outpost_knight[black][i] = (outpost_knight[black][i] * perso.outpostKnight) / 100;
		outpost_knight[white][i] = (outpost_knight[white][i] * perso.outpostKnight) / 100;
		outpost_bishop[black][i] = (outpost_bishop[black][i] * perso.outpostBishop) / 100;
		outpost_bishop[white][i] = (outpost_bishop[white][i] * perso.outpostBishop) / 100;
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEval::addTuningPSQT(const std::string &name, Piece piece)
{
	for (Rank y=rank_1; y<=rank_8; ++y) {
		for (File x=file_a; x<=file_d; ++x) {
			Square p = makeSquare(x, y);
			if ((piece != pawn) || ((y > rank_1) && (y < rank_8))) {
				std::string strname = name + std::string("_") + 
					ChessMoveParser::square_to_string(p);

				int s = y * nb_files + x;
				int sp = y * nb_files_half + x;

				parameters.emplace_back(strname, psqt[white][piece][s], 
					terms.psqt[piece][sp]);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEval::initTuning()
{
	parameters.clear();

	addTuningPSQT("psqt_pawn", pawn);
	addTuningPSQT("psqt_knight", knight);
	addTuningPSQT("psqt_bishop", bishop);
	addTuningPSQT("psqt_rook", rook);
	addTuningPSQT("psqt_queen", queen);
	addTuningPSQT("psqt_king", king);

	for (Rank y=rank_4; y<=rank_6; ++y) {
		for (File x=file_b; x<=file_d; ++x) {

			Square p = makeSquare(x, y);
			int s = y * nb_files + x;
			int sp = y * nb_files_half + x;

			std::string strname = std::string("outpost_knight_") + 
				ChessMoveParser::square_to_string(p);
			parameters.emplace_back(strname, outpost_knight[white][s], 
				terms.outpost_knight[sp]);

			strname = std::string("outpost_bishop_") + 
				ChessMoveParser::square_to_string(p);
			parameters.emplace_back(strname, outpost_bishop[white][s], 
				terms.outpost_bishop[sp]);
		}
	}

	for (int n=0; n<max_mobility_knight; n++) {
		std::string str = "mobility_knight_" + std::to_string(n + 1);
		parameters.emplace_back(str, mobility_knight[n], 
			terms.mobility_knight[n]);
	}
	for (int n=0; n<max_mobility_bishop; n++) {
		std::string str = "mobility_bishop_" + std::to_string(n + 1);
		parameters.emplace_back(str, mobility_bishop[n], 
			terms.mobility_bishop[n]);
	}
	for (int n=0; n<max_mobility_rook; n++) {
		std::string str = "mobility_rook_" + std::to_string(n + 1);
		parameters.emplace_back(str, mobility_rook[n], 
			terms.mobility_rook[n]);
	}
	for (int n=0; n<max_mobility_queen; n++) {
		std::string str = "mobility_queen_" + std::to_string(n + 1);
		parameters.emplace_back(str, mobility_queen[n], 
			terms.mobility_queen[n]);
	}

	for (int n=2; n<nb_ranks-1; n++) {
		parameters.emplace_back("passed_pawn_base_" + std::to_string(n + 1), 
			passed_base[n], terms.passed_base[n]);
	}

	for (int n=3; n<nb_ranks-1; n++) {
		parameters.emplace_back("passed_pawn_free_" + std::to_string(n + 1), 
			passed_free[n], terms.passed_free[n]);
	}

	for (int n=3; n<nb_ranks-1; n++) {
		parameters.emplace_back("passed_pawn_free_attacked_" + std::to_string(n + 1), 
			passed_free_attacked[n], terms.passed_free_attacked[n]);
	}

	for (int n=3; n<nb_ranks-1; n++) {
		parameters.emplace_back("passed_pawn_king_own_" + std::to_string(n + 1), 
			passed_king_own[n], terms.passed_king_own[n]);
	}

	for (int n=3; n<nb_ranks-1; n++) {
		parameters.emplace_back("passed_pawn_king_opp_" + std::to_string(n + 1), 
			passed_king_opp[n], terms.passed_king_opp[n]);
	}

	for (int n=0; n<nb_files_half; n++) {
		parameters.emplace_back("passed_file_" + std::to_string(n + 1),
			passed_file[n], terms.passed_file[n]);
	}

	for (int n=0; n<nb_ranks-1; n++) {
		parameters.emplace_back("eval_pawn_shield_a_rank_" + 
			std::to_string(n + 1), pawn_shield[0][n], terms.pawn_shield[0][n]);
	}
	for (int n=0; n<nb_ranks-1; n++) {
		parameters.emplace_back("eval_pawn_shield_b_rank_" + 
			std::to_string(n + 1), pawn_shield[1][n], terms.pawn_shield[1][n]);
	}
	for (int n=0; n<nb_ranks-1; n++) {
		parameters.emplace_back("eval_pawn_shield_c_rank_" + 
			std::to_string(n + 1), pawn_shield[2][n], terms.pawn_shield[2][n]);
	}
	for (int n=0; n<nb_ranks-1; n++) {
		parameters.emplace_back("eval_pawn_shield_d_rank_" + 
			std::to_string(n + 1), pawn_shield[3][n], terms.pawn_shield[3][n]);
	}

	for (int i=0; i<nb_files_half; i++) {
		for (int n=rank_1; n<=rank_5; n++) {
			parameters.emplace_back("eval_pawn_storm_free_" + 
				std::to_string(i + 1) + "_rank_" + std::to_string(n + 1), 
				pawn_storm[0][i][n], terms.pawn_storm[0][i][n]);
		}
	}

	for (int i=0; i<nb_files_half; i++) {
		for (int n=rank_3; n<=rank_5; n++) {
			parameters.emplace_back("eval_pawn_storm_blocked_" + 
				std::to_string(i + 1) + "_rank_" + std::to_string(n + 1), 
				pawn_storm[1][i][n], terms.pawn_storm[1][i][n]);
		}
	}

	parameters.emplace_back("material_pawn", pieces_score[pawn], 
		terms.pieces_score[pawn]);
	parameters.emplace_back("material_knight", pieces_score[knight], 
		terms.pieces_score[knight]);
	parameters.emplace_back("material_bishop", pieces_score[bishop], 
		terms.pieces_score[bishop]);
	parameters.emplace_back("material_rook", pieces_score[rook], 
		terms.pieces_score[rook]);
	parameters.emplace_back("material_queen", pieces_score[queen], 
		terms.pieces_score[queen]);

	parameters.emplace_back("bishop_pair", bishop_pair, terms.bishop_pair);

	parameters.emplace_back("doubled_pawn", doubled_pawn, 
		terms.doubled_pawn);
	parameters.emplace_back("isolated_pawn", isolated_pawn, 
		terms.isolated_pawn);
	parameters.emplace_back("backward_pawn", backward_pawn, 
		terms.backward_pawn);

	parameters.emplace_back("rook_open_file", rook_open_file, 
		terms.rook_open_file);
	parameters.emplace_back("rook_semiopen_file", rook_semi_open_file, 
		terms.rook_semi_open_file);
	parameters.emplace_back("rook_7th", rook_on_7th, terms.rook_on_7th);

	parameters.emplace_back("trapped_pawn", blocked_pawn, 
		terms.blocked_pawn);
	parameters.emplace_back("trapped_knight", trapped_knight, 
		terms.trapped_knight);
	parameters.emplace_back("trapped_bishop", trapped_bishop, 
		terms.trapped_bishop);
	parameters.emplace_back("trapped_rook", blocked_rook, 
		terms.blocked_rook);
}

////////////////////////////////////////////////////////////////////////////////
constexpr auto print_eval_score = [](std::stringstream &sstr, 
	const EvalScore &s) {
	sstr << "(";
	sstr << std::setfill(' ') << std::setw(4) << s.mid;
	sstr << ", "; 
	sstr << std::setfill(' ') << std::setw(4) << s.end;
	sstr << ");\n";
};

////////////////////////////////////////////////////////////////////////////////
template<std::size_t sz>
constexpr auto print_eval_score_array = [] (std::stringstream &sstr,
	const std::array<EvalScore, sz> &scores, std::size_t nb) {
	sstr << "        ";
	for (auto i=0U; i<scores.size(); i++) {
		if ((i != 0) && ((i % nb) == 0)) {
			sstr << std::endl;
			sstr << "        ";
		}
		sstr << "{ "; 
		sstr << std::setfill(' ') << std::setw(4) << scores[i].mid; 
		sstr << ", ";
		sstr << std::setfill(' ') << std::setw(4) << scores[i].end;
		sstr << " }";
		if (i != (scores.size() - 1)) {
			sstr << ", ";
		}
	}
};

////////////////////////////////////////////////////////////////////////////////
constexpr auto print_eval_score_psqt = [] (std::stringstream &sstr,
	const std::array<EvalScore, nb_squares> &scores) {
	sstr << "        ";
	for (auto j=0U; j<nb_ranks; j++) {
		for (auto i=0U; i<nb_files_half; i++) {

			auto p = j * nb_files + i;

			sstr << "{ "; 
			sstr << std::setfill(' ') << std::setw(4) << scores[p].mid;
			sstr << ", ";
			sstr << std::setfill(' ') << std::setw(4) << scores[p].end;
			sstr << " }";

			if ((j != (nb_ranks - 1)) || (i != (nb_files_half - 1))) {
				sstr << ", ";
			}
		}
		if (j != (nb_ranks - 1)) {
			sstr << std::endl;
			sstr << "        ";
		}
	}
};

////////////////////////////////////////////////////////////////////////////////
std::string ChessEval::printEval() const
{
	std::stringstream sstr;

	sstr << "inline constexpr std::array<std::array<EvalScore, nb_squares_half>, ";
	sstr << "nb_pieces_type> psqt = \n";
	sstr << "{{\n";
	sstr << "    {},\n";
	
	for (int p=pawn; p<=king; p++) {		
		sstr << "    {{\n";
		print_eval_score_psqt(sstr, psqt[white][p]);
		sstr << "\n    }},\n";
	}

	sstr << "    {}\n";
	sstr << "}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_squares_half> outpost_knight = ";
	sstr << "{{\n";
	print_eval_score_psqt(sstr, outpost_knight[white]);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_squares_half> outpost_bishop = ";
	sstr << "{{\n";
	print_eval_score_psqt(sstr, outpost_bishop[white]);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, max_mobility_knight> ";
	sstr << "mobility_knight = ";
	sstr << "{{\n";
	print_eval_score_array<max_mobility_knight>(sstr, mobility_knight, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, max_mobility_bishop> ";
	sstr << "mobility_bishop = ";
	sstr << "{{\n";
	print_eval_score_array<max_mobility_bishop>(sstr, mobility_bishop, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, max_mobility_rook> ";
	sstr << "mobility_rook = ";
	sstr << "{{\n";
	print_eval_score_array<max_mobility_rook>(sstr, mobility_rook, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, max_mobility_queen> ";
	sstr << "mobility_queen = ";
	sstr << "{{\n";
	print_eval_score_array<max_mobility_queen>(sstr, mobility_queen, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_ranks> passed_base = ";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, passed_base, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_ranks> passed_free = ";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, passed_free, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_ranks> passed_free_attacked = ";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, passed_free_attacked, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_files_half> passed_file = ";
	sstr << "{{\n";
	print_eval_score_array<nb_files_half>(sstr, passed_file, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_ranks> passed_king_own = ";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, passed_king_own, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_ranks> passed_king_opp = ";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, passed_king_opp, 4);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<std::array<EvalScore, nb_ranks>, nb_files_half> ";
	sstr << "pawn_shield = ";
	sstr << "{{\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_shield[0], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_shield[1], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_shield[2], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_shield[3], 4);
	sstr << "\n}}\n";
	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<std::array<std::array<EvalScore, nb_ranks>, ";
	sstr << "nb_files_half>, nb_storm_type> pawn_storm = ";
	sstr << "{{\n";

	sstr << "{{\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_storm[0][0], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_storm[0][1], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_storm[0][2], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_storm[0][3], 4);
	sstr << "\n}},\n";
	sstr << "\n}},\n";

	sstr << "{{\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_storm[1][0], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_storm[1][1], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_storm[1][2], 4);
	sstr << "\n}},\n";
	sstr << "{{\n";
	print_eval_score_array<nb_ranks>(sstr, pawn_storm[1][3], 4);
	sstr << "\n}},\n";
	sstr << "\n}},\n";

	sstr << "\n}};\n";

	sstr << "\ninline constexpr std::array<EvalScore, nb_pieces_type> pieces_score = ";
	sstr << "{{\n";
	print_eval_score_array<nb_pieces_type>(sstr, pieces_score, 1);
	sstr << "\n}};\n";

	sstr << "\ninline constexpr EvalScore bishop_pair";
	print_eval_score(sstr, bishop_pair);

	sstr << "\ninline constexpr EvalScore doubled_pawn";
	print_eval_score(sstr, doubled_pawn);

	sstr << "\ninline constexpr EvalScore isolated_pawn";
	print_eval_score(sstr, isolated_pawn);

	sstr << "\ninline constexpr EvalScore backward_pawn";
	print_eval_score(sstr, backward_pawn);

	sstr << "\ninline constexpr EvalScore rook_open_file";
	print_eval_score(sstr, rook_open_file);

	sstr << "\ninline constexpr EvalScore rook_semi_open_file";
	print_eval_score(sstr, rook_semi_open_file);

	sstr << "\ninline constexpr EvalScore rook_on_7th";
	print_eval_score(sstr, rook_on_7th);

	sstr << "\ninline constexpr EvalScore blocked_pawn";
	print_eval_score(sstr, blocked_pawn);

	sstr << "\ninline constexpr EvalScore trapped_bishop";
	print_eval_score(sstr, trapped_bishop);

	sstr << "\ninline constexpr EvalScore trapped_knight";
	print_eval_score(sstr, trapped_knight);

	sstr << "\ninline constexpr EvalScore blocked_rook";
	print_eval_score(sstr, blocked_rook);
	
	sstr << "\ninline constexpr std::array<std::int32_t, nb_pieces_type> ";
	sstr << "king_attackers_weight";
	sstr << " =  {\n    ";
	for (auto i=0U; i<king_attackers_weight.size(); i++) {
		sstr << king_attackers_weight[i];
		if (i != (king_attackers_weight.size() - 1)) {
			sstr << ", ";
		}
	}
	sstr << "\n};\n";

	sstr << std::endl;	
	return sstr.str();
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEval::saveEval(const std::string &filename) const
{
	ConfigFile conf;
	conf.set("version", std::string(project_ver));

	for (const auto &param : parameters) {
		conf.set(param.name + "_mid", std::to_string(param.value.mid));
		conf.set(param.name + "_end", std::to_string(param.value.end));
	}

	return conf.save(filename);
}

////////////////////////////////////////////////////////////////////////////////
bool ChessEval::loadEval(const std::string &filename)
{
	ConfigFile conf;

	if (!conf.load(filename)) {
		Logger::info() << "file not found : '" << filename << "'";
		return false;
	}

	for (const auto &p : conf) {

		if (p.first.size() > 4) {
			auto strtype = p.first.substr(p.first.size() - 4);
			auto strname = p.first.substr(0, p.first.size() - 4);

			auto param = std::find_if(parameters.begin(), parameters.end(), 
				[strname](const Tunable &pa) {
					return (pa.name == strname);
			});

			if (param == parameters.end()) {	
				Logger::warning() << "unknown parameter : " << strname;
				continue;
			}

			if (ConfigFile::isNumber(p.second)) {
				if (strtype == "_mid") {
					param->value.mid = std::stoi(p.second);
					continue;
				}
				if (strtype == "_end") {
					param->value.end = std::stoi(p.second);
					continue;
				}
			}
		}
		Logger::error() << "bad parameter : " << p.first;
		return false;
	}

	return true;
}

} // namespace Cheese

