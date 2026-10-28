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

#include "bitbase.h"
#include "bitbasekpk.h"
#include "util/logfile.h"
#include "util/timer.h"

#include <array>
#include <iomanip>
#include <iostream>

namespace Cheese::BitBases {

using bitbase_type = std::uint32_t;

////////////////////////////////////////////////////////////////////////////////
enum BitBaseScore {
	kpk_draw,
	kpk_win,
	kpk_invalid,
	kpk_unknown,
	kpk_loss
};

// uncompressed kpk table size
constexpr auto total_kpk = static_cast<bitbase_type>(nb_sides * nb_squares * 
	nb_squares * nb_squares);

// compressed kpk table size
constexpr auto total_bit_kpk = static_cast<bitbase_type>((nb_sides * 
	(nb_squares / 2) * nb_squares * nb_squares) / 32);

////////////////////////////////////////////////////////////////////////////////
constexpr void decodeIndexKPK(bitbase_type i, Side &s, Square &p, Square &kw, 
	Square &kb)
{
	s = static_cast<Side>((i >> 18) & 1);
	p = static_cast<Square>((i >> 12) & 0x3f);
	kw = static_cast<Square>((i >> 6) & 0x3f);
	kb = static_cast<Square>(i & 0x3f);
}

////////////////////////////////////////////////////////////////////////////////
constexpr auto encodeIndexKPK(Side s, Square p, Square kw, Square kb)
{
	return (static_cast<bitbase_type>(s) << 18) |
		   (static_cast<bitbase_type>(p) << 12) |
		   (static_cast<bitbase_type>(kw) << 6) |
		   static_cast<bitbase_type>(kb);
}

////////////////////////////////////////////////////////////////////////////////
constexpr auto encodeIndexBitKPK(Side s, Square p, Square kw, Square kb)
{
	return (static_cast<bitbase_type>(s) << 17) |
		   (static_cast<bitbase_type>(p) << 12) |
		   (static_cast<bitbase_type>(kw) << 6) |
		   static_cast<bitbase_type>(kb);
}

////////////////////////////////////////////////////////////////////////////////
// a step in kpk table generation
bool update_kpk(Side side, Square p, Square kw, Square kb, 
	std::array<char, total_kpk> &kpk)
{
	bitbase_type index = encodeIndexKPK(side, p, kw, kb);

	bitbase_type adr {0};
	bool changed {false};

	if (kpk[index] == kpk_unknown) {

		std::array<int, 5> countpos {};
		int count = 0;

		// generate all successor positions (legal moves)
		if (side == white) {
			// pawn move 1
			if (squareRank(p) < rank_8) {
				adr = encodeIndexKPK(~side , p + dir_up, kw, kb);
				if (kpk[adr] != kpk_invalid) {
					++countpos[kpk[adr]];
					++count;
				}
			}

			// pawn move 2
			if (squareRank(p) == rank_2) {
				adr = encodeIndexKPK(~side, p + dir_up_2, kw, kb);
				if (((p + dir_up) != kw) && 
					((p + dir_up) != kb) &&
					(kpk[adr] != kpk_invalid)) {
					++countpos[kpk[adr]];
					++count;
				}
			}

			// move white king
			for (Rank y=squareRank(kw)-1; y<=squareRank(kw)+1; ++y) {
				for (File x=squareFile(kw)-1; x<=squareFile(kw)+1; ++x) 
				{
					Square sq = makeSquare(x, y);
					adr = encodeIndexKPK(~side, p, sq, kb);
					if ((x >= file_a) && (x <= file_h) &&
						(y >= rank_1) && (y <= rank_8) &&
						(sq != kw) &&
						(kpk[adr] != kpk_invalid)) {

						++countpos[kpk[adr]];
						++count;
					}
				}
			}
		} else {
			// move black king
			for (Rank y=squareRank(kb)-1; y<=squareRank(kb)+1; ++y) {
				for (File x=squareFile(kb)-1; x<=squareFile(kb)+1; ++x) 
				{
					Square sq = makeSquare(x, y);
					adr = encodeIndexKPK(~side, p, kw, sq);
					if ((x >= file_a) && (x <= file_h) &&
						(y >= rank_1) && (y <= rank_8) &&
						(sq != kb) &&
						(kpk[adr] != kpk_invalid)) {

						++countpos[kpk[adr]];
						++count;
					}
				}
			}
		}

		if (count != 0) {
			// if any successor is LOSS, set to WIN
			// if all successor are WIN, set to LOSS
			// if all successor are DRAW, set to DRAW
			if (countpos[kpk_loss] != 0) {
				kpk[index] = kpk_win;
				changed = true;
			} else
			if (countpos[kpk_win] == count) {
				kpk[index] = kpk_loss;
				changed = true;
			} else
			if (countpos[kpk_draw] == count) {
				kpk[index] = kpk_draw;
				changed = true;
			}
		}
	}

	return changed;
}

////////////////////////////////////////////////////////////////////////////////
// generate KPK table
//
// count iteration = 20
// kpk win         = 124960 (23.8%)
// kpk loss        = 123574 (23.6%)
// kpk draw        = 110778 (21.1%)
// kpk invalid     = 164976 (31.5%)
//
// index format : 32 bits / 19 bits used
// ........ .....x.. ........ ........ : side (0 = black, 1 = white)
// ........ ......xx xxxx.... ........ : white pawn square
// ........ ........ ....xxxx xx...... : white king square
// ........ ........ ........ ..xxxxxx : black king square
// 00000000 00000... ........ ........ : unused
//
void make_kpk_table(std::array<char, total_kpk> &kpk) {
	
	kpk.fill(kpk_unknown);

	Square kw = a1;
	Square kb = a1;
	Square p = a1;
	Side side = black;

	// invalid positions
	// + set known mate and draw positions
	for (bitbase_type i=0; i<total_kpk; i++) {

		decodeIndexKPK(i, side, p, kw, kb);

		// impossible positions
		if ((kw == p) || (kb == p) ||
			(p < a2) ||
			((p > h7) && (side == white)) ||
			(squareDistance(kw, kb) < 2)) {
			kpk[i] = kpk_invalid;
		}

		if (side == white) {
			// white pawn attack black king
			if ((squareRank(kb) == (squareRank(p) + 1)) &&
				((squareFile(kb) == (squareFile(p) + 1)) ||
				(squareFile(kb) == (squareFile(p) - 1)))) {
				kpk[i] = kpk_invalid;
			}
		}

		if (kpk[i] != kpk_invalid) {

			if (side == black) {

				// if the pawn is taken it's a draw
				if ((squareDistance(kb, p) == 1) &&
					(squareDistance(kw, p) > 1)) {
					kpk[i] = kpk_draw;
				}

				// promoted pawn is loss for black
				if ((squareRank(p) == rank_8) &&
					((squareDistance(kb, p) > 1) ||
						((squareDistance(kb, p) == 1) &&
						(squareDistance(kw, p) == 1)))) {

					kpk[i] = kpk_loss;
				}
			}
		}
	}

	// repeat until no changes are possible
	bool changed = true;
	while (changed) {

		changed = false;

		// better to test all black moves, then white
		for (int s=1; s>=0; s--) {

			side = static_cast<Side>(s);

			for (p=a1; p<=h8; ++p) {
				for (kw=a1; kw<=h8; ++kw) {
					for (kb=a1; kb<=h8; ++kb) {
						if (update_kpk(side, p, kw, kb, kpk)) {
							changed = true;
						}
					}
				}
			}
		}
	}

	// end : set all unknown to draw
	for (bitbase_type i=0; i<total_kpk; i++) {
		if (kpk[i] == kpk_unknown) {
			kpk[i] = kpk_draw;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// generate compressed kpk bitbase table
void generateKPK()
{
	if constexpr (generate_bitbase) {

		std::array<bitbase_type, total_bit_kpk> bitbase_table {};

		std::array<char, total_kpk> kpk {};
		make_kpk_table(kpk);

		Square kw = a1;
		Square kb = a1;
		Square p = a1;
		Side side = black;

		for (bitbase_type i=0; i<total_kpk; i++) {

			if (kpk[i] != kpk_invalid) {

				decodeIndexKPK(i, side, p, kw, kb);

				if (squareFile(p) <= file_d) {

					p = static_cast<Square>(
						(static_cast<unsigned int>(squareRank(p)) << 2) | 
						static_cast<unsigned int>(squareFile(p)));

					bitbase_type adrbit = encodeIndexBitKPK(side, p, 
						kw, kb);
					bitbase_type adrbyte = adrbit >> 5;
					bitbase_type posbit = adrbit & 31;
					bitbase_type mask = 1 << posbit;

					if ((kpk[i] == kpk_win) || (kpk[i] == kpk_loss)) {
						bitbase_table[adrbyte] |= mask;
					}
				}
			}
		}

		// output
		for (std::uint32_t i=0; i<total_bit_kpk; i++) {
			std::cout << "0x" << std::setfill('0') << std::setw(8) 
				<< std::hex << bitbase_table[i] << ", ";
			if ((i % 8) == 7) {
				std::cout << std::endl;	
			}
		}

		std::array<std::size_t, 5> countKPK {};

		for (bitbase_type i=0; i<total_kpk; i++) {
			++countKPK[kpk[i]];
		}

		auto func_percent = [](std::size_t v, std::size_t m) {
			return static_cast<double>(v) * 100.0 / static_cast<double>(m);
		};

		Logger::info() << "kpk total = " << total_kpk;
		Logger::info() << "count bytes = " << total_bit_kpk * 4;

		Logger::info() << "kpk win = " << countKPK[kpk_win] << " "
			<< func_percent(countKPK[kpk_win], total_kpk);
		Logger::info() << "kpk loss = " << countKPK[kpk_loss] << " "
			<< func_percent(countKPK[kpk_loss], total_kpk);
		Logger::info() << "kpk draw = " << countKPK[kpk_draw] << " "
			<< func_percent(countKPK[kpk_draw], total_kpk);
		Logger::info() << "kpk invalid = " << countKPK[kpk_invalid] << " "
			<< func_percent(countKPK[kpk_invalid], total_kpk);
		Logger::info() << "kpk unknown = " << countKPK[kpk_unknown] << " "
			<< func_percent(countKPK[kpk_unknown], total_kpk);
	}
}

////////////////////////////////////////////////////////////////////////////////
int readKPK(Side side, Square kw, Square kb, Square p)
{
	// pawn symmetry
	if (squareFile(p) > file_d) {
		kw = flipSquareFile(kw);
		kb = flipSquareFile(kb);
		p = flipSquareFile(p);
	}

	// recalc pawn square offset with the symmetry
	// because file now takes 2 bits
	p = static_cast<Square>((static_cast<unsigned int>(squareRank(p)) 
		<< 2) | static_cast<unsigned int>(squareFile(p)));

	bitbase_type adrbit = encodeIndexBitKPK(side, p, kw, kb);
	bitbase_type adrbyte = adrbit >> 5;
	bitbase_type posbit = adrbit & 31;
	bitbase_type mask = 1 << posbit;

	return (bitbase_kpk[adrbyte] & mask) ? 1 : 0;
}

} // namespace Cheese::BitBases

