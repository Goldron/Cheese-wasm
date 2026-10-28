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

#ifndef CHEESE_MOVEGEN_H_
#define CHEESE_MOVEGEN_H_

#include "config.h"

#include "board.h"
#include "move.h"
#include "movelist.h"

namespace Cheese {

// move generation template parameter for promotions
enum class GenPromotion {
	all,
	queen,
	under
};

////////////////////////////////////////////////////////////////////////////////
// Chess moves generation
namespace ChessMoveGen {

	// generate non capture moves (and no promotion)
	void generateQuiets(const ChessBoard &board, ChessMoveSortList &moves);

	// generate captures moves + promotion moves
	void generateCaptures(const ChessBoard &board, ChessMoveSortList &moves);

	void generateQuietCaptures(const ChessBoard &board,
		ChessMoveSortList &moves);

	// generate all legal moves when king must evade check
	void generateKingEvasions(const ChessBoard &board, 
		ChessMoveSortList &moves);

	void generateQuietKingEvasions(const ChessBoard &board, 
		ChessMoveSortList &moves);

	// Generate non capture moves that give check
	void generateQuietChecks(const ChessBoard &board, ChessMoveSortList &moves);

	// generate all legal moves for this position
	void generateMoves(const ChessBoard &board, ChessMoveSortList &moves,
		bool incheck);

	// count legal moves
	[[nodiscard]]
	int countLegalMoves(ChessBoard &board);

	// generate legal moves
	void generateLegalMoves(ChessBoard &board, ChessMoveSortList &moves);
}

} // namespace Cheese

#endif //CHEESE_MOVEGEN_H_
