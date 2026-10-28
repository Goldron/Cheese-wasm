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

#ifndef CHEESE_BITBASE_H_
#define CHEESE_BITBASE_H_

#include "config.h"

#include "move.h"

namespace Cheese::BitBases {

// use bitbase in search and eval
inline constexpr bool use_bitbase = true;

// generate bitbase data
inline constexpr bool generate_bitbase = false;

void generateKPK();

[[nodiscard]]
int readKPK(Side side, Square kw, Square kb, Square p);

} // namespace Cheese::BitBases

#endif //CHEESE_BITBASE_H_
