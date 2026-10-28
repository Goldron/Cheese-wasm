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

#ifndef CHEESE_TABLEBASES_H_
#define CHEESE_TABLEBASES_H_

#include "config.h"

#include "board.h"
#include "movegen.h"

#include <array>

#ifdef USE_TABLEBASES
inline constexpr bool use_tablebases = true;
#else
inline constexpr bool use_tablebases = false;
#endif

// Interface to use Fathom tablebases

namespace Cheese::TableBases {

// initialize tablebases
void init(const std::string &filename);

// free tablebases
void free();

// probe
[[nodiscard]]
int probe_wdl(const ChessBoard &board, int &score);

// probe at root
[[nodiscard]]
int probe_root(const ChessBoard &board, int &score, ChessMoveSortList &moves, 
	bool allmoves = false);

// get max syzygy men (0 if not loaded)
[[nodiscard]]
int largest();

// convert WDL score to string
[[nodiscard]]
std::string wdl_to_string(int r);

} // namespace Cheese::TableBases

#endif //CHEESE_TABLEBASES_H_

