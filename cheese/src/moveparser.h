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

#ifndef CHEESE_MOVEPARSER_H_
#define CHEESE_MOVEPARSER_H_

#include <string>
#include <string_view>

#include "board.h"
#include "move.h"

namespace Cheese::ChessMoveParser {

constexpr std::string_view str_files = "abcdefgh";

constexpr std::string_view str_ranks = "12345678";

constexpr std::string_view str_pieces_lower = " pnbrqk";

constexpr std::string_view str_pieces_upper = " PNBRQK";

constexpr std::string_view str_castle_short = "O-O";

constexpr std::string_view str_castle_long = "O-O-O";

constexpr char char_capture = 'x';

constexpr std::size_t max_char = 256;

////////////////////////////////////////////////////////////////////////////////
// convert ASCII char to piece type (lowercase = black, uppercase = white)
constexpr auto ascii_to_piece = []() {
	std::array<int, max_char> table {};
	for (std::size_t c=0; c<max_char; ++c) {
		int p = 0;
		auto i = str_pieces_lower.find(static_cast<char>(c));
		if (i != std::string_view::npos) {
			p = piece_id(black, static_cast<Piece>(i));
		} else {
			i = str_pieces_upper.find(static_cast<char>(c));
			if (i != std::string_view::npos) {
				p = piece_id(white, static_cast<Piece>(i));
			}	
		}
		table[c] = p;
	}
	return table;
} ();

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr bool is_char_file(char c)
{
	auto n = static_cast<int>(c) - static_cast<int>('a');
	return ((n >= 0) && (n < nb_files));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr bool is_char_rank(char c)
{
	auto n = static_cast<int>(c) - static_cast<int>('1');
	return ((n >= 0) && (n < nb_ranks));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr bool is_char_piece(char c)
{
	return (ascii_to_piece[c] != 0);
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr bool is_char_side(char c)
{
	if (c == 'b') {
		return true;
	}
	if (c == 'w') {
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
// assume c is a valid file
[[nodiscard]]
constexpr File file_from_char(char c)
{
	return static_cast<File>(static_cast<int>(c) - static_cast<int>('a'));
}

////////////////////////////////////////////////////////////////////////////////
// assume c is a valid rank
[[nodiscard]]
constexpr Rank rank_from_char(char c)
{
	return static_cast<Rank>(static_cast<int>(c) - static_cast<int>('1'));
}

////////////////////////////////////////////////////////////////////////////////
// assume c is a valid piece
[[nodiscard]]
constexpr Piece piece_from_char(char c)
{
	return static_cast<Piece>(ascii_to_piece[c] & (piece_color_offset - 1));
}

////////////////////////////////////////////////////////////////////////////////
// assume c is a valid piece
[[nodiscard]]
constexpr Side piece_color_from_char(char c)
{
	int p = ascii_to_piece[c];
	return (p < piece_color_offset) ? black : white;
}

////////////////////////////////////////////////////////////////////////////////
// assume c is a valid side
[[nodiscard]]
constexpr Side side_from_char(char c)
{
	return (c == 'b') ? black : white;
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr char file_to_char(File f)
{
	return static_cast<char>(static_cast<int>(f) + static_cast<int>('a'));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr char rank_to_char(Rank r)
{
	return static_cast<char>(static_cast<int>(r) + static_cast<int>('1'));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr char piece_to_char_upper(Piece p)
{
	return str_pieces_upper[static_cast<std::size_t>(p)];
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr char piece_to_char_lower(Piece p)
{
	return str_pieces_lower[static_cast<std::size_t>(p)];
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr char piece_to_char(Piece p, Side s)
{
	return (s == black) ?
		str_pieces_lower[static_cast<std::size_t>(p)] : 
		str_pieces_upper[static_cast<std::size_t>(p)];
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr char side_to_char(Side s)
{
	return (s == black) ? 'b' : 'w';
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr char piece_id_to_char(int p)
{
	return (p < piece_color_offset) ?
		str_pieces_lower[p] :
		str_pieces_upper[p];
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
constexpr bool is_string_square(const std::string_view str)
{
	return (str.size() == 2) &&
		is_char_file(str[0]) &&
		is_char_rank(str[1]);
}

////////////////////////////////////////////////////////////////////////////////
// assume str is a valid square
[[nodiscard]]
constexpr Square square_from_string(const std::string_view &str)
{
	File f = file_from_char(str[0]);
	Rank r = rank_from_char(str[1]);
	return makeSquare(f, r);
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
std::string square_to_string(Square s);

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
std::string move_to_string(const ChessMove &m);

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
bool move_from_string(const std::string_view &str, const ChessBoard &board,
	ChessMove &move);

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
std::string move_to_string_san(const ChessMove &m, const ChessBoard &board);

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
std::string move_to_string_lan(const ChessMove &m);

////////////////////////////////////////////////////////////////////////////////
// create move from string in SAN format for current position, 
// also support LAN, and use default queen promotion if not set
// skip check and mate characters
[[nodiscard]]
bool move_from_string_san(const std::string_view &str, const ChessBoard &board, 
	ChessMove &move);

} // namespace Cheese::ChessMoveParser

#endif //CHEESE_MOVEPARSER_H_

