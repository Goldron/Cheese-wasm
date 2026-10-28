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

#include "board.h"

#include "moveparser.h"
#include "util/logfile.h"

#include <cassert>

#include <string>

namespace Cheese {

// offset to en passant square for each side (for the side who move)
const std::array<SquareDir, nb_sides> enpassant_offset = { dir_up, dir_down };

const std::array<CastlingRight, nb_sides> castle_default = {
	castle_black_init,
	castle_white_init
};

////////////////////////////////////////////////////////////////////////////////
// default pieces positions used by setStartPosition
static const std::array<int, nb_squares> default_position_pieces = {
	 piece_id(white, rook),   piece_id(white, knight),  
     piece_id(white, bishop), piece_id(white, queen),
	 piece_id(white, king),   piece_id(white, bishop),  
	 piece_id(white, knight), piece_id(white, rook),
	 piece_id(white, pawn),   piece_id(white, pawn),
	 piece_id(white, pawn),   piece_id(white, pawn),
	 piece_id(white, pawn),   piece_id(white, pawn),
	 piece_id(white, pawn),   piece_id(white, pawn),
	 0,     0,       0,       0,      0,     0,       0,       0,
	 0,     0,       0,       0,      0,     0,       0,       0,
	 0,     0,       0,       0,      0,     0,       0,       0,
	 0,     0,       0,       0,      0,     0,       0,       0,
	piece_id(black, pawn),   piece_id(black, pawn),
	piece_id(black, pawn),   piece_id(black, pawn),
	piece_id(black, pawn),   piece_id(black, pawn),
	piece_id(black, pawn),   piece_id(black, pawn),
	piece_id(black, rook),   piece_id(black, knight), 
	piece_id(black, bishop), piece_id(black, queen), 
	piece_id(black, king),   piece_id(black, bishop), 
	piece_id(black, knight), piece_id(black, rook)
};

// material values for SEE
const std::array<int, nb_pieces_type> piece_value_see = {
	0,			// EMPTY
	100,		// pawn
	325,		// knight
	325,		// bishop
	500,		// rook
	975,		// queen
	5000,		// king
	0
};

////////////////////////////////////////////////////////////////////////////////
// clear board
void ChessBoard::clear()
{
	hashKey = 0;
	hashKeyPawns = 0;
	bbAllPiece = 0;
	bbColorPiece = {};
	bbPiece = {};
	pieces = {};
	evalMaterial = {};
	phaseCount = {};
	posKing = {};
	matCount = 0;
	player = black;
	ply = 0;
	castle = no_castle;
	enpassant = 0;
	fifty = 0;
	squaresCastleShort = {};
	squaresCastleLong = {};
	sqKingStart = {};
	sqCastleRookShort = {};
	sqCastleRookLong = {};
}

////////////////////////////////////////////////////////////////////////////////
// set chess default position
// FEN : rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
void ChessBoard::setStartPosition()
{
	setPosition(default_position_pieces, white, castle_init, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////
void ChessBoard::setPosition(const std::array<int, nb_squares> &pc, Side s,
	CastlingRight cst, int ep, int fft)
{
	bbAllPiece = 0;
	bbColorPiece = {};
	bbPiece = {};
	pieces = {};
	evalMaterial = {};
	phaseCount = {};
	matCount = 0;

	for (Square sq=a1; sq<=h8; ++sq) {
		Side pside = (pc[sq] < piece_color_offset) ? black : white;
		auto p = static_cast<Piece>
			((pside == black) ? pc[sq] : (pc[sq] - piece_color_offset));
		if ((p >= pawn) && (p <= king)) {
			setBoardPiece(pside, sq, p);
			evalMaterial[pside] += pieces_value[p];
			matCount += material_count_mask[pside][p];
			phaseCount[pside] += piece_game_phase[p];
			if (p == king) {
				posKing[pside] = sq;
			}
		}
	}

	player = s;
	castle = cst;
	enpassant = static_cast<std::uint8_t>(ep);
	fifty = static_cast<std::uint8_t>(fft);

	hashKey = createHashKey();
	hashKeyPawns = createHashKeyPawns();

	initCastlingRights();
}

////////////////////////////////////////////////////////////////////////////////
bool ChessBoard::findCastlingRook(Side side, Square src, Square dst, 
	Square &sq) const
{
	if (src != dst) {
		auto d = (src < dst) ? dir_right : dir_left;
		for (auto s=src+d; s!=dst+d; s+=d) {
			if (bitboardPiece(side, rook) & BitBoards::square[s]) {
				sq = s;
				return true;
			}
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
void ChessBoard::initCastlingRights()
{
	// king must be on start square
	// (should not happen with a valid FEN)
	if (squareRank(posKing[white]) != rank_1) {
		castle &= ~castle_white_init;
	}

	if (squareRank(posKing[black]) != rank_8) {
		castle &= ~castle_black_init;
	}

	// king start position
	// if the king didn't move we know he is on his start square
	// else suppose it was the default chess start position
	sqKingStart[black] = canCastle(castle_black_init) ? posKing[black] : e8;
	sqKingStart[white] = canCastle(castle_white_init) ? posKing[white] : e1;

	// default
	sqCastleRookShort[black] = h8;
	sqCastleRookLong[black] = a8;
	sqCastleRookShort[white] = h1;
	sqCastleRookLong[white] = a1;

	// find rook position and build castling path

	squaresCastleShort[black] = 0ULL;
	if (canCastle(castle_black_short)) {
		if (findCastlingRook(black, posKing[black], h8, 
			sqCastleRookShort[black])) {			
			squaresCastleShort[black] = 
				(BitBoards::between[posKing[black]][h8] |
				BitBoards::between[sqCastleRookShort[black]][e8]) &
				~BitBoards::square[sqKingStart[black]] &
				~BitBoards::square[sqCastleRookShort[black]];
		} else {
			castle &= ~castle_black_short;
		}
	}

	squaresCastleLong[black] = 0ULL;
	if (canCastle(castle_black_long)) {
		if (findCastlingRook(black, posKing[black], a8, 
			sqCastleRookLong[black])) {			
			squaresCastleLong[black] = 
				(BitBoards::between[posKing[black]][b8] |
				BitBoards::between[sqCastleRookLong[black]][e8]) &
				~BitBoards::square[sqKingStart[black]] &
				~BitBoards::square[sqCastleRookLong[black]];
		} else {
			castle &= ~castle_black_long;
		}
	}

	squaresCastleShort[white] = 0ULL;
	if (canCastle(castle_white_short)) {
		if (findCastlingRook(white, posKing[white], h1, 
			sqCastleRookShort[white])) {			
			squaresCastleShort[white] = 
				(BitBoards::between[posKing[white]][h1] |
				BitBoards::between[sqCastleRookShort[white]][e1]) &
				~BitBoards::square[sqKingStart[white]] &
				~BitBoards::square[sqCastleRookShort[white]];
		} else {
			castle &= ~castle_white_short;
		}
	}

	squaresCastleLong[white] = 0ULL;
	if (canCastle(castle_white_long)) {
		if (findCastlingRook(white, posKing[white], a1, 
			sqCastleRookLong[white])) {			
			squaresCastleLong[white] = 
				(BitBoards::between[posKing[white]][b1] |
				BitBoards::between[sqCastleRookLong[white]][e1]) &
				~BitBoards::square[sqKingStart[white]] &
				~BitBoards::square[sqCastleRookLong[white]];
		} else {
			castle &= ~castle_white_long;
		}
	}

	Logger::debug() << "White castle : "
			  << (canCastle(castle_white_short) ? 'K' : ' ')
			  << (canCastle(castle_white_long) ? 'Q' : ' ');
	Logger::debug() << "Black castle : "
			  << (canCastle(castle_black_short) ? 'k' : ' ')
			  << (canCastle(castle_black_long) ? 'q' : ' ');
	Logger::debug() << "White King position = "
		<< ChessMoveParser::square_to_string(sqKingStart[white]);
	Logger::debug() << "Black King position = "
		<< ChessMoveParser::square_to_string(sqKingStart[black]);
	Logger::debug() << "White Rook short = "
		<< ChessMoveParser::square_to_string(sqCastleRookShort[white]);
	Logger::debug() << "White Rook long = "
		<< ChessMoveParser::square_to_string(sqCastleRookLong[black]);
	Logger::debug() << "Black Rook short = "
		<< ChessMoveParser::square_to_string(sqCastleRookShort[white]);
	Logger::debug() << "Black Rook long = "
		<< ChessMoveParser::square_to_string(sqCastleRookLong[black]);
}

////////////////////////////////////////////////////////////////////////////////
// Static Exchange Evaluation (SEE)
int ChessBoard::staticExchangeEvaluation(ChessMove move) const
{
	static constexpr std::size_t max_see_capture = 32;

	// attack type : 1 = bishop like, 2 = rook like
	static const std::array<int, nb_pieces_type> attType =
		{ 0, 1, 0, 1, 2, 3, 0, 0 };

	std::array<int, max_see_capture> capture;

	Square src = move.src();
	Square dst = move.dst();

	// find pieces attacking & defending the square
	// generate attacks bitboard, on dst square for all sides
	BitBoard bbQueenBishop = bitboardPieceType(queen) |
							 bitboardPieceType(bishop);

	BitBoard bbQueenRook = bitboardPieceType(queen) |
						   bitboardPieceType(rook);

	BitBoard bbAttack =
		((bitboardPiece(black, pawn) & BitBoards::attackPawn(white, dst)) |
		 (bitboardPiece(white, pawn) & BitBoards::attackPawn(black, dst)));
	bbAttack |= bitboardPieceType(knight) & BitBoards::attackKnight(dst);
	bbAttack |= (bbQueenBishop & BitBoards::attackBishop(dst, bitboardAll()));
	bbAttack |= (bbQueenRook & BitBoards::attackRook(dst, bitboardAll()));
	bbAttack |= bitboardPieceType(king) & BitBoards::attackKing(dst);

	BitBoard bbOcc = bitboardAll();

	int ncap = 0;

	// current piece value (the next piece captured)
	Piece cPiece = move.piece();

	capture[ncap] = piece_value_see[move.capture()];

	Piece pro = move.promotion();
	if (pro != no_piece) {
		cPiece = pro;
		capture[ncap] += piece_value_see[pro] - piece_value_see[pawn];
	}

	++ncap;

	// remove first attacker
	bbOcc &= ~BitBoards::square[src];

	// e.p : remove the captures piece !
	// a rook or queen in front of the e.p captured pawn could capture
	// the moved piece
	if (move.isEnPassant()) {
		Square sqe = dst + enpassant_offset[player];
		bbOcc &= ~BitBoards::square[sqe];
	}

	// discovered attacks
	if (attType[cPiece] & 1) {
		bbAttack |= (BitBoards::attackBishop(dst, bbOcc) & bbQueenBishop);
	}

	// note : 1st pawn move can discover file attack (promotion, en passant)
	if ((cPiece == pawn) || (attType[cPiece] & 2)) {
		bbAttack |= (BitBoards::attackRook(dst, bbOcc) & bbQueenRook);
	}

	bbAttack &= bbOcc;

	Side side = ~player;

	// loop all possible captures
	while (bbAttack) {

		BitBoard bbTmp;

		// find the least valuable attacker
		Piece n;
		for (n=pawn; n<=king; ++n) {
			bbTmp = (bbAttack & bitboardPiece(side, n));
			if (bbTmp) {
				break;
			}
		}

		if (n > king) {
			break;
		}

		// remove attacker
		bbOcc ^= BitBoards::isolateFirstBit(bbTmp);

		// discovered attacks
		if (attType[n] & 1) {
			bbAttack |= (BitBoards::attackBishop(dst, bbOcc) & bbQueenBishop);
		}

		if (attType[n] & 2) {
			bbAttack |= (BitBoards::attackRook(dst, bbOcc) & bbQueenRook);
		}

		bbAttack &= bbOcc;

		capture[ncap] = -capture[ncap - 1] + piece_value_see[cPiece];

		++ncap;

		// stop after king capture ?
		if (cPiece == king) {
			break;
		}

		cPiece = n;

		// stop here : doesn't change the result ?
		//if ((capture[ncap - 1]  - piece_value_see[cPiece]) > 0) {
		//	break;
		//}

		side = ~side;
	}

	while (--ncap) {
		capture[ncap - 1] = -std::max(-capture[ncap - 1], capture[ncap]);
	}

	return capture[0];
}

////////////////////////////////////////////////////////////////////////////////
// Make a move
ChessMoveRec ChessBoard::makeMove(ChessMove move)
{
	const ChessMoveRec rec { hashKey, hashKeyPawns, castle, enpassant, fifty };

	// decode move
	const Square src = move.src();
	Square dst = move.dst();
	const Piece mp = move.piece();
	const Piece cp = move.capture();
	const Piece pp = move.promotion();
	const bool cst = move.isCastle();

	// fifty moves rule : pawn move or capture = reset
	if ((mp == pawn) || (cp != no_piece)) {
		fifty = 0;
	} else {
		++fifty;
	}

	Square rsrc = a1;
	Square rdst = a1;

	if (cst) {
		rsrc = dst;
		if (player == white) {
			rdst = (dst > src) ? f1 : d1;
			dst = (dst > src) ? g1 : c1;
		} else {
			rdst = (dst > src) ? f8 : d8;
			dst = (dst > src) ? g8 : c8;
		}
	}

	// clear source piece
	clearBoardPiece(player, src, mp);

	const Side notplayer = ~player;

	// clear destination piece (captured) if needed
	if (cp) {

		// update material count
		matCount -= material_count_mask[notplayer][cp];
		evalMaterial[notplayer] -= pieces_value[cp];
		phaseCount[notplayer] -= piece_game_phase[cp];

		Square cdst = dst;

		// en passant
		if (move.isEnPassant()) {
			cdst += enpassant_offset[player];
		}

		clearBoardPiece(notplayer, cdst, cp);

		hashKey ^= Zobrist::piece(notplayer, cp, cdst);

		if (cp == pawn) {
			hashKeyPawns ^= Zobrist::piece(notplayer, pawn, cdst);
		}

		// if the rook is captured and we have not castled
		if (cp == rook) {

			if (player == black) {
				if (dst == sqCastleRookShort[white]) {
					castle &= ~castle_white_short;
				} else
				if (dst == sqCastleRookLong[white]) {
					castle &= ~castle_white_long;
				}
			} else {
				if (dst == sqCastleRookShort[black]) {
					castle &= ~castle_black_short;
				} else
				if (dst == sqCastleRookLong[black]) {
					castle &= ~castle_black_long;
				}
			}
		}
	}

	if (mp == king) {

		// castling
		if (cst) {

			// move rook
			// FRC : need to clear king and rook square
			// before adding king and rook to their new squares
			clearBoardPiece(player, rsrc, rook);
			setBoardPiece(player, rdst, rook);

			hashKey ^= (Zobrist::piece(player, rook, rsrc) ^
						Zobrist::piece(player, rook, rdst));
		}

		// king move : change position, and disable castling
		posKing[player] = dst;

		castle &= ~castle_default[player];

	} else
	if (mp == rook) { // rook move : disable castling on this side

		if (player == white) {
			if (src == sqCastleRookShort[white]) {
				castle &= ~castle_white_short;
			} else
			if (src == sqCastleRookLong[white]) {
				castle &= ~castle_white_long;
			}
		} else {
			if (src == sqCastleRookShort[black]) {
				castle &= ~castle_black_short;
			} else
			if (src == sqCastleRookLong[black]) {
				castle &= ~castle_black_long;
			}
		}
	}

	// new value of piece
	Piece np = mp;

	if (pp) {
		np = pp;

		// update material count
		matCount += (material_count_mask[player][pp] -
					 material_count_mask[player][pawn]);
		evalMaterial[player] += (pieces_value[pp] -
								 pieces_value[pawn]);
		phaseCount[player] += piece_game_phase[pp];
	}

	// move new piece to destination
	setBoardPiece(player, dst, np);

	hashKey ^= (Zobrist::piece(player, mp, src) ^
				Zobrist::piece(player, np, dst));

	enpassant = 0;	// clear en passant file for next move
	if (mp == pawn) {

		if (isMoveTwiceUp(src, dst)) {
			enpassant = static_cast<std::uint8_t>(squareFile(dst) + 1);
		}

		hashKeyPawns ^= Zobrist::piece(player, pawn, src);
		if (!pp) {
			hashKeyPawns ^= Zobrist::piece(player, pawn, dst);
		}
	}

	if (rec.enpassant != enpassant) {
		hashKey ^= (Zobrist::enpassant(rec.enpassant) ^
					Zobrist::enpassant(enpassant));
	}

	// replace castling rights in hash key
	if (castle != rec.castle) {
		hashKey ^= (Zobrist::castling(rec.castle) ^
					Zobrist::castling(castle));
	}

	// change side
	hashKey ^= Zobrist::side();

	// next player to move
	player = notplayer;

	return rec;
}

////////////////////////////////////////////////////////////////////////////////
// unMake a move
void ChessBoard::unMakeMove(ChessMove move, const ChessMoveRec &rec)
{
	// decode move
	const Square src = move.src();
	Square dst = move.dst();
	const Piece mp = move.piece();
	const Piece cp = move.capture();
	const Piece pp = move.promotion();
	const bool cst = move.isCastle();

	// previous player to move
	const Side notplayer = player;
	player = ~player;

	Square rsrc = a1;
	Square rdst = a1;

	if (cst) {

		rdst = dst;

		if (player == white) {
			rsrc = (dst > src) ? f1 : d1;
			dst = (dst > src) ? g1 : c1;
		} else {
			rsrc = (dst > src) ? f8 : d8;
			dst = (dst > src) ? g8 : c8;
		}
	}

	// clear destination piece
	Piece np = mp;
	if (pp) {
		np = pp;

		// update material count
		matCount += (material_count_mask[player][pawn] -
					 material_count_mask[player][pp]);
		evalMaterial[player] += (pieces_value[pawn] -
								 pieces_value[pp]);
		phaseCount[player] -= piece_game_phase[pp];
	}

	clearBoardPiece(player, dst, np);

	// set old destination piece if it was a capture
	if (cp) {

		// update material count
		matCount += material_count_mask[notplayer][cp];
		evalMaterial[notplayer] += pieces_value[cp];
		phaseCount[notplayer] += piece_game_phase[cp];

		// en passant
		Square cdst = dst;

		if (move.isEnPassant()) {
			cdst += enpassant_offset[player];
		}

		setBoardPiece(notplayer, cdst, cp);
	}

	if (mp == king)  {

		// castling
		if (cst) {
			// move rook
			clearBoardPiece(player, rsrc, rook);
			setBoardPiece(player, rdst, rook);
		}

		posKing[player] = src;
	}

	// set old source piece
	setBoardPiece(player, src, mp);

	// change ep flag + castling flags
	hashKey	  = rec.hashKey;
	hashKeyPawns = rec.hashKeyPawns;
	castle    = rec.castle;
	enpassant = rec.enpassant;
	fifty     = rec.fifty;
}

////////////////////////////////////////////////////////////////////////////////
// make a null move
ChessMoveRec ChessBoard::makeNullMove()
{
	const ChessMoveRec rec { hashKey, hashKeyPawns, castle, enpassant, fifty };

	fifty = 0;

	if (enpassant != 0) {
		hashKey ^= Zobrist::enpassant(enpassant);
		enpassant = 0;
	}

	hashKey ^= Zobrist::side();
	player = ~player;

	return rec;
}

////////////////////////////////////////////////////////////////////////////////
// unmake a null move
void ChessBoard::unMakeNullMove(const ChessMoveRec &rec)
{
	player = ~player;
	hashKey = rec.hashKey;
	hashKeyPawns = rec.hashKeyPawns;
	enpassant = rec.enpassant;
	fifty = rec.fifty;
}

////////////////////////////////////////////////////////////////////////////////
// Test move validity : but don't test if the move leave us in check
bool ChessBoard::validMove(ChessMove move) const
{
	if (move.isEmpty()) {
        return false;
	}

	const Square src = move.src();
	const Square dst = move.dst();
	const Piece mp  = move.piece();
	const Piece cp  = move.capture();

	// test source square = moved piece
    if (!(bitboardPiece(player, mp) & BitBoards::square[src])) {
		return false;
	}

	// test destination square
    if ((cp != no_piece) && (!move.isEnPassant())) {
		if ((cp == king) ||
			(!(bitboardPiece(~player, cp) & BitBoards::square[dst]))) {
			return false;
		}
    } else {
		if ((!move.isCastle()) &&
			(piece(dst) != no_piece)) {
			return false;
		}
	}

	switch (mp) {

		case	pawn:
				if (move.isPromotion()) {
					Rank n = (player == white) ? rank_8 : rank_1;
					if (squareRank(dst) != n) {
						return false;
					}
				}
				if (!move.isEnPassant()) {
					SquareDir n = (player == white) ? dir_down : dir_up;
					if (move.isPawnTwice()) {
						if ((piece(dst + n) != no_piece) ||
							((dst + n + n) != src)) {
							return false;
						}
					} else {
						if (cp == no_piece) {
							if ((dst + n) != src) {
								return false;
							}
						} else {
							if (((dst + n + dir_left) != src) &&
								((dst + n + dir_right) != src)) {
								return false;
							}
						}
					}
				} else {
					if (enpassant == (squareFile(dst) + 1)) {

						SquareDir n = (player == white) ? dir_down : dir_up;

						if (!(bitboardPiece(~player, pawn) &
							BitBoards::square[dst + n])) {
							return false;
						}

					} else {
						return false;
					}
				}
				break;

		case	bishop:
				if (!(BitBoards::attackBishop(src, bitboardAll()) &
					BitBoards::square[dst])) {
					return false;
				}
				break;

		case	rook:
				if (!(BitBoards::attackRook(src, bitboardAll()) &
					BitBoards::square[dst])) {
					return false;
				}
				break;

		case	queen:
				if (!(BitBoards::attackQueen(src, bitboardAll()) &
					BitBoards::square[dst])) {
					return false;
				}
				break;

		case	king:
				if (move.isCastle()) {

					if (piece(sqKingStart[player]) != king) {
						return false;
					}

					if (player == white) {

						if (dst == sqCastleRookShort[white]) {

							if (!canCastle(castle_white_short)) {
								return false;
							}

							if (!(bitboardPiece(white, rook) &
								BitBoards::square[sqCastleRookShort[white]])) {
								return false;
							}

							if (bitboardAll() & squaresCastleShort[white]) {
								return false;
							}

							for (Square i=std::min(sqKingStart[white], g1);
								i<=std::max(sqKingStart[white], g1); ++i) {
								if (isAttacked(i, black)) {
									return false;
								}
							}

						} else
						if (dst == sqCastleRookLong[white]) {

							if (!canCastle(castle_white_long)) {
								return false;
							}

							if (!(bitboardPiece(white, rook) &
								BitBoards::square[sqCastleRookLong[white]])) {
								return false;
							}

							if (bitboardAll() & squaresCastleLong[white]) {
								return false;
							}

							for (Square i=std::min(sqKingStart[white], c1);
								i<=std::max(sqKingStart[white], c1); ++i) {
								if (isAttacked(i, black)) {
									return false;
								}
							}

						} else {
							return false;
						}

					} else {

						if (dst == sqCastleRookShort[black]) {

							if (!canCastle(castle_black_short)) {
								return false;
							}

							if (!(bitboardPiece(black, rook) &
								BitBoards::square[sqCastleRookShort[black]])) {
								return false;
							}

							if (bitboardAll() & squaresCastleShort[black]) {
								return false;
							}

							for (Square i=std::min(sqKingStart[black], g8);
								i<=std::max(sqKingStart[black], g8); ++i) {
								if (isAttacked(i, white)) {
									return false;
								}
							}

						} else
						if (dst == sqCastleRookLong[black]) {

							if (!canCastle(castle_black_long)) {
								return false;
							}

							if (!(bitboardPiece(black, rook) &
								BitBoards::square[sqCastleRookLong[black]]))
							{
								return false;
							}

							if (bitboardAll() & squaresCastleLong[black]) {
								return false;
							}

							for (Square i=std::min(sqKingStart[black], c8);
								i<=std::max(sqKingStart[black], c8); ++i) {
								if (isAttacked(i, white)) {
									return false;
								}
							}

						} else {
							return false;
						}
					}

				} else {
					// * king destination square
					if (!(BitBoards::attackKing(src) &
						BitBoards::square[dst])) {
						return false;
					}
				}
				break;

		case	knight:
				// * test : dst = knight move
				if (!(BitBoards::attackKnight(src) & BitBoards::square[dst])) {
					return false;
				}
				break;

		default:
			return false;
	}

	// here, the king can be in check but it is tested in the search

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// check if a square is attacked by a given color
// sq = square to test
// color = color of the attacker
bool ChessBoard::isAttacked(Square sq, Side s) const
{
	return ((BitBoards::attackPawn(~s, sq) & bitboardPiece(s, pawn)) ||
			(BitBoards::attackKnight(sq) & bitboardPiece(s, knight)) ||
			(BitBoards::attackBishop(sq, bitboardAll()) &
				(bitboardPiece(s, bishop) | bitboardPiece(s, queen))) ||
			(BitBoards::attackRook(sq, bitboardAll()) &
				(bitboardPiece(s, rook) | bitboardPiece(s, queen))) ||
			(BitBoards::attackKing(sq) & bitboardPiece(s, king)));
}

////////////////////////////////////////////////////////////////////////////////
// same as isAttacked but with modified occupency
// (to see through the king for example)
bool ChessBoard::isAttackedOcc(Square sq, Side s, BitBoard occ) const
{
	return ((BitBoards::attackPawn(~s, sq) & bitboardPiece(s, pawn)) ||
			(BitBoards::attackKnight(sq) & bitboardPiece(s, knight)) ||
			(BitBoards::attackBishop(sq, occ) &
				(bitboardPiece(s, bishop) | bitboardPiece(s, queen))) ||
			(BitBoards::attackRook(sq, occ) &
				(bitboardPiece(s, rook) | bitboardPiece(s, queen))) ||
			(BitBoards::attackKing(sq) & bitboardPiece(s, king)));
}

////////////////////////////////////////////////////////////////////////////////
// initialize Zobrist HashKey from current position
HashKey ChessBoard::createHashKey() const
{
	HashKey key = 0ULL;

	for (Square sq=a1; sq<=h8; ++sq) {
		if (piece(sq) != no_piece) {
			if (bitboardColor(white) & BitBoards::square[sq]) {
				key ^= Zobrist::piece(white, piece(sq), sq);
			} else
			if (bitboardColor(black) & BitBoards::square[sq]) {
				key ^= Zobrist::piece(black, piece(sq), sq);
			}
		}
	}

	key ^= Zobrist::enpassant(enpassant);
	key ^= Zobrist::castling(castle);

	if (player == black) {
		key ^= Zobrist::side();
	}

	return key;
}

////////////////////////////////////////////////////////////////////////////////
// initialize Zobrist HashKey for pawns from current position
HashKey ChessBoard::createHashKeyPawns() const
{
	HashKey key = 0ULL;

	for (Square sq=a1; sq<=h8; ++sq) {
		if (piece(sq) == pawn) {
			if (bitboardColor(white) & BitBoards::square[sq]) {
				key ^= Zobrist::piece(white, pawn, sq);
			} else
			if (bitboardColor(black) & BitBoards::square[sq]) {
				key ^= Zobrist::piece(black, pawn, sq);
			}
		}
	}

	return key;
}

////////////////////////////////////////////////////////////////////////////////
SameAttack ChessBoard::findSamePieceAttack(Side s, Piece p, Square src, 
	Square dst) const
{
	if (pieces[src] != p) {
		return SameAttack::error;	
	}
	
	// piece attack from dst square 
	BitBoard bb;
	switch (p) {
		case pawn:
			bb = BitBoards::pawn_attack[~s][dst] & bitboardPiece(s, p);
			break;
		case knight:
			bb = BitBoards::attackKnight(dst) & bitboardPiece(s, p);
			break;
		case bishop:
			bb = BitBoards::attackBishop(dst, bitboardAll()) & 
				bitboardPiece(s, p);
			break;
		case rook:
			bb = BitBoards::attackRook(dst, bitboardAll()) & 
				bitboardPiece(s, p);
			break;
		case queen:
			bb = BitBoards::attackQueen(dst, bitboardAll()) & 
				bitboardPiece(s, p);
			break;
		case king:
			return SameAttack::none;
		default:
			return SameAttack::error;
	}

	auto n = BitBoards::bitCount(bb);
	if ((n == 0) || !(bb & BitBoards::square[src])) {
		return SameAttack::error;
	}

	if (n == 1) {
		return SameAttack::none;
	}

	if (n > 2) {
		return SameAttack::other;
	}

	auto r = SameAttack::error;		
	bb &= ~BitBoards::square[src];

	if (bb & BitBoards::file[squareFile(src)]) {			
		if (bb & BitBoards::rank[squareRank(src)]) {
			r = SameAttack::error;
		} else {
			r = SameAttack::file;
		}
	} else {
		if (bb & BitBoards::rank[squareRank(src)]) {
			r = SameAttack::rank;
		} else {
			r = SameAttack::other;
		}
	}

	return r;	
}

////////////////////////////////////////////////////////////////////////////////
BitBoard ChessBoard::findPinnedPieces(Side s, Square dst) const
{
	BitBoard attackers = (BitBoards::attackBishop(dst, bitboardColor(~s)) 
		& (bitboardPiece(~s, queen) | bitboardPiece(~s, bishop))) |
		(BitBoards::attackRook(dst, bitboardColor(~s)) 
		& (bitboardPiece(~s, queen) | bitboardPiece(~s, rook)));

	BitBoard pinned = 0ULL;
	BitBoards::scan(attackers, [&](Square src) {
		BitBoard b = BitBoards::between[dst][src] & bitboardColor(s);
		if (!BitBoards::bitCountMoreThanOne(b)) {
			pinned |= b;
		}
	});

	return pinned;
}

////////////////////////////////////////////////////////////////////////////////
Square ChessBoard::findPieceAttackFor(Side s, Piece p, Square dst, 
	BitBoard mask) const
{
	BitBoard m = bitboardPiece(s, p) & mask;

	if (p != king) {
		BitBoard pinned = 0;
		BitBoard bb = m;
		BitBoards::scan(bb, [&](Square src) {
			BitBoard occ = (bitboardAll() & ~BitBoards::square[src]) | 
				BitBoards::square[dst];

			BitBoard pb = (bitboardPiece(~s, bishop) | bitboardPiece(~s, queen))
				& ~BitBoards::square[dst]; 
			BitBoard pr = (bitboardPiece(~s, rook) | bitboardPiece(~s, queen))
				& ~BitBoards::square[dst]; 

			BitBoard att = (BitBoards::attackBishop(kingPosition(s), occ) & pb) 
				|| (BitBoards::attackRook(kingPosition(s), occ) & pr);

			if (att != 0ULL) {
				pinned |= BitBoards::square[src]; 				
			}
		});
		m = m & ~pinned;
	}

	BitBoard att = 0;
	switch (p) {
		case pawn:
			att = BitBoards::attackPawn(~s, dst) & m;
			if (att != 0ULL) {
				return BitBoards::getFirstSquare(att);
			} 
			if (s == white) {
				Square sq = relSquareDown<white>(dst);
				if ((m & BitBoards::square[sq]) != 0ULL) {
					return sq;
				}
				Square sq2 = relSquareDown2<white>(dst);
				if ((piece(sq) == no_piece) &&
					((m & BitBoards::square[sq2]) != 0ULL)) {
					return sq2;
				}
			} else {
				Square sq = relSquareDown<black>(dst);
				if ((m & BitBoards::square[sq]) != 0ULL) {
					return sq;
				}
				Square sq2 = relSquareDown2<black>(dst);
				if ((piece(sq) == no_piece) &&
					((m & BitBoards::square[sq2]) != 0ULL)) {
					return sq2;
				}
			}
			break;
		case knight:
			att = BitBoards::attackKnight(dst) & m;
			if (att != 0ULL) {
				return BitBoards::getFirstSquare(att);
			}
			break;
		case bishop:
			att = BitBoards::attackBishop(dst, bitboardAll()) & m;
			if (att != 0ULL) {
				return BitBoards::getFirstSquare(att);
			}
			break;
		case rook:
			att = BitBoards::attackRook(dst, bitboardAll()) & m;
			if (att != 0ULL) {
				return BitBoards::getFirstSquare(att);
			}
			break;
		case queen:
			att = BitBoards::attackQueen(dst, bitboardAll()) & m;
			if (att != 0ULL) {
				return BitBoards::getFirstSquare(att);
			}
			break;
		case king:
			att = BitBoards::attackKing(dst) & m;
			if (att != 0ULL) {
				return BitBoards::getFirstSquare(att);
			}
			break;
		default:
			break;
	}
	return a1;
}

////////////////////////////////////////////////////////////////////////////////
Square ChessBoard::findPieceAttack(Side s, Piece p, Square dst) const
{
	static constexpr BitBoard full = std::numeric_limits<std::uint64_t>::max();
	return findPieceAttackFor(s, p, dst, full);
}

////////////////////////////////////////////////////////////////////////////////
Square ChessBoard::findPieceAttackOnFile(Side s, Piece p, Square dst, File f)
	const
{
	return findPieceAttackFor(s, p, dst, BitBoards::file[f]);
}

////////////////////////////////////////////////////////////////////////////////
Square ChessBoard::findPieceAttackOnRank(Side s, Piece p, Square dst, Rank r)
	const
{
	return findPieceAttackFor(s, p, dst, BitBoards::rank[r]);
}

////////////////////////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &os, const ChessBoard &board) 
{
	static const std::string_view line = "|---|---|---|---|---|---|---|---|";
	std::string tmp;
	os << '\n' << line << '\n';
	for (Rank y=rank_1; y<=rank_8; ++y) {
		tmp = '|';
		for (File x=file_a; x<=file_h; ++x) {
			tmp += ' ';
			Square p = makeSquare(x, static_cast<Rank>(rank_8 - y));
			Side s = (board.bitboardColor(white) & BitBoards::square[p]) ? 
				white : black;
			tmp += ChessMoveParser::piece_to_char(board.piece(p), s);
			tmp += ' ';
			tmp += '|';
		}
		tmp += '\n';
		os << tmp;
		os << line << '\n';
	}
	os << ((board.side() == white) ? "White" : "Black")
		<< " to play" << std::endl;
	return os;
}

} // namespace Cheese
