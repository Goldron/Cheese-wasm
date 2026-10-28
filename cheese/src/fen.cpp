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

#include "fen.h"

#include "move.h"
#include "moveparser.h"

#include "util/logfile.h"

#include <array>
#include <fstream>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
bool is_char_digit(char c)
{
	return ((c >= '0') && (c <= '9'));
}

////////////////////////////////////////////////////////////////////////////////
int digit_from_char(char c)
{
	return static_cast<int>(c) - static_cast<int>('0');
}

////////////////////////////////////////////////////////////////////////////////
char digit_to_char(int v)
{
	return static_cast<char>(static_cast<int>('0') + v);
}

////////////////////////////////////////////////////////////////////////////////
bool ChessFENParser::load(const std::string &filename,
	std::vector<std::string> &fens)
{
	std::ifstream ifs(filename, std::ifstream::in);
	if (!ifs.is_open()) {
		return false;
	}

	std::string str;
	while (std::getline(ifs, str)) {

		// remove special characters
		str.erase(std::remove_if(str.begin(), str.end(), 
			[](unsigned char c) { return !std::isprint(c); }), str.end());

		if (!str.empty()) {
			fens.push_back(str);
		}
	}

	ifs.close();

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessFENParser::parseBoard(const std::string_view &str, 
	std::array<int, nb_squares> &pieces, std::array<Square, nb_sides> &kingpos)
{
	if (str.empty()) {
		return false; 
	}
	std::array<std::array<int, nb_pieces_type>, nb_sides> nbpieces {};
	int x = 0;
	int y = static_cast<int>(rank_8);
	static constexpr int max_x = static_cast<int>(file_h);
	for (const auto c : str) {
		if (ChessMoveParser::is_char_rank(c)) {
			x += static_cast<int>(ChessMoveParser::rank_from_char(c)) + 1;
			if (x > (max_x + 1)) {
				return false;
			}
		} else
		if (c == '/') {
			// don't allow unfinished line, but accept empty line
			if ((x != 0) && (x != 8)) {
				return false;
			}
			x = 0;
			--y;
			if (y < 0) {
				return false;
			}
		} else {
			if (ChessMoveParser::is_char_piece(c)) {
				if (x > max_x) {
					return false;
				}
				auto s = ChessMoveParser::piece_color_from_char(c);
				auto p = ChessMoveParser::piece_from_char(c);
				auto sq = makeSquare(static_cast<File>(x), 
					static_cast<Rank>(y));
				if ((p == pawn) && ((y == 0) || (y == nb_ranks - 1))) {
					return false;
				}
				pieces[sq] = piece_id(s, p);
				if (p == king) {
					kingpos[s] = sq;
				}
				++nbpieces[s][p];
				++x;
			} else {
				return false;
			}
		}
	}
	if ((nbpieces[black][king] != 1) ||
		(nbpieces[white][king] != 1)) {
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessFENParser::parseCastling(const std::string_view &str, Square kingsq, 
	CastlingRight &cst)
{
	if ((str.size() == 1) && (str[0] == '-')) {
		return true;
	}
	cst = no_castle;
	for (const auto c : str) {
		Side s = std::islower(c) ? black : white;
		auto cl = static_cast<char>(std::tolower(c));
		switch (cl) {
			case	'k':
					cst |= (s == white) ? castle_white_short : 
						castle_black_short;
					break;
			case	'q':
					cst |= (s == white) ? castle_white_long : 
						castle_black_long;
					break;
			default:
				// FRC support
				if (ChessMoveParser::is_char_file(cl)) {
					File f = ChessMoveParser::file_from_char(cl);
					if (f > squareFile(kingsq)) {
						cst |= (s == white) ? castle_white_short : 
							castle_black_short;
					} else {
						cst |= (s == white) ? castle_white_long : 
							castle_black_long;
					}
				} else {
					return false;
				}
				break;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessFENParser::parseEnPassant(const std::string_view &str, Side side, 
	int &ep)
{
	ep = 0;
	if ((str.size() == 1) && (str[0] == '-')) {
		return true;
	}
	if ((str.size() != 2) ||
		!ChessMoveParser::is_char_file(str[0]) ||
		!ChessMoveParser::is_char_rank(str[1])) {
		return false;
	}
	auto f = ChessMoveParser::file_from_char(str[0]);
	auto r = ChessMoveParser::rank_from_char(str[1]);
	auto r2 = (side == black) ? rank_3 : rank_6;
	if (r != r2) {
		return false;
	}
	ep = static_cast<int>(f) + 1;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessFENParser::parseNumber(const std::string_view &str, int &v)
{
	v = 0;
	if (str.empty() || (str == "-")) {
		return false;
	}
	for (const auto c : str) {
		if (is_char_digit(c)) {
			v = v * 10 + digit_from_char(c);
		} else {
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessFENParser::set(const std::string_view &fen, ChessBoard &pos)
{
	std::array<int, nb_squares> pieces {};
	std::array<Square, nb_sides> kingpos { e1, e8 };
	int ep = 0, fifty = 0, ply = 0;
	Side side = white;
	CastlingRight cst = no_castle;

	// pieces
	auto p2 = fen.find_first_of(' ');
	if (p2 == std::string_view::npos) {
		return false;
	}
	if (!parseBoard(fen.substr(0, p2), pieces, kingpos)) {
		return false;
	}

	// side
	auto p1 = p2 + 1;
	p2 = fen.find_first_of(' ', p1);
	auto str = fen.substr(p1, p2 - p1);
	if ((p2 == std::string_view::npos) || (str.size() != 1)) {
		return false;
	}
	auto c = str[0];
	if (!ChessMoveParser::is_char_side(c)) {
		return false;
	}
	side = ChessMoveParser::side_from_char(c);

	// castling
	if (p2 == std::string_view::npos) {
		return false;
	}
	p1 = p2 + 1;
	p2 = fen.find_first_of(' ', p1);
	if (!parseCastling(fen.substr(p1, p2 - p1), kingpos[side], 
		cst)) {
		return false;
	}

	// en passant
	if (p2 == std::string_view::npos) {
		return false;
	}
	p1 = p2 + 1;
	p2 = fen.find_first_of(' ', p1);
	if (!parseEnPassant(fen.substr(p1, p2 - p1), side, ep)) {
		return false;
	}

	// fifty rule
	if (p2 != std::string_view::npos) {
		p1 = p2 + 1;
		p2 = fen.find_first_of(' ', p1);
		bool r = parseNumber(fen.substr(p1, p2 - p1), fifty);
		if (r) {	
			// ply count
			if (p2 != std::string_view::npos) {
				p1 = p2 + 1;
				p2 = fen.find_first_of(' ', p1);
				if (!parseNumber(fen.substr(p1, p2 - p1), ply)) {
					ply = 1;
				}
			}
		}
	}

	pos.setPosition(pieces, side, cst, ep, fifty);
	pos.setPly(ply);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
std::string ChessFENParser::get(const ChessBoard &pos)
{
	std::string fen;
	int	nb = 0;

	for (int y=7; y>=0; y--) {
		for (int x=0; x<8; x++) {

			File f = static_cast<File>(x);
			Rank r = static_cast<Rank>(y);
			Square sq = makeSquare(f, r);
			Side s = (pos.bitboardColor(white) & BitBoards::square[sq]) ? 
				white : black;
			Piece p = pos.piece(sq);

			if (p == no_piece) {
				++nb;
			} else {
				if (nb != 0) {
					fen += digit_to_char(nb);
					nb = 0;
				}
				fen += ChessMoveParser::piece_to_char(p, s);
			}
		}

		if (nb != 0) {
			fen += digit_to_char(nb);
			nb = 0;
		}

		if (y != 0) {
			fen += '/';
		}
	}

	fen += ' ';

	Side side = pos.side();
	fen += ChessMoveParser::side_to_char(side);

	fen += ' ';

	auto p = fen.size();

	if (pos.canCastle(castle_white_short)) {
		fen += 'K';
	}
	if (pos.canCastle(castle_white_long)) {
		fen += 'Q';
	}
	if (pos.canCastle(castle_black_short)) {
		fen += 'k';
	}
	if (pos.canCastle(castle_black_long)) {
		fen += 'q';
	}

	if (fen.size() == p) {
		fen += '-';
	}

	fen += ' ';

	if (pos.isEnPassant()) {
		fen += ChessMoveParser::square_to_string(pos.enPassantSquare(side));
	} else {
		fen += '-';
	}

	fen += ' ';

	// clock moves
	fen += std::to_string(pos.fiftyRule());

	fen += ' ';

	// full moves
	fen += std::to_string(pos.getPly());

	return fen;
}

////////////////////////////////////////////////////////////////////////////////
// reverse a FEN string :
//
// - flip board verticaly
// - flip colour
// - flip side to move
// - flip castle
// - flip en passant
//
bool ChessFENParser::reverse(const std::string_view &str, std::string &newfen)
{
	auto ptr = str.begin();
	auto ptrend = str.end();

	while ((ptr != ptrend) && (*ptr != ' ')) {
		++ptr;
	}
	if (ptr == ptrend) {
		return false;
	}

    newfen = "";

    auto ps = str.begin();
	auto pe = ptr;
	auto p = ptr - 1;
	while (ptr >= ps) {
		if ((ptr == ps) || (*(ptr - 1) == '/')) {
			for (int i=0; i<=(p - ptr); i++) {
				char c = *(ptr + i);
				if ((c >= 'a') && (c <= 'z')) {
					c = static_cast<char>(c + ('A' - 'a'));
				} else if ((c >= 'A') && (c <= 'Z')) {
					c = static_cast<char>(c + ('a' - 'A'));
				} else if ((c < '0') || (c > '9')) {
					return false;
				}
				newfen += c;
			}
			if (ptr > ps) {
				newfen += '/';
				p = ptr - 2;
				--ptr;
			}
		}
		--ptr;
	}

	ptr = pe;
	if (*ptr != ' ') {
		return false;
	}
	newfen += ' ';
	++ptr;
	if (ptr == ptrend) {
		return false;
	}

	Side side = black;
	if (*ptr == 'w') {
		side = white;
		newfen += 'b';
	} else
	if (*ptr == 'b') {
		side = black;
		newfen += 'w';
	} else {
		return false;
	}

	++ptr;
	if (ptr == ptrend) {
		return true;
	}
	if (*ptr != ' ') {
		return false;
	}
	newfen += ' ';
	++ptr;

	// castle
	char c = 0;
    int n = 0;
	bool cwk = false;
	bool cwq = false;
	bool cbk = false;
	bool cbq = false;
	while (ptr < ptrend) {

		c = *(ptr++);

		if (c == ' ') {

			if (cbk) {
				newfen += 'K';
			}
			if (cbq) {
				newfen += 'Q';
			}
			if (cwk) {
				newfen += 'k';
			}
			if (cwq) {
				newfen += 'q';
			}

			newfen += ' ';
			break;
		} else {
			if ((++n) > 4) {
				return false;
			}
			switch (c) {
				case 	'-':
						newfen += '-';
						break;
				case	'K':
						cwk = true;
						break;
				case	'Q':
						cwq = true;
						break;
				case	'k':
						cbk = true;
						break;
				case	'q':
						cbq = true;
						break;
				default:
					if ((c >= 'A') && (c <= 'H')) {
						newfen += static_cast<char>(c - 'A' + 'a');
					} else
					if ((c >= 'a') && (c <= 'h')) {
						newfen += static_cast<char>(c - 'a' + 'A');
					} else {
						return false;
					}
					break;
			}
		}
	}

	if (ptr == ptrend) {
		return false;
	}

	// en passant
	c = *(ptr++);

	if (c == '-') {
		newfen += '-';
		if (ptr == ptrend) {
			return false;
		}
	} else {

		if ((c >= 'a') && (c <= 'h')) {

			if (ptr == ptrend) {
				return false;
			}

			newfen += c;
			c = *(ptr++);

			if ((c >= '1') && (c <= '8')) {

				int y = c - '1';
				if ((side == black) && (y != 2)) {
					return false;
				}
				if ((side == white) && (y != 5)) {
					return false;
				}

				if (y == 2) {
					newfen += '6';
				} else {
					newfen += '3';
				}

			} else {
				return false;
			}

		} else {
			return false;
		}
	}

	while (ptr < ptrend) {
		newfen += *(ptr++);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
std::string ChessFENParser::getOption(const std::string &fen,
							     const std::string &name)
{
	auto p = fen.find(name);
	if ((p != std::string::npos) &&
		(p != 0U) &&
		((fen[p - 1] == ' ') || (fen[p - 1] == ';')) &&
		((p + name.size()) < fen.size()) &&
		(fen[p + name.size()] == ' ')) {

		auto p1 = fen.find_first_of(';', p + name.size());
		if (p1 != std::string::npos) {
			return fen.substr(p + name.size() + 1, p1 - p - name.size() - 1);
		} else {
			return fen.substr(p + name.size(), fen.size() - p - name.size());
		}
	}
	return "";
}

} // namespace Cheese
