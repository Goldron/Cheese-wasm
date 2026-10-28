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

#include "moveorder.h"

#include "engine.h"
#include "util/logfile.h"

#include <array>

namespace Cheese {

// sort moves values
const ChessMoveSort::value_type sortmove_capture_good = 80000;
const ChessMoveSort::value_type sortmove_killer1 = 70001;
const ChessMoveSort::value_type sortmove_killer2 = 70002;
//const ChessMoveSort::value_type sortmove_counter = 70003;
const ChessMoveSort::value_type sortmove_history = 4000;
const ChessMoveSort::value_type sortmove_default = 0;

// MVV-LVA table [captured][moved]
constexpr auto mvv_lva = []() {
		
	constexpr std::array<std::int32_t, nb_pieces_type> mvv_lva_pieces =
		{ 0, 1, 3, 3, 5, 9, 32 };

	constexpr std::int32_t scale_mvv_lva = 64;

	std::array<std::array<std::int32_t, nb_pieces_type>, nb_pieces_type> 
		table {};
	for (int y=0; y<nb_pieces_type; y++) {
		for (int x=0; x<nb_pieces_type; x++) {
			table[y][x] = (((y >= pawn) && (y <= king)) &&
						   ((x >= pawn) && (x <= king))) ?
				mvv_lva_pieces[y] * scale_mvv_lva - mvv_lva_pieces[x] : 0;
		}
	}	

	return table;
} ();

////////////////////////////////////////////////////////////////////////////////
ChessMoveOrder::ChessMoveOrder(ChessHistory &h, ChessMoveSortList &moves, 
	MoveOrderPhase ph, ChessMove mh, ChessMove mk1, ChessMove mk2, 
	ChessMove mc) : history(h), movelist(moves), phaseNext(ph), 
	count(moves.size()), moveHash(mh), moveKiller1(mk1), moveKiller2(mk2), 
	moveCounter(mc)
{
}

////////////////////////////////////////////////////////////////////////////////
template<MoveOrderPhase phase>
void ChessMoveOrder::orderMoves(const ChessBoard &board,
	ChessMoveSortList &moves, int pos, int nbmoves)
{
	static_assert((phase == MoveOrderPhase::init_capture) ||
		(phase == MoveOrderPhase::init_quiet) ||
		(phase == MoveOrderPhase::init_check) ||
		(phase == MoveOrderPhase::init_quiescence_capture) ||
		(phase == MoveOrderPhase::init_quiescence_capture_checks) ||
		(phase == MoveOrderPhase::init_quiescence_evasion));

	for (auto n=pos; n<(pos + nbmoves); ++n) {

		auto &[move, value] = moves[n];

		if constexpr ((phase == MoveOrderPhase::init_capture) ||
			(phase == MoveOrderPhase::init_check) ||
			(phase == MoveOrderPhase::init_quiescence_capture) ||
			(phase == MoveOrderPhase::init_quiescence_capture_checks) ||
			(phase == MoveOrderPhase::init_quiescence_evasion)) {

			Piece cp = move.capture();
			Piece pp = move.promotion();						
			if ((cp != no_piece) || (pp == queen)) {
				Piece mp = move.piece();
				value = sortmove_capture_good + mvv_lva[cp][mp];
				continue;
			}
		}
		
		if constexpr (phase == MoveOrderPhase::init_check) {
			if (move == moveKiller1) {
				value = sortmove_killer1;
				continue;
			}
			if (move == moveKiller2) {
				value = sortmove_killer2;
				continue;
			}
			//if (move == moveCounter) {
			//	value = sortmove_counter;
			//	continue;
			//}
		}

		if constexpr ((phase == MoveOrderPhase::init_quiet) ||
			(phase == MoveOrderPhase::init_check) ||
			(phase == MoveOrderPhase::init_quiescence_evasion)) {
			value = sortmove_history + history.get(move, board.side());
			continue;
		}

		value = sortmove_default;
	}

	moves.sort(pos, nbmoves);
}

////////////////////////////////////////////////////////////////////////////////
ChessMove ChessMoveOrder::getNextMove(const ChessBoard &board)
{
	switch (phaseNext) {

		case	MoveOrderPhase::hash:
				if (board.validMove(moveHash)) {
					phase = phaseNext++;
					return moveHash;
				}
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::init_capture:					
				position = 0;
				pos_capture = 0;
				movelist.clear();
				ChessMoveGen::generateCaptures(board, movelist);
				count = movelist.size();
				orderMoves<MoveOrderPhase::init_capture>(board, movelist, 
					0, count);
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::next_capture:
				while (position < count) {
					ChessMove move = movelist[position++].move;
					if (move != moveHash) {
						if ((piece_value_see[move.capture()] <
							 piece_value_see[move.piece()]) 
							&& (board.staticExchangeEvaluation(move) < 0)) {
							// use begining of the list for bad captures
							// pos_capture will never overwrite untested
							// captures
							movelist[pos_capture++].move = move;
						} else {
							phase = phaseNext;		
							return move;
						}
					}
				}
				++phaseNext;
				
				[[fallthrough]];

		case	MoveOrderPhase::killer1:					
				if ((moveKiller1 != moveHash) &&
					(board.validMove(moveKiller1))) {
					phase = phaseNext++;
					return moveKiller1;
				}
				++phaseNext;
				
				[[fallthrough]];

		case	MoveOrderPhase::killer2:
				if ((moveKiller2 != moveHash) &&
					(board.validMove(moveKiller2))) {
					phase = phaseNext++;
					return moveKiller2;
				}
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::countermove:
				if ((moveCounter != moveHash) &&
					(moveCounter != moveKiller1) &&
					(moveCounter != moveKiller2) &&
					(board.validMove(moveCounter))) {
					phase = phaseNext++;
					return moveCounter;
				}
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::init_quiet:					
				ChessMoveGen::generateQuiets(board, movelist);
				orderMoves<MoveOrderPhase::init_quiet>(board, movelist, 
					count, movelist.size() - count);
				count = movelist.size();
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::next_quiet:
				while (position < count) {
					ChessMove move = movelist[position++].move;
					if ((move != moveHash)
						&& (move != moveKiller1)
						&& (move != moveKiller2)
						&& (move != moveCounter)) {
						phase = phaseNext;
						return move;
					}
				}
				position = 0;
				count = pos_capture;
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::next_bad_capture:
				if (position < count) {
					ChessMove move = movelist[position++].move;
					phase = phaseNext;
					return move;
				}
				break;

		case	MoveOrderPhase::hash_check:					
				if (board.validMove(moveHash)) {
					phase = phaseNext++;
					return moveHash;
				}
				++phaseNext;
				
				[[fallthrough]];

		case	MoveOrderPhase::init_check:					
				position = 0;
				movelist.clear();
				ChessMoveGen::generateKingEvasions(board, movelist);
				count = movelist.size();
				orderMoves<MoveOrderPhase::init_check>(board, movelist, 
					0, count);
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::next_check:
				while (position < count) {
					ChessMove move = movelist[position++].move;
					if (move != moveHash) {
						phase = phaseNext;
						return move;
					}
				} 
				break;

		case	MoveOrderPhase::init_quiescence_capture:
				position = 0;
				movelist.clear();
				ChessMoveGen::generateQuietCaptures(board, movelist);
				count = movelist.size();
				orderMoves<MoveOrderPhase::init_quiescence_capture>(board, 
					movelist, 0, count);
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::next_quiescence_capture:
				while (position < count) {
					ChessMove move = movelist[position].move;
					++position;
					if ((piece_value_see[move.capture()] >=
						 piece_value_see[move.piece()]) ||
						(board.staticExchangeEvaluation(move) >= 0)) {
						phase = phaseNext;
						return move;
					}
				} 
				break;

		case	MoveOrderPhase::init_quiescence_capture_checks:
				position = 0;
				movelist.clear();
				ChessMoveGen::generateQuietCaptures(board, movelist);
				count = movelist.size();
				orderMoves<MoveOrderPhase::init_quiescence_capture_checks>(
					board, movelist, 0, count);
				++phaseNext;

				[[fallthrough]];

		case	MoveOrderPhase::next_quiescence_capture_checks:

				while (position < count) {
					ChessMove move = movelist[position].move;
					++position;
					if ((piece_value_see[move.capture()] >=
						 piece_value_see[move.piece()]) ||
						(board.staticExchangeEvaluation(move) >= 0)) {
						phase = phaseNext;
						return move;
					}
				} 
				++phaseNext;
			
				[[fallthrough]];

		case	MoveOrderPhase::init_quiescence_check:
				position = 0;
				movelist.clear();
				ChessMoveGen::generateQuietChecks(board, movelist);
				count = movelist.size();
				++phaseNext;

				[[fallthrough]];
				
		case	MoveOrderPhase::next_quiescence_check:
				while (position < count) {
					ChessMove move = movelist[position++].move;
					if (board.staticExchangeEvaluation(move) >= 0) {
						phase = phaseNext;
						return move;
					}
				}
				break;

		case	MoveOrderPhase::init_quiescence_evasion:
				position = 0;
				movelist.clear();
				ChessMoveGen::generateQuietKingEvasions(board, movelist);
				count = movelist.size();
				orderMoves<MoveOrderPhase::init_quiescence_evasion>(
					board, movelist, 0, count);
				++phaseNext;

			    [[fallthrough]];

		case	MoveOrderPhase::next_quiescence_evasion:
				if (position < count) {
					phase = phaseNext;
					return movelist[position++].move;
				}
				break;

		default:
			break;
	}

	phaseNext = MoveOrderPhase::end;
	phase = phaseNext;
	return ChessMove(0);
}

} // namespace Cheese
