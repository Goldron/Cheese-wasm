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

#include "moveparser.h"

#include "util/logfile.h"

namespace Cheese::ChessMoveParser {

////////////////////////////////////////////////////////////////////////////////
std::string square_to_string(Square s)
{
	std::string str;
	str += file_to_char(squareFile(s));
	str += rank_to_char(squareRank(s));
	return str;
}

////////////////////////////////////////////////////////////////////////////////
std::string move_to_string(const ChessMove &m)
{
	std::string str;
	Square src = m.src();
	str += file_to_char(squareFile(src));
	str += rank_to_char(squareRank(src));
	Square dst = m.dst();
	str += file_to_char(squareFile(dst));
	str += rank_to_char(squareRank(dst));
	Piece p = m.promotion();
	if (p != no_piece) {
		str += piece_to_char_lower(m.promotion());
	}
	return str;
}

////////////////////////////////////////////////////////////////////////////////
bool move_from_string(const std::string_view &str, const ChessBoard &board,
	ChessMove &move)
{
	const Side side = board.side();

	if (str == str_castle_short) {
		move = ChessMove(board.squareKingStart(side), 
			board.squareRookShort(side), king, no_piece, no_piece, 1, 0);
		return true;
	}

	if (str == str_castle_long) {
		move = ChessMove(board.squareKingStart(side), 
			board.squareRookLong(side), king, no_piece, no_piece, 1, 0);
		return true;
	}

	if (str.size() > 5) {
		return false;
	}
	
	if ((str.size() >= 4) && 
		is_char_file(str[0]) &&
		is_char_rank(str[1]) &&
		is_char_file(str[2]) &&
		is_char_rank(str[3])) {

		File f1 = file_from_char(str[0]);
		Rank r1 = rank_from_char(str[1]);
		File f2 = file_from_char(str[2]);
		Rank r2 = rank_from_char(str[3]);

		Piece p = no_piece;
		if (str.size() == 5) {
			if (!is_char_piece(str[4])) {
				return false;
			}
			p = piece_from_char(str[4]);
			if ((p < knight) || (p > queen)) {
				return false;
			}
		}
		
		Square src = makeSquare(f1, r1);
		Square dst = makeSquare(f2, r2);

		Piece mp = board.piece(src);

		int ep = ((mp == pawn) && (board.piece(dst) == no_piece) &&
				  (f1 != f2) && board.isEnPassant());
		
		Piece cap = (ep == 0) ? board.piece(dst) : pawn;

		// force queen promotion if not set	
		//if (((r2 == rank_8) || (r2 == rank_1)) && (mp == pawn) && 
		//	(p == no_piece)) {
		//	p = queen;
		//}

		int cas = 0;
		if (mp == king) {

			// classic rules : change castle move to "king capture rook"
			if (board.getChessRules() == ChessRules::classic) {
		
				Square sqcr = (side == white) ? g1 : g8;
				Square sqcl = (side == white) ? c1 : c8;

				if ((src == board.squareKingStart(side)) && (dst == sqcr)) {
					dst = board.squareRookShort(side);
				}
				if ((src == board.squareKingStart(side)) && (dst == sqcl)) {
					dst = board.squareRookLong(side);
				}
			}

			if ((src == board.squareKingStart(side)) &&
				(board.bitboardPiece(side, rook) &
					BitBoards::square[dst]) &&
				((dst == board.squareRookShort(side)) ||
				 (dst == board.squareRookLong(side)))) {
				cas = 1;
				cap = no_piece;
			}
		}

		move = ChessMove(src, dst, mp, cap, p, cas, ep);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////
std::string move_to_string_san(const ChessMove &m, const ChessBoard &board)
{
	std::string str;

	if (m.isCastle()) {
		if (m.dst() > m.src()) {
			str += str_castle_short;
		} else {
			str += str_castle_long;
		}
		return str;
	}

	Piece p = m.piece();
	
	if (p == pawn) {

		Square src = m.src();
		Square dst = m.dst();

		str += file_to_char(squareFile(src));

		if (m.isEnPassant()) {
			str += 'x';
			str += file_to_char(squareFile(dst));
			str += rank_to_char(squareRank(dst));
		} else {
			if (m.isCapture()) {
				str += 'x';
				str += file_to_char(squareFile(dst));
				str += rank_to_char(squareRank(dst));
			} else {			
				str += rank_to_char(squareRank(dst));
			}

			if (m.isPromotion()) {
				str += '=';
				str += piece_to_char_upper(m.promotion());
			}
		}
		
	} else {

		str += piece_to_char_upper(m.piece());

		Square src = m.src();
		Square dst = m.dst();

		SameAttack a = board.findSamePieceAttack(board.side(), m.piece(),
			src, dst);

		switch (a) {
			case SameAttack::none:
				break;
			case SameAttack::file:
				str += rank_to_char(squareRank(src));
				break;
			case SameAttack::rank:
			case SameAttack::other:
				str += file_to_char(squareFile(src));
				break;
			case SameAttack::error:
				return "";
				break;
		}

		if (m.isCapture()) {
			str += 'x';
		}

		str += file_to_char(squareFile(dst));
		str += rank_to_char(squareRank(dst));				
	}

	// TODO : check and mate symbol

	return str;	
}

////////////////////////////////////////////////////////////////////////////////
std::string move_to_string_lan(const ChessMove &m)
{
	std::string str;

	if (m.isCastle()) {
		if (m.dst() > m.src()) {
			str += str_castle_short;
		} else {
			str += str_castle_long;
		}
		return str;
	}

	Piece p = m.piece();
	
	if (p != pawn) {
		str += piece_to_char_upper(p);
	}

	Square src = m.src();
	Square dst = m.dst();

	str += file_to_char(squareFile(src));
	str += rank_to_char(squareRank(src));

	if (m.isCapture()) {
		str += 'x';
	}

	str += file_to_char(squareFile(dst));
	str += rank_to_char(squareRank(dst));

	if (m.isPromotion()) {
		str += '=';
		str += piece_to_char_upper(m.promotion());		
	}	

	return str;	
}

////////////////////////////////////////////////////////////////////////////////
bool move_from_string_san(const std::string_view &str, const ChessBoard &board, 
	ChessMove &move)
{
	const Side side = board.side();

	std::string_view s = str;
	if (str.back() == '+') {
		s = str.substr(0, str.size() - 1);
	}

	if (str.back() == '#') {
		s = str.substr(0, str.size() - 1);
	}

	if (s == str_castle_short) {
		move = ChessMove(board.squareKingStart(side), 
			board.squareRookShort(side), king, no_piece, no_piece, 1, 
			0);
		return true;
	}

	if (s == str_castle_long) {
		move = ChessMove(board.squareKingStart(side), 
			board.squareRookLong(side), king, no_piece, no_piece, 1, 
			0);
		return true;
	}

	if ((s.size() < 2) || (s.size() > 6)) {
		return false;
	}

	if (is_char_file(s[0])) {

		File f1 = file_from_char(s[0]);

		if (is_char_rank(s[1])) {

			Rank r1 = rank_from_char(s[1]);
			
			Square dst = makeSquare(f1, r1);

			Square src = board.findPieceAttackOnFile(side, pawn, dst, 
				f1);

			// ex: e4
			if (s.size() == 2) {
				move = ChessMove(src, dst, pawn, no_piece, 
					(r1 == rank_8) ? queen : no_piece, 0, 0);
				return true;
			}

			if (s.size() == 4) {
				// ex: e8=Q
				if ((s[2] == '=') &&
					is_char_piece(s[3])) {
					Piece p = piece_from_char(s[3]);
					if ((p > pawn) && (p < king)) {
						move = ChessMove(src, dst, pawn, no_piece, p, 0, 0);
						return true;
					}
				}

				// ex: LAN e2e4
				if (is_char_file(s[2]) &&
					is_char_rank(s[3])) {
					src = dst;
					Rank r2 = rank_from_char(s[3]);
					dst = makeSquare(file_from_char(s[2]), r2);
					move = ChessMove(src, dst, pawn, no_piece, 
						(r2 == rank_8) ? queen : no_piece, 0, 0);
					return true;
				}
			}

			// ex: LAN e2xf3
			//	+ e7xf8 =>=Q
			if ((s.size() == 5) &&
				(s[2] == 'x') &&
				is_char_file(s[3]) &&
				is_char_rank(s[4])) {
				src = dst;
				Rank r2 = rank_from_char(s[4]);
				dst = makeSquare(file_from_char(s[3]), r2);
				int ep = (board.isEnPassant() &&
						  (board.enPassantSquare(side) == dst)) ? 1 : 0;
				move = ChessMove(src, dst, pawn, board.piece(dst), 
						(r2 == rank_8) ? queen : no_piece, 0, ep);
				return true;
			}

			// ex: LAN e7e8=Q
			if ((s.size() == 6) &&
				is_char_file(s[2]) &&
				is_char_rank(s[3]) &&
				(s[4] == '=') &&
				is_char_piece(s[5])) {
				src = dst;
				Rank r2 = rank_from_char(s[3]);
				dst = makeSquare(file_from_char(s[2]), r2);
				move = ChessMove(src, dst, pawn, board.piece(dst), 
						piece_from_char(s[5]), 0, 0);
					return true;
			}

			// ex: LAN e7xf8=Q
			if ((s.size() == 7) &&
				(s[2] == 'x') &&
				is_char_file(s[3]) &&
				is_char_rank(s[4]) &&
				(s[5] == '=') &&
				is_char_piece(s[6])) {
				src = dst;
				Rank r2 = rank_from_char(s[4]);
				dst = makeSquare(file_from_char(s[3]),
							     r2);
				move = ChessMove(src, dst, pawn, board.piece(dst), 
						piece_from_char(s[6]), 0, 0);
					return true;
			}
			
			return false;
		}

		if (s[1] == 'x') {
	
			if ((s.size() == 4) &&
				is_char_file(s[2]) &&
				is_char_rank(s[3])) {

				// ex: exf6
				File f2 = file_from_char(s[2]);	
				Rank r2 = rank_from_char(s[3]);	
				Square dst = makeSquare(f2, r2);

				Square src = board.findPieceAttackOnFile(side, pawn, 
					dst, f1);

				int ep = (board.isEnPassant() &&
						  (board.enPassantSquare(side) == dst)) ? 1 : 0;

				move = ChessMove(src, dst, pawn, ep ? pawn : board.piece(dst), 
					(r2 == rank_8) ? queen : no_piece, 0, ep);
				return true;
			}

			if ((s.size() == 6) &&
				is_char_file(s[2]) &&
				is_char_rank(s[3]) &&
				(s[4] == '=') &&
				is_char_piece(s[5])) {
				
				// ex: exf8=Q
				File f2 = file_from_char(s[2]);	
				Rank r2 = rank_from_char(s[3]);	
				Square dst = makeSquare(f2, r2);

				Square src = board.findPieceAttackOnFile(side, pawn, dst, f1);

				move = ChessMove(src, dst, pawn, board.piece(dst), 
					piece_from_char(s[5]), 0, 0);
				return true;
			}
		}
	
		return false;
	}
	
	if (is_char_piece(s[0])) {
			
		Piece p = piece_from_char(s[0]);

		if (is_char_file(s[1])) {

			File f1 = file_from_char(s[1]);	

			if ((s.size() == 3) && (is_char_rank(s[2]))) {

				// ex: Nc3
				Rank r1 = rank_from_char(s[2]);	
				Square dst = makeSquare(f1, r1);

				Square src = board.findPieceAttack(side, p, dst);

				move = ChessMove(src, dst, p, no_piece, no_piece, 0, 0);
				return true;
			}

			if ((s.size() == 4) && 
				is_char_file(s[2]) &&
				is_char_rank(s[3])) {

				// ex: Nbc3
				File f2 = file_from_char(s[2]);	
				Rank r2 = rank_from_char(s[3]);	
				Square dst = makeSquare(f2, r2);

				Square src = board.findPieceAttackOnFile(side, p, 
					dst, f1);

				move = ChessMove(src, dst, p, no_piece, no_piece, 0, 0);
				return true;
			}

			if ((s.size() == 5) && 
				is_char_rank(s[2]) &&
				is_char_file(s[3]) &&
				is_char_rank(s[4])) {

				Rank r1 = rank_from_char(s[2]);	
				Square src = makeSquare(f1, r1);

				// ex: Nb1c3
				File f2 = file_from_char(s[3]);	
				Rank r2 = rank_from_char(s[4]);	
				Square dst = makeSquare(f2, r2);

				move = ChessMove(src, dst, p, no_piece, no_piece, 0, 0);
				return true;
			}

			if ((s.size() == 5) &&
				(s[2] == 'x') &&
				is_char_file(s[3]) &&
				is_char_rank(s[4])) {
				
				// ex: Bcxd4
				File f2 = file_from_char(s[3]);	
				Rank r2 = rank_from_char(s[4]);	
				Square dst = makeSquare(f2, r2);

				Square src = board.findPieceAttackOnFile(side, p, dst, f1);

				move = ChessMove(src, dst, p, board.piece(dst), no_piece, 0, 
					0);
				return true;

			}

			if ((s.size() == 6) && 
				is_char_rank(s[2]) &&
				(s[3] == 'x') &&
				is_char_file(s[4]) &&
				is_char_rank(s[5])) {

				// ex: Bc3xd4
				Rank r1 = rank_from_char(s[2]);	
				Square src = makeSquare(f1, r1);

				// ex: Nb1c3
				File f2 = file_from_char(s[4]);	
				Rank r2 = rank_from_char(s[5]);	
				Square dst = makeSquare(f2, r2);

				move = ChessMove(src, dst, p, board.piece(dst), no_piece, 0, 0);
				return true;
			}

			return false;
		}

		if (is_char_rank(s[1])) {		

			Rank r1 = rank_from_char(s[1]);	

			if ((s.size() == 4) &&
				is_char_file(s[2]) &&
				is_char_rank(s[3])) {

				// ex: N1c3
				File f2 = file_from_char(s[2]);	
				Rank r2 = rank_from_char(s[3]);	
				Square dst = makeSquare(f2, r2);

				Square src = board.findPieceAttackOnRank(side, p, 
					dst, r1);

				move = ChessMove(src, dst, p, no_piece, no_piece, 0, 0);
				return true;
			}

			if ((s.size() == 5) && 
				(s[2] == 'x') &&
				is_char_file(s[3]) &&
				is_char_rank(s[4])) {

				// ex: B3xd4
				File f2 = file_from_char(s[3]);	
				Rank r2 = rank_from_char(s[4]);	
				Square dst = makeSquare(f2, r2);

				Square src = board.findPieceAttackOnRank(side, p, dst, r1);

				move = ChessMove(src, dst, p, board.piece(dst), no_piece, 0, 0);
				return true;
			}

			return false;
		}

		if (s[1] == 'x') {

			// ex: Bxd4
			if ((s.size() == 4) &&
				is_char_file(s[2]) &&
				is_char_rank(s[3])) {
				
				File f2 = file_from_char(s[2]);	
				Rank r2 = rank_from_char(s[3]);	
				Square dst = makeSquare(f2, r2);

				Square src = board.findPieceAttack(side, p, 
					dst);

				move = ChessMove(src, dst, p, board.piece(dst), no_piece, 0, 0);
				return true;
			}
		}
	}

	return false;
}

} // namespace ChessMoveParser

