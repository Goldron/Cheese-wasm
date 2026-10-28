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

#include "tablebases.h"

#include "util/logfile.h"

#ifdef USE_TABLEBASES
#include "fathom/tbprobe.h"
#endif

#include <string>

namespace Cheese::TableBases {

#ifdef USE_TABLEBASES

// convert Fathom promotion piece to Cheese format
const std::array<Piece, 6> promotions_fathom = { 
	no_piece, queen, rook, bishop, knight, no_piece 
};

// wdl result : loss, blessed_loss, draw, cursed_win, win
const std::array<int, 5> score_tb = { 
	-2, -1, 0, 1, 2
};

////////////////////////////////////////////////////////////////////////////////
// convert Fathom move to Cheese
ChessMove convert_move(const ChessBoard &board, unsigned r)
{
	auto src = static_cast<Square>(TB_GET_FROM(r));
	auto dst = static_cast<Square>(TB_GET_TO(r));
	auto pro = promotions_fathom[TB_GET_PROMOTES(r)];
	int ep = static_cast<int>(TB_GET_EP(r));
	auto mv = board.piece(src);
	auto cap = board.piece(dst); 

	return ChessMove(src, dst, mv, cap, pro, ep);
}

////////////////////////////////////////////////////////////////////////////////
int largest()
{
	return static_cast<int>(TB_LARGEST);
}

////////////////////////////////////////////////////////////////////////////////
void init(const std::string &filename)
{
	if (!tb_init(filename.c_str())) {
		Logger::info() << "no tablebases found : " << filename;
	} else {
		if (TB_LARGEST != 0) {
			Logger::info() << "init tablebases from " << filename;
			Logger::info() << "tablebase largest = " << TB_LARGEST;	
		} else {
			Logger::info() << "tablebases not used";
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void free()
{
	if (TB_LARGEST != 0) {
		Logger::info() << "free tablebases";
	}

	tb_free();
}

////////////////////////////////////////////////////////////////////////////////
int probe_wdl(const ChessBoard &board, int &score)
{
	if (TB_LARGEST == 0) {
		return -1;
	}

	score = 0;

	unsigned r = tb_probe_wdl(
			board.bitboardColor(white),
			board.bitboardColor(black),
			board.bitboardPieceType(king),
			board.bitboardPieceType(queen),
			board.bitboardPieceType(rook),
			board.bitboardPieceType(bishop),
			board.bitboardPieceType(knight),
			board.bitboardPieceType(pawn),
			static_cast<unsigned>(board.fiftyRule()),
			board.canCastle(castle_init) ? 1 : 0,
			board.isEnPassant() ? board.enPassantSquare(board.side()) : 0,
			(board.side() == white)
		);

	if (r == TB_RESULT_FAILED) {
		return 0;
	}

	unsigned wdl = TB_GET_WDL(r);

	score = score_tb[wdl];

	return 1;
}

////////////////////////////////////////////////////////////////////////////////
int probe_root(const ChessBoard &board, int &score, ChessMoveSortList &moves, 
	bool allmoves)
{
	if (TB_LARGEST == 0) {
		return -1;
	}

	score = 0;

	unsigned results[TB_MAX_MOVES];

	unsigned r = tb_probe_root(
			board.bitboardColor(white),
			board.bitboardColor(black),
			board.bitboardPieceType(king),
			board.bitboardPieceType(queen),
			board.bitboardPieceType(rook),
			board.bitboardPieceType(bishop),
			board.bitboardPieceType(knight),
			board.bitboardPieceType(pawn),
			static_cast<unsigned>(board.fiftyRule()),
			board.canCastle(castle_init) ? 1 : 0,
			board.isEnPassant() ? board.enPassantSquare(board.side()) : 0,
			(board.side() == white),
			results	
		);

	if (r == TB_RESULT_FAILED) {
		return 0;
	}

	unsigned wdl = TB_GET_WDL(r);
	score = score_tb[wdl];

	moves.clear();

	if (allmoves) {
		wdl = 0;
	}

	ChessMoveSort ms;
	int i=0;
	while ((i < TB_MAX_MOVES) && (results[i] != TB_RESULT_FAILED)) {
		auto m = convert_move(board, results[i]);
		unsigned s = TB_GET_WDL(results[i]);
		if (s >= wdl) {
			ms.move = m;
			ms.value = static_cast<std::int32_t>(score_tb[s]);
			moves.push(ms);
		}
		++i;
	}	

	return 1;
}

#else // USE_TABLEBASES

////////////////////////////////////////////////////////////////////////////////
// initialize tablebases
void init(const std::string &filename)
{
}

////////////////////////////////////////////////////////////////////////////////
// free tablebases
void free()
{
}

////////////////////////////////////////////////////////////////////////////////
// probe
int probe_wdl(const ChessBoard &board, int &score)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// probe at root
int probe_root(const ChessBoard &board, int &score, ChessMoveSortList &moves, 
	bool allmoves)
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// get max syzygy men (0 if not loaded)
int largest()
{
	return 0;
}

#endif // USE_TABLEBASES

////////////////////////////////////////////////////////////////////////////////
std::string wdl_to_string(int r)
{
	switch (r) {
		case -2:
			return "loss";
			break;
		case -1:
			return "blessed loss";
			break;
		case 0:
			return "draw";
			break;
		case 1:
			return "cursed win";
			break;
		case 2:
			return "win";
			break;
		default:
			return "unknown";
	}
}

} // namespace Cheese::TableBases

