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

#ifndef CHEESE_MATERIAL_H_
#define CHEESE_MATERIAL_H_

#include "config.h"

#include "move.h"

namespace Cheese {

using MaterialMask = std::uint64_t;

// number of bits by piece for MaterialMask
inline constexpr unsigned int max_material_mask_bits = 4;

// maximum count of pieces in MaterialMask
inline constexpr unsigned int max_material_mask_count = 
	(1 << max_material_mask_bits) - 1;

[[nodiscard]]
constexpr auto shiftMaterialMask(Side s, Piece p)
{
	return static_cast<unsigned int>(((s == black) ? 
		p : (p + piece_color_offset)) * max_material_mask_bits);
}

[[nodiscard]]
constexpr auto makeMaterialMask(Side s, Piece p, unsigned int count = 1)
{
	return static_cast<MaterialMask>(count) << shiftMaterialMask(s, p);
}

// create Material Mask from string
[[nodiscard]]
constexpr auto makeMaterialMask(const std::string_view &str)
{
	constexpr std::string_view maskPiecesNames { " PNBRQK" };
	MaterialMask m = 0ULL;
	Side s = black;
	for (auto c : str) {
		auto p = maskPiecesNames.find(c);
		if (p != std::string_view::npos) {
			if (c != 'K') {
				m += makeMaterialMask(s, static_cast<Piece>(p));
			} else {
				s = ~s;
			}
		}
	}
	return m;
}

[[nodiscard]]
constexpr int countMaterial(MaterialMask m, Side s, Piece p)
{
	return static_cast<int>(((m >> shiftMaterialMask(s, p)) & 
		static_cast<MaterialMask>(max_material_mask_count)));
}

// test if there are pawns on the position
[[nodiscard]]
constexpr bool havePawns(MaterialMask m)
{
	constexpr auto mask = 
		makeMaterialMask(black, pawn, max_material_mask_count) |
		makeMaterialMask(white, pawn, max_material_mask_count);
	return (m & mask) != 0ULL;
}

inline constexpr std::array<std::array<MaterialMask, nb_pieces_type>, nb_sides>
	material_count_mask = {{
	{
	  0ULL,
	  makeMaterialMask(black, pawn),
	  makeMaterialMask(black, knight),
	  makeMaterialMask(black, bishop),
	  makeMaterialMask(black, rook),
	  makeMaterialMask(black, queen),
	  0ULL,
	  0ULL,
	},
	{
	  0ULL,
	  makeMaterialMask(white, pawn),
	  makeMaterialMask(white, knight),
	  makeMaterialMask(white, bishop),
	  makeMaterialMask(white, rook),
	  makeMaterialMask(white, queen),
	  0ULL,
	  0ULL,
	}
}};

} // namespace Cheese

#endif //CHEESE_MATERIAL_H_

