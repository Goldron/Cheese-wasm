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

#ifndef CHEESE_ENDGAME_H_
#define CHEESE_ENDGAME_H_

#include "config.h"

#include "board.h"

namespace Cheese::ChessEndGames {

	inline constexpr int max_draw_scale = 32;

	// evaluate special endgames
	[[nodiscard]]
	int evalEndGames(const ChessBoard &board, int &eval, int &scale);

	// evaluate KPK endgame
	[[nodiscard]]
	int evalKPK(Side side, const ChessBoard &board);

} // namespace Cheese::ChessEndGames

#endif //CHEESE_ENDGAME_H_
