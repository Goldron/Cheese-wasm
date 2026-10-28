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

#ifndef CHEESE_FEN_H_
#define CHEESE_FEN_H_

#include "config.h"

#include "board.h"

#include <string>
#include <vector>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
// FEN (Forsythe-Edwards Notation)
//
// default position :
//		rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
//

////////////////////////////////////////////////////////////////////////////////
class ChessFENParser {

public:

	ChessFENParser() = default;

	// set position from FEN string
	// note : don't allow extra spaces, don't test maximum pieces count
	[[nodiscard]]
	static bool set(const std::string_view &fen, ChessBoard &pos);

	// get FEN string from position
	[[nodiscard]]
	static std::string get(const ChessBoard &pos);

	[[nodiscard]]
	static bool reverse(const std::string_view &str, std::string &newfen);

	[[nodiscard]]
	static bool load(const std::string &filename, 
		std::vector<std::string> &fens);

	[[nodiscard]]
	static std::string getOption(const std::string &fen, 
		const std::string &name);

	[[nodiscard]]	
	static bool parseNumber(const std::string_view &str, int &v);

	[[nodiscard]]
	static bool parseEnPassant(const std::string_view &str, Side side, int &ep);

	[[nodiscard]]
	static bool parseCastling(const std::string_view &str, Square kingsq, 
		CastlingRight &cst);

	[[nodiscard]]
	static bool parseBoard(const std::string_view &str, 
		std::array<int, nb_squares> &pieces, 
		std::array<Square, nb_sides> &kingpos);
};

} // namespace Cheese

#endif //CHEESE_FEN_H_
