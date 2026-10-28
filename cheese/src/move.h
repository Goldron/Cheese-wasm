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

#ifndef CHEESE_MOVE_H_
#define CHEESE_MOVE_H_

#include "config.h"

#include "util/bitfield.h"

#include <algorithm>
#include <array>
#include <string>

namespace Cheese {

inline constexpr int nb_squares = 64;
inline constexpr int nb_ranks = 8;
inline constexpr int nb_files = 8;
inline constexpr int nb_sides = 2;
inline constexpr int nb_pieces_type = 8;
inline constexpr int max_square_distance = 8;
inline constexpr int nb_pieces_name = 16;
inline constexpr int piece_color_offset = 8;

// Directions from square to square (from black side point of view)
enum SquareDir : int {
	dir_none  		= 0,
	dir_up    		=  nb_files,
	dir_down  		= -nb_files,
	dir_right 		=  1,
	dir_left  		= -1,
	dir_up_2 		= dir_up * 2,
	dir_down_2 		= dir_down * 2,
	dir_up_right   	= dir_up + dir_right,
	dir_up_left    	= dir_up + dir_left,
	dir_down_right 	= dir_down + dir_right,
	dir_down_left  	= dir_down + dir_left
};

// Squares
enum Square : int {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

// Squares ranks
enum Rank : int {
	rank_1,
	rank_2,
	rank_3,
	rank_4,
	rank_5,
	rank_6,
	rank_7,
	rank_8
};

// Squares files
enum File : int {
	file_a,
	file_b,
	file_c,
	file_d,
	file_e,
	file_f,
	file_g,
	file_h
};

// Playing side
enum Side {
	black,
	white
};

// Chess Pieces type values
enum Piece {
	no_piece,
	pawn,
	knight,
	bishop,
	rook,
	queen,
	king
};

// castling rights
enum CastlingRight : std::uint8_t {

	// no castling rights
	no_castle = 0,

	// white short castle enable
	castle_white_short = 1,

	// white long castle enable
	castle_white_long = 2,

	// black short castle enable
	castle_black_short = 4,

	// black long castle enable
	castle_black_long = 8,

	// white default castle rights
	castle_white_init = (castle_white_short | castle_white_long),

	// black default castle enable
	castle_black_init = (castle_black_short | castle_black_long),

	// default castling rights
	castle_init = (castle_black_init | castle_white_init)
};

struct ChessMoveSort;

////////////////////////////////////////////////////////////////////////////////
// a chess move packed in 32 bits
struct ChessMove
{
	using move_type = std::uint32_t;

private:

	using bits_src       = BitField<move_type, 0, 6>;
	using bits_dst       = BitField<move_type, 6, 6>;
	using bits_moved     = BitField<move_type, 12, 3>;
	using bits_captured  = BitField<move_type, 15, 3>;
	using bits_promotion = BitField<move_type, 18, 3>;
	using bits_castle    = BitField<move_type, 21, 1>;
	using bits_enpassant = BitField<move_type, 22, 1>;

	// move bit format (32 bits / 23 bits used)
	//
	// 00000000 00000000 00000000 00xxxxxx : src square	(y*8+x) : bits  1 -  6
	// 00000000 00000000 0000xxxx xx000000 : dst square	(y*8+x) : bits  7 - 12
	// 00000000 00000000 0xxx0000 00000000 : moved piece        : bits 13 - 15
	// 00000000 000000xx x0000000 00000000 : captured piece     : bits 16 - 18
	// 00000000 000xxx00 00000000 00000000 : promotion piece    : bits 18 - 20
	// 00000000 00x00000 00000000 00000000 : castle             : bit  21
	// 00000000 0x000000 00000000 00000000 : en passant         : bit  22
	// ???????? ?0000000 00000000 00000000 : not used           : bits 23 - 31

	move_type flag {0};

public:

	ChessMove() = default;

	explicit ChessMove(move_type data) : flag(data)
	{}

	ChessMove(Square src, Square dst, Piece mv, Piece cap = no_piece, 
		Piece pro = no_piece, int cas = 0, int ep = 0);

	// get move raw data
	[[nodiscard]]
	move_type data() const;

	// set move data
	void set(move_type f);

	// clear move
	void clear();

	// test if move is empty
	[[nodiscard]]
	bool isEmpty() const;

	bool operator== (const ChessMove &m) const;

	bool operator!= (const ChessMove &m) const;

	[[nodiscard]]
	bool isCastle() const;

	[[nodiscard]]
	bool isCapture() const;

	[[nodiscard]]
	bool isCaptureOrPromotion() const;

	[[nodiscard]]
	bool isPromotion() const;

	[[nodiscard]]
	bool isEnPassant() const;

	[[nodiscard]]
	bool isPawnTwice() const;

	// pawn moving on 7th rank
	[[nodiscard]]
	bool isMovePawnOn7th() const;

	// pawn moving past 4th rank
	[[nodiscard]]
	bool isMovePawnPast4th(Side side) const;

	// test if the move is reversible
	// not a pawn move, castling, or capture
	[[nodiscard]]
	bool isReversible() const;

	// get source square
	[[nodiscard]]
	Square src() const;

	// get destination square
	[[nodiscard]]
	Square dst() const;

	// get moved piece
	[[nodiscard]]
	Piece piece() const;

	// get captured piece
	[[nodiscard]]
	Piece capture() const;

	// get promotion piece
	[[nodiscard]]
	Piece promotion() const;

};

////////////////////////////////////////////////////////////////////////////////
// sortable chess move
struct ChessMoveSort {

	using value_type = std::int32_t;

	// chess move
	ChessMove move;

	// order value
	value_type value {0};

	ChessMoveSort() = default;

	ChessMoveSort(ChessMove m, value_type v = 0) : move(m), value(v)
	{ }

	ChessMoveSort & operator= (const ChessMove &m)
	{
		move = m;
		value = 0;
		return *this;
	}
};

// SquareDir operators
constexpr auto operator+(const SquareDir &a, const SquareDir &b)
{
	return static_cast<SquareDir>(static_cast<int>(a) + static_cast<int>(b));
}

constexpr auto operator-(const SquareDir &a, const SquareDir &b)
{
	return static_cast<SquareDir>(static_cast<int>(a) - static_cast<int>(b));
}

// Square operators
constexpr auto operator+(const Square &a, const SquareDir &b)
{
	return static_cast<Square>(static_cast<int>(a) + static_cast<int>(b));
}

constexpr auto operator-(const Square &a, const SquareDir &b)
{
	return static_cast<Square>(static_cast<int>(a) - static_cast<int>(b));
}

constexpr auto operator+=(Square &a, const SquareDir &b)
{
	return a = static_cast<Square>(static_cast<int>(a) + static_cast<int>(b));
}

constexpr auto operator-=(Square &a, const SquareDir &b)
{
	return a = static_cast<Square>(static_cast<int>(a) - static_cast<int>(b));
}

constexpr auto operator++(Square &a)
{
	return a = static_cast<Square>(static_cast<int>(a) + 1);
}

constexpr auto operator--(Square &a)
{
	return a = static_cast<Square>(static_cast<int>(a) - 1);
}

// Rank operators
constexpr auto operator+(const Rank &a, const int &b)
{
	return static_cast<Rank>(static_cast<int>(a) + b);
}

constexpr auto operator-(const Rank &a, const int &b)
{
	return static_cast<Rank>(static_cast<int>(a) - b);
}

constexpr auto operator-(const Rank &a, const Rank &b)
{
	return static_cast<int>(a) - static_cast<int>(b);
}

constexpr auto operator++(Rank &a)
{
	return a = static_cast<Rank>(static_cast<int>(a) + 1);
}

constexpr auto operator--(Rank &a)
{
	return a = static_cast<Rank>(static_cast<int>(a) - 1);
}

// File operators
constexpr auto operator+(const File &a, const int &b)
{
	return static_cast<File>(static_cast<int>(a) + b);
}

constexpr auto operator-(const File &a, const int &b)
{
	return static_cast<File>(static_cast<int>(a) - b);
}

constexpr auto operator-(const File &a, const File &b)
{
	return static_cast<int>(a) - static_cast<int>(b);
}

constexpr auto operator++(File &a)
{
	return a = static_cast<File>(static_cast<int>(a) + 1);
}

constexpr auto operator--(File &a)
{
	return a = static_cast<File>(static_cast<int>(a) - 1);
}

// flip side
constexpr auto operator~(Side s)
{
	return static_cast<Side>(static_cast<std::uint32_t>(s) ^ 1);
}

// Piece operators
constexpr auto operator++(Piece &a)
{
	return a = static_cast<Piece>(static_cast<int>(a) + 1);
}

constexpr auto operator--(Piece &a)
{
	return a = static_cast<Piece>(static_cast<int>(a) - 1);
}

// CastlingRight operators
constexpr auto operator~(CastlingRight c)
{
	return static_cast<CastlingRight>(~static_cast<std::uint8_t>(c));
}

constexpr auto operator&=(CastlingRight &a, CastlingRight b)
{
	return (a = static_cast<CastlingRight>(static_cast<std::uint8_t>(a) & 
		static_cast<std::uint8_t>(b)));
}

constexpr auto operator|=(CastlingRight &a, CastlingRight b)
{
	return (a = static_cast<CastlingRight>(static_cast<std::uint8_t>(a) | 
		static_cast<std::uint8_t>(b)));
}

////////////////////////////////////////////////////////////////////////////////
// calculate square offset
[[nodiscard]]
constexpr Square makeSquare(File x, Rank y)
{
	return static_cast<Square>(static_cast<unsigned int>(y) * nb_files +
							   static_cast<unsigned int>(x));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr Square flipSquareFile(Square s)
{
	constexpr auto m = static_cast<unsigned int>(nb_files - 1);
	return static_cast<Square>(static_cast<unsigned int>(s) ^ m);
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr Square flipSquareRank(Square s)
{
	constexpr auto m = static_cast<unsigned int>(nb_squares - 1) ^
					   		  static_cast<unsigned int>(nb_files - 1);
	return static_cast<Square>(static_cast<unsigned int>(s) ^ m);
}

////////////////////////////////////////////////////////////////////////////////
// get rank from square offset
[[nodiscard]]
constexpr Rank squareRank(Square sq)
{
	return static_cast<Rank>(static_cast<unsigned int>(sq) / nb_files);
}

////////////////////////////////////////////////////////////////////////////////
// get file from square offset
[[nodiscard]]
constexpr File squareFile(Square sq)
{
	constexpr unsigned int f = nb_files - 1;
	return static_cast<File>(static_cast<unsigned int>(sq) & f);	
}

////////////////////////////////////////////////////////////////////////////////
// get file distance
[[nodiscard]]
inline int fileDistance(Square sqa, Square sqb)
{
	return std::abs(squareFile(sqa) - squareFile(sqb));
}

////////////////////////////////////////////////////////////////////////////////
// get rank distance
[[nodiscard]]
inline int rankDistance(Square sqa, Square sqb)
{
	return std::abs(squareRank(sqa) - squareRank(sqb));
}

////////////////////////////////////////////////////////////////////////////////
// calculate square distance
[[nodiscard]]
inline int squareDistance(Square sqa, Square sqb)
{
	return std::max(fileDistance(sqa, sqb), rankDistance(sqa, sqb));
}

////////////////////////////////////////////////////////////////////////////////
// relative rank from player position
template<Side side>
[[nodiscard]]
constexpr Rank relRank(Rank r)
{
	return static_cast<Rank>((side == white) ? r : (rank_8 - r));
}

////////////////////////////////////////////////////////////////////////////////
// relative square rank from player position
template<Side side>
[[nodiscard]]
constexpr Rank relSquareRank(Square sq)
{
	return relRank<side>(squareRank(sq));
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquare(Square sq)
{
	constexpr auto m = static_cast<unsigned int>(nb_squares - 1) ^
					   static_cast<unsigned int>(nb_files - 1);
	return ((side == white) ? sq : 
		static_cast<Square>(static_cast<unsigned int>(sq) ^ m));
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquareUp(Square sq)
{
	return sq + ((side == white) ? dir_up : dir_down);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquareUp2(Square sq)
{
	return sq + ((side == white) ? dir_up_2 : dir_down_2);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquareDown(Square sq)
{
	return sq + ((side == white) ? dir_down : dir_up);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquareDown2(Square sq)
{
	return sq + ((side == white) ? dir_down_2 : dir_up_2);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquareUpLeft(Square sq)
{
	return sq + ((side == white) ? dir_up_left : dir_down_right);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquareUpRight(Square sq)
{
	return sq + ((side == white) ? dir_up_right : dir_down_left);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquareDownRight(Square sq)
{
	return sq + ((side == white) ? dir_down_right : dir_up_left);
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
[[nodiscard]]
constexpr Square relSquareDownLeft(Square sq)
{
	return sq + ((side == white) ? dir_down_left : dir_up_right);
}

////////////////////////////////////////////////////////////////////////////////
// test if moving 2 squares up or down
[[nodiscard]]
constexpr bool isMoveTwiceUp(Square src, Square dst)
{
	return (static_cast<unsigned int>(src) ^ static_cast<unsigned int>(dst)) 
		== dir_up_2;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove::ChessMove(Square src, Square dst, Piece mv, Piece cap, 
	Piece pro, int cas, int ep) : 
		flag(bits_src::encode(src) |
			 bits_dst::encode(dst) |
			 bits_moved::encode(mv) |
			 bits_captured::encode(cap) |
			 bits_promotion::encode(pro) |
			 bits_castle::encode(cas) |
 			 bits_enpassant::encode(ep))
{
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove::move_type ChessMove::data() const
{
	return flag;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessMove::set(ChessMove::move_type f)
{
	flag = f;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessMove::clear()
{
	flag = 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isEmpty() const
{
	return (flag == 0);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::operator== (const ChessMove &m) const
{
	return (flag == m.flag);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::operator!= (const ChessMove &m) const
{
	return (flag != m.flag);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isCastle() const
{
	return bits_castle::test(flag);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isCapture() const
{
	return bits_captured::test(flag);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isPromotion() const
{
	return bits_promotion::test(flag);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isCaptureOrPromotion() const
{
	return (isCapture() || 
		    isPromotion());
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isEnPassant() const
{
	return bits_enpassant::test(flag);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isPawnTwice() const
{
	return ((piece() == pawn) &&
			isMoveTwiceUp(src(), dst()));
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isMovePawnOn7th() const
{
	Rank r = squareRank(dst());
	return ((piece() == pawn) &&
			((r == rank_2) || (r == rank_7)));
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isMovePawnPast4th(Side side) const
{
	Rank r = (side == white) ?
				squareRank(src()) :
				static_cast<Rank>(rank_8 - squareRank(src()));			
	return ((piece() == pawn) && (r > rank_4));
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessMove::isReversible() const
{
	return ((piece() != pawn) &&
			(!isCapture()) &&
			(!isCastle()));
}

////////////////////////////////////////////////////////////////////////////////
inline Square ChessMove::src() const
{
	return bits_src::get<Square>(flag);
}

////////////////////////////////////////////////////////////////////////////////
inline Square ChessMove::dst() const
{
	return bits_dst::get<Square>(flag);
}

////////////////////////////////////////////////////////////////////////////////
inline Piece ChessMove::piece() const
{
	return bits_moved::get<Piece>(flag);
}

////////////////////////////////////////////////////////////////////////////////
inline Piece ChessMove::capture() const
{
	return bits_captured::get<Piece>(flag);
}

////////////////////////////////////////////////////////////////////////////////
inline Piece ChessMove::promotion() const
{
	return bits_promotion::get<Piece>(flag);
}

////////////////////////////////////////////////////////////////////////////////
// get a piece id from piece and playing side
constexpr int piece_id(Side c, Piece p)
{
	return (c == white) ? (static_cast<int>(p) + piece_color_offset) :
		static_cast<int>(p);
}

extern const std::array<char, nb_pieces_name> pieces_name;

} // namespace Cheese

#endif //CHEESE_MOVE_H_
