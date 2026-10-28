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

#include "endgame.h"

#include "bitbase.h"

namespace Cheese::ChessEndGames {

// endgames signatures
constexpr MaterialMask material_endgame_KNKN      = makeMaterialMask("KNKN");
constexpr MaterialMask material_endgame_KBKB      = makeMaterialMask("KBKB");
constexpr MaterialMask material_endgame_KBKN      = makeMaterialMask("KBKN");
constexpr MaterialMask material_endgame_KNKB      = makeMaterialMask("KNKB");
constexpr MaterialMask material_endgame_KKNN      = makeMaterialMask("KKNN");
constexpr MaterialMask material_endgame_KNNK      = makeMaterialMask("KNNK");

constexpr MaterialMask material_endgame_KBKNN 	  = makeMaterialMask("KBKNN");
constexpr MaterialMask material_endgame_KNKNN     = makeMaterialMask("KNKNN");
constexpr MaterialMask material_endgame_KNNKB     = makeMaterialMask("KNNKB");
constexpr MaterialMask material_endgame_KNNKN     = makeMaterialMask("KNNKN");
constexpr MaterialMask material_endgame_KNNKNN    = makeMaterialMask("KNNKNN");

constexpr MaterialMask material_endgame_KBKNB 	  = makeMaterialMask("KBKNB");
constexpr MaterialMask material_endgame_KNKNB     = makeMaterialMask("KNKNB");
constexpr MaterialMask material_endgame_KNBKB     = makeMaterialMask("KNBKB");
constexpr MaterialMask material_endgame_KNBKN     = makeMaterialMask("KNBKN");

constexpr MaterialMask material_endgame_KBKBB 	  = makeMaterialMask("KBKBB");
constexpr MaterialMask material_endgame_KNKBB     = makeMaterialMask("KNKBB");
constexpr MaterialMask material_endgame_KBBKB     = makeMaterialMask("KBBKB");
constexpr MaterialMask material_endgame_KBBKN     = makeMaterialMask("KBBKN");

constexpr MaterialMask material_endgame_KKP       = makeMaterialMask("KKP");
constexpr MaterialMask material_endgame_KPK       = makeMaterialMask("KPK");
constexpr MaterialMask material_endgame_KKR       = makeMaterialMask("KKR");
constexpr MaterialMask material_endgame_KRK       = makeMaterialMask("KRK");
constexpr MaterialMask material_endgame_KKQ       = makeMaterialMask("KKQ");
constexpr MaterialMask material_endgame_KQK       = makeMaterialMask("KQK");

// drawish endgames
constexpr MaterialMask material_endgame_KNKR      = makeMaterialMask("KNKR");
constexpr MaterialMask material_endgame_KRKN      = makeMaterialMask("KRKN");
constexpr MaterialMask material_endgame_KBKR      = makeMaterialMask("KBKR");
constexpr MaterialMask material_endgame_KRKB      = makeMaterialMask("KRKB");

constexpr MaterialMask material_endgame_KBKP      = makeMaterialMask("KBKP");
constexpr MaterialMask material_endgame_KPKB      = makeMaterialMask("KPKB");
constexpr MaterialMask material_endgame_KNKP      = makeMaterialMask("KNKP");
constexpr MaterialMask material_endgame_KPKN      = makeMaterialMask("KPKN");


// bonus to push our king near opponent king
const std::array<std::int32_t, max_square_distance> eval_king_to_king = {
	0, 0, 25, 20, 15, 10, 5, 0
};

// push ennemy king to border in endgames
const std::array<std::int32_t, nb_squares> king_to_border = {
   80,  70,  60,  50,  50,  60,  70,  80,
   70,  60,  40,  30,  30,  40,  60,  70,
   60,  40,  20,  10,  10,  20,  40,  60,
   50,  30,  10,   0,   0,  10,  30,  50,
   50,  30,  10,   0,   0,  10,  30,  50,
   60,  40,  20,  10,  10,  20,  40,  60,
   70,  60,  40,  30,  30,  40,  60,  70,
   80,  70,  60,  50,  50,  60,  70,  80
};

////////////////////////////////////////////////////////////////////////////////
template<Side side>
int evalKPK(const ChessBoard &board)
{
	BitBoard bb = board.bitboardPiece(side, pawn);
	if (bb) {
		Square sq = BitBoards::getFirstSquare(bb);

		// use only draw information from KPK table
		if constexpr (side == white) {
			if (BitBases::readKPK(board.side(), 
								  board.kingPosition(side), 
								  board.kingPosition(~side), 
								  sq) == 0) {
				// draw
				return 0;
			}
	 	} else {
			if (BitBases::readKPK(~board.side(), 
								  flipSquareRank(board.kingPosition(side)), 
								  flipSquareRank(board.kingPosition(~side)), 
								  flipSquareRank(sq)) == 0) {
				// draw
				return 0;
			}
		}

		// win				
		int v = pieces_value_end[pawn] +  
			    static_cast<int>(relSquareRank<side>(sq)) * 8;
				
		return (board.side() == side) ? v : -v;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
int evalKPK(Side side, const ChessBoard &board)
{
	return (side == white) ? evalKPK<white>(board) : evalKPK<black>(board);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
int evalKRK(const ChessBoard &board)
{
	int v = pieces_value_end[rook] +
			king_to_border[board.kingPosition(~side)] +
			eval_king_to_king[squareDistance(board.kingPosition(side),
				board.kingPosition(~side))];
	return (board.side() == side) ? v : -v;
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
int evalKQK(const ChessBoard &board)
{
	int v = pieces_value_end[queen] +
			king_to_border[board.kingPosition(~side)] +
			eval_king_to_king[squareDistance(board.kingPosition(side),
				board.kingPosition(~side))];
	return (board.side() == side) ? v : -v;
}

////////////////////////////////////////////////////////////////////////////////
// evaluate known endgames
int evalEndGames(const ChessBoard &board, int &eval, int &scale)
{
	scale = max_draw_scale;

	// draw by insufficient material
	if (board.evaluateDraw()) {
		eval = draw_value;
		return 1;
	}

	// TODO : wrong color bishop

	// known endgames
	switch (board.materialCount()) {

		// KPK
		case	material_endgame_KPK:
				if constexpr (BitBases::use_bitbase) {
					eval = evalKPK<white>(board);
					return 1;
				} else {
					return 0;
				}

		case	material_endgame_KKP:
				if constexpr (BitBases::use_bitbase) {
					eval = evalKPK<black>(board);
					return 1;
				} else {
					return 0;
				}

		// KRK
		case	material_endgame_KRK:
				eval = evalKRK<white>(board);
				return 1;

		case	material_endgame_KKR:
				eval = evalKRK<black>(board);
				return 1;

		// KQK
		case	material_endgame_KQK:
				eval = evalKQK<white>(board);
				return 1;

		case	material_endgame_KKQ:
				eval = evalKQK<black>(board);
				return 1;

		// easy draw
		case	material_endgame_KNKN:
		case	material_endgame_KBKB:
		case	material_endgame_KNKB:
		case	material_endgame_KBKN:
		case	material_endgame_KNNK:
		case	material_endgame_KKNN:
				eval = draw_value;
				return 1;

		case	material_endgame_KRKN:
		case	material_endgame_KRKB:
		case	material_endgame_KBKR:
		case	material_endgame_KNKR:
				scale = 8;
				return 0;

		case	material_endgame_KBKNN:
		case	material_endgame_KNKNN:
		case	material_endgame_KNNKB:
		case	material_endgame_KNNKN:

		case	material_endgame_KNBKN:
		case	material_endgame_KNBKB:
		case	material_endgame_KNKNB:
		case	material_endgame_KBKNB:

		case	material_endgame_KNNKNN:
				scale = 2;
				return 0;

		case	material_endgame_KBBKN:
		case	material_endgame_KBBKB:
		case	material_endgame_KNKBB:
		case	material_endgame_KBKBB:
				scale = 4;
				return 0;

		case	material_endgame_KBKP:
		case	material_endgame_KNKP:
		case	material_endgame_KPKB:
		case	material_endgame_KPKN:
				scale = 8;
				return 0;

		default:
				return 0;
	}
}

} // namespace Cheese::ChessEndGames
