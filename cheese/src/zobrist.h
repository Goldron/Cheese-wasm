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

// Zobrist 64 bits Hashing, using XorShift* for random number generation

#ifndef CHEESE_ZOBRIST_H_
#define CHEESE_ZOBRIST_H_

#include "config.h"

#include "move.h"

#include "util/xorshiftstar.h"

#include <array>
#include <string>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
// Zobrist keys used for hash tables
//
//	need :   pieces		: 2 * 6 * 64 = 768
//			 side		: 1
//			 enpassant	: 8
//			 castling	: 4 castling rights
//
//			 total		: 781

// 64 bits hash key used for zobrist hash
using HashKey = std::uint64_t;

////////////////////////////////////////////////////////////////////////////////
namespace Zobrist {

	inline constexpr std::size_t nb_castling_hash = 16;

	struct ZobristData {
		
		std::array<std::array<std::array<HashKey, nb_squares>,
			nb_pieces_type>, nb_sides> piece {};

		std::array<HashKey, nb_castling_hash> castling {};

		std::array<HashKey, nb_files + 1> enpassant {};

		HashKey side {};
	};

	// generate Zobrist keys at compile time
	inline constexpr auto zobrist_keys = []() {

		ZobristData data {};

		constexpr std::size_t nbk = nb_sides * 6 * nb_squares + 4 + 
			nb_files + 1;

		constexpr std::uint64_t seed = 0x09bb57623c3b583b;
		Random::XorShiftStar64 rng(seed);

		std::array<HashKey, nbk> tab {};

		for (std::size_t n=0; n<nbk; n++) {
			tab[n] = rng();
		}

		std::size_t ptab = 0;
		for (int co=0; co<nb_sides; co++) {
			for (int pt=0; pt<nb_pieces_type; pt++) {
				for (int sq=0; sq<nb_squares; sq++) {
					if ((pt >= pawn) && (pt <= king)) {
						data.piece[co][pt][sq] = tab[ptab++];
					}
				}
			}
		}

		HashKey key_white_short = tab[ptab++];
		HashKey key_white_long = tab[ptab++];
		HashKey key_black_short = tab[ptab++];
		HashKey key_black_long = tab[ptab++];

		for (std::size_t i=0; i<nb_castling_hash; i++) {
			auto c = static_cast<CastlingRight>(i);
			HashKey k = 0;
			if (c & castle_white_short) {
				k ^= key_white_short;
			}
			if (c & castle_white_long) {
				k ^= key_white_long;
			}
			if (c & castle_black_short) {
				k ^= key_black_short;
			}
			if (c & castle_black_long) {
				k ^= key_black_long;
			}
			data.castling[i] = k;
		}

		data.enpassant[0] = 0;
		for (int f=0; f<nb_files; f++) {
			data.enpassant[f + 1] = tab[ptab++];
		}

		data.side = tab[ptab++];

		return data;
	} ();

	[[nodiscard]]
	inline HashKey piece(Side s, Piece p, Square sq) {
		return zobrist_keys.piece[s][p][sq];
	}

	[[nodiscard]]
	inline HashKey castling(CastlingRight c) {
		return zobrist_keys.castling[c];
	}
	
	[[nodiscard]]
	inline HashKey enpassant(unsigned int f) {
		return zobrist_keys.enpassant[f];
	}

	[[nodiscard]]
	inline HashKey side() {
		return zobrist_keys.side;
	}

	// convert hashkey to string
	[[nodiscard]]
	std::string hashKeyToString(HashKey key);
}

} // namespace Cheese

#endif //CHEESE_ZOBRIST_H_

