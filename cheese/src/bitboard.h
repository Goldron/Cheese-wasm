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

#ifndef CHEESE_BITBOARD_H_
#define CHEESE_BITBOARD_H_

#include "config.h"

#include "move.h"

#include <array>
#include <ostream>

#include <cassert>

#ifdef _MSC_VER
#ifdef SYSTEM64BIT
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(__popcnt64)
#else
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif
#endif

#ifdef USE_PEXT
#include <immintrin.h>
#endif

namespace Cheese {

// precalc magic bitboards mask, magic and shifts
inline constexpr bool find_magic_keys = false;

////////////////////////////////////////////////////////////////////////////////
// 8x8 BitBoard (64 bits)
using BitBoard = std::uint64_t;

namespace BitBoards {

extern const std::array<BitBoard, nb_squares> magic_rook_mask;
extern const std::array<BitBoard, nb_squares> magic_rook_magic;
extern std::array<BitBoard *, nb_squares> magic_rook_ptr;
extern const std::array<unsigned int, nb_squares> magic_rook_shift;
extern const std::array<BitBoard, nb_squares> magic_bishop_mask;
extern const std::array<BitBoard, nb_squares> magic_bishop_magic;
extern std::array<BitBoard *, nb_squares> magic_bishop_ptr;
extern const std::array<unsigned int, nb_squares> magic_bishop_shift;

// pawn attacks
extern const std::array<std::array<BitBoard, nb_squares>, nb_sides> pawn_attack;

// knight attacks
extern const std::array<BitBoard,nb_squares> knight_attack;

// king attacks
extern const std::array<BitBoard, nb_squares> king_attack;

extern const std::array<BitBoard, nb_squares> square;

// all diagonal 45 R
extern const std::array<BitBoard, nb_squares> diag_45r;

// all diagonal 45 L
extern const std::array<BitBoard, nb_squares> diag_45l;

// all squares between 2 squares
extern const std::array<std::array<BitBoard, nb_squares>, nb_squares> between;

// direction from a square to another
extern const std::array<std::array<std::int8_t, nb_squares>, nb_squares> 
	direction;

extern const std::array<BitBoard, nb_files> file;

extern const std::array<BitBoard, nb_ranks> rank;

extern const std::array<BitBoard, nb_files> files_around;

// all white squares
extern const BitBoard white_squares;

// all black squares
extern const BitBoard black_squares;

// bitboard attacks functions

////////////////////////////////////////////////////////////////////////////////
// compute knight attack
[[nodiscard]]
inline BitBoard	attackKnight(Square sq)
{
	return knight_attack[sq];
}

////////////////////////////////////////////////////////////////////////////////
// compute king attack
[[nodiscard]]
inline BitBoard	attackKing(Square sq)
{
	return king_attack[sq];
}

////////////////////////////////////////////////////////////////////////////////
// compute pawn attack
[[nodiscard]]
inline BitBoard	attackPawn(Side side, Square sq)
{
	return pawn_attack[side][sq];
}

#ifndef SYSTEM64BIT
////////////////////////////////////////////////////////////////////////////////
// compute sliding attack index in 32 bits
[[nodiscard]]
inline std::uint32_t magicAttack(BitBoard occ, BitBoard mask, 
	[[maybe_unused]] BitBoard magic,
	[[maybe_unused]] unsigned int shift)
{
	const BitBoard bb = occ & mask;
	std::uint32_t lo = static_cast<unsigned>(static_cast<std::int32_t>(bb) *
		static_cast<std::int32_t>(magic));
	std::uint32_t hi = static_cast<unsigned>(static_cast<std::int32_t>
		(bb >> 32) * static_cast<std::int32_t>(magic >> 32));
	return ((lo ^ hi) >> shift);
}
#else
////////////////////////////////////////////////////////////////////////////////
// compute sliding attack index in 64 bits
[[nodiscard]]
inline std::uint64_t magicAttack(BitBoard occ, BitBoard mask, 
	[[maybe_unused]] BitBoard magic,
	[[maybe_unused]] unsigned int shift)
{
#ifdef USE_PEXT
	return _pext_u64(occ, mask);
#else
	return ((occ & mask) * magic) >> shift;
#endif //USE_BITBOARD_PEXT
}
#endif // SYSTEM64BIT

////////////////////////////////////////////////////////////////////////////////
// compute rook attack
[[nodiscard]]
inline BitBoard	attackRook(Square sq, BitBoard occ)
{
	return *(magic_rook_ptr[sq] +
		magicAttack(occ, magic_rook_mask[sq],
			magic_rook_magic[sq],
			magic_rook_shift[sq]));
}

////////////////////////////////////////////////////////////////////////////////
// compute bishop attack
[[nodiscard]]
inline BitBoard	attackBishop(Square sq, BitBoard occ)
{
	return *(magic_bishop_ptr[sq] +
		magicAttack(occ, magic_bishop_mask[sq],
			magic_bishop_magic[sq],
			magic_bishop_shift[sq]));
}

////////////////////////////////////////////////////////////////////////////////
// compute queen attack
[[nodiscard]]
inline BitBoard	attackQueen(Square sq, BitBoard occ)
{
	return (attackRook(sq, occ) | attackBishop(sq, occ));
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr BitBoard shiftUp(BitBoard bb)
{
	return (side == white) ? (bb << dir_up) : (bb >> dir_up);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr BitBoard shiftUp2(BitBoard bb)
{
	return (side == white) ? (bb << dir_up_2) : (bb >> dir_up_2);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr BitBoard shiftUpLeft(BitBoard bb)
{
	return (side == white) ? (bb << dir_up_left) : (bb >> dir_up_left);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr BitBoard shiftUpRight(BitBoard bb)
{
	return (side == white) ? (bb << dir_up_right) : (bb >> dir_up_right);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr BitBoard allPawnsAttacksLeft(BitBoard bbpawns)
{
	constexpr auto fl = (side == white) ? file_h : file_a;
	return shiftUpLeft<side>(bbpawns) & ~file[fl];
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr BitBoard allPawnsAttacksRight(BitBoard bbpawns)
{
	constexpr auto fr = (side == white) ? file_a : file_h;
	return shiftUpRight<side>(bbpawns) & ~file[fr];
}

////////////////////////////////////////////////////////////////////////////////
// calculate attacks from all pawns
template<Side side>
[[nodiscard]]
constexpr BitBoard allPawnsAttacks(BitBoard bbpawns)
{
	return allPawnsAttacksLeft<side>(bbpawns) |
		   allPawnsAttacksRight<side>(bbpawns);
}

////////////////////////////////////////////////////////////////////////////////
// fill all bits up
[[nodiscard]]
inline BitBoard fillUp(BitBoard bb)
{
	BitBoard b = bb;
	b |= (b << 8);
	b |= (b << 16);
	b |= (b << 32);
	return b;
}

////////////////////////////////////////////////////////////////////////////////
// fill all bits down
[[nodiscard]]
inline BitBoard fillDown(BitBoard bb)
{
	BitBoard b = bb;
	b |= (b >> 8);
	b |= (b >> 16);
	b |= (b >> 32);
	return b;
}

////////////////////////////////////////////////////////////////////////////////
template<Side s>
[[nodiscard]]
constexpr BitBoard fillFront(BitBoard bb)
{
	return (s == white) ? fillUp(bb) : fillDown(bb);
}

////////////////////////////////////////////////////////////////////////////////
template<Side s>
[[nodiscard]]
constexpr BitBoard fillBack(BitBoard bb)
{
	return (s == white) ? fillDown(bb) : fillUp(bb);
}

////////////////////////////////////////////////////////////////////////////////
// tell if we have more than 1 bit set in the bitboard
// bb must not be empty !
[[nodiscard]]
constexpr bool bitCountMoreThanOne(BitBoard bb)
{
	return (bb & (bb - 1)) != 0ULL;
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline int bitCountSoft(BitBoard bb)
{
#ifdef SYSTEM64BIT
	static constexpr BitBoard k1 = 0x5555555555555555ULL;
	static constexpr BitBoard k2 = 0x3333333333333333ULL;
	static constexpr BitBoard k4 = 0x0f0f0f0f0f0f0f0fULL;
	static constexpr BitBoard kf = 0x0101010101010101ULL;

	BitBoard b =  bb - ((bb >> 1)  & k1);
    b = (b & k2) + ((b >> 2)  & k2);
    b = (b       +  (b >> 4)) & k4 ;
    b = (b * kf) >> 56;

	return static_cast<int>(b);
#else
	static constexpr BitBoard bitcount_m1 = 0x5555555555555555ULL;
	static constexpr BitBoard bitcount_m2 = 0x3333333333333333ULL;

	BitBoard a = bb - ((bb >> 1) & bitcount_m1);
	BitBoard c = (a & bitcount_m2) + ((a >> 2) & bitcount_m2);

	auto n = static_cast<std::uint32_t>(c) +
			 static_cast<std::uint32_t>(c >> 32);
	n = (n & 0x0F0F0F0F) + ((n >> 4) & 0x0F0F0F0F);
	n = (n & 0xFFFF) + (n >> 16);
	n = (n & 0xFF) + (n >> 8);

	return static_cast<int>(n);
#endif
}

////////////////////////////////////////////////////////////////////////////////
// bitboard bitcount
[[nodiscard]]
inline int bitCount(BitBoard bb)
{
#ifdef SYSTEM64BIT
#ifdef USE_POPCOUNT
#ifdef _MSC_VER
	return static_cast<int>(__popcnt64(bb));
#else
	return static_cast<int>(__builtin_popcountll(bb));
#endif // _MSC_VER
#else
	return bitCountSoft(bb);
#endif // USE_POPCOUNT
#else
	return bitCountSoft(bb);
#endif // SYSTEM64BIT
}

////////////////////////////////////////////////////////////////////////////////
// test : get the first bit of a bitboard
// warning ! don't work for bb = 0
[[nodiscard]]
inline int getFirstBit(BitBoard bb)
{
	assert(bb != 0ULL);
#ifdef _MSC_VER
#ifdef SYSTEM64BIT
	unsigned long index;
	_BitScanForward64(&index, bb);
	return static_cast<int>(index);
#else
	unsigned long index;
	if (static_cast<std::uint32_t>(bb)) {
		_BitScanForward(&index, static_cast<std::uint32_t>(bb));
		return static_cast<int>(index);
	} else {
		_BitScanForward(&index, static_cast<std::uint32_t>(bb >> 32));
		return static_cast<int>(index) + 32;
	}
#endif // SYSTEM64BIT
#else
	return static_cast<int>(__builtin_ctzll(bb));
#endif // _MSC_VER
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline int getLastBit(BitBoard bb)
{
	assert(bb != 0ULL);
#ifdef _MSC_VER
#ifdef SYSTEM64BIT
	unsigned long index;
	_BitScanReverse64(&index, bb);
	return static_cast<int>(index);
#else
	unsigned long index;
	if (static_cast<std::uint32_t>(bb >> 32)) {
		_BitScanReverse(&index, static_cast<std::uint32_t>(bb >> 32));
		return static_cast<int>(index) + 32;
	} else {
		_BitScanReverse(&index, static_cast<std::uint32_t>(bb));
		return static_cast<int>(index);
	}
#endif // SYSTEM64BIT
#else
	return 63 - static_cast<int>(__builtin_clzll(bb));
#endif // _MSC_VER
}

////////////////////////////////////////////////////////////////////////////////
inline void clearFirstBit(BitBoard &bb)
{
	bb &= (bb - 1);
}

////////////////////////////////////////////////////////////////////////////////
// get the first bit and remove it from bitboard
[[nodiscard]]
inline int popFirstBit(BitBoard &bb)
{
	int x = getFirstBit(bb);
	clearFirstBit(bb);
	return x;
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline BitBoard isolateFirstBit(BitBoard bb)
{
	return (bb & ~(bb - 1));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline Square getFirstSquare(BitBoard bb)
{
	return static_cast<Square>(getFirstBit(bb));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline Square getLastSquare(BitBoard bb)
{
	return static_cast<Square>(getLastBit(bb));
}

////////////////////////////////////////////////////////////////////////////////
// get nearest square on a file bitboard for a given side
[[nodiscard]]
inline Square getNearestSquare(Side s, BitBoard bb)
{
	return (s == white) ? getFirstSquare(bb) : getLastSquare(bb);
}

////////////////////////////////////////////////////////////////////////////////
// get farthest square on a file bitboard for a given side
[[nodiscard]]
inline Square getFarthestSquare(Side s, BitBoard bb)
{
	return (s == white) ? getLastSquare(bb) : getFirstSquare(bb);
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline Square popSquare(BitBoard &bb)
{
	return static_cast<Square>(popFirstBit(bb));
}

////////////////////////////////////////////////////////////////////////////////
// for all squares in a Bitboard
template<class Func>
void scan(BitBoard &bb, Func f)
{
	while (bb) {
		auto src = BitBoards::popSquare(bb);
		f(src);
	}
}

// init bitboard tables
void init();

////////////////////////////////////////////////////////////////////////////////
class OutputBitBoard {

public:

	BitBoard value;

	explicit OutputBitBoard(BitBoard b) : value(b)
	{}
};

// print a bitboard 
std::ostream &operator<<(std::ostream &os, const OutputBitBoard &bb);

} // namespace BitBoards

} // namespace Cheese

#endif // CHEESE_BITBOARD_H_
