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

#ifndef CHEESE_BOARD_H_
#define CHEESE_BOARD_H_

#include "config.h"

#include "bitboard.h"
#include "eval.h"
#include "material.h"
#include "move.h"
#include "zobrist.h"

#include <array>
#include <ostream>
#include <string_view>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
enum class ChessRules {
	// classic chess rules
	classic,

	// Fischer Random Chess (Chess 960)
	frc
};

using ChessGamePhase = std::uint32_t;

// game phases
//
//	value : pawns and king = 0
//			knight  = 1
//			bishop  = 1
//			rook    = 2
//			queen   = 4
//

// pawns ending (0)
inline constexpr ChessGamePhase gamephase_ending_pawns = 0;

// ending (1 .. 8)
inline constexpr ChessGamePhase gamephase_ending = 1;

// middle game (9 .. 20)
inline constexpr ChessGamePhase gamephase_middle_game = 9;

// opening (21 .. 31)
inline constexpr ChessGamePhase gamephase_opening = 21;

// start value (31)
inline constexpr ChessGamePhase gamephase_start = 31;

inline constexpr ChessGamePhase max_game_phase = 24;

// game phase value for each pieces
// max = 4 + 2*2 + 1*2 + 1*2 = 12 * 2 = 24
inline constexpr std::array<ChessGamePhase, nb_pieces_type> piece_game_phase = {
	0, 0, 1, 1, 2, 4, 0, 0
};

// return value from findSamePieceAttack function
enum class SameAttack {
	// no other attackers found
	none,
	// found an attacker on the same file
	file,
	// found an attacker on the same rank
	rank,
	// found multiple attackers or not on same file or rank
	other,
	// error
	error
};

////////////////////////////////////////////////////////////////////////////////
// Move undo information used in make/unmake
struct ChessMoveRec {

	// hashkey
	HashKey hashKey {0};

	HashKey hashKeyPawns {0};

	// castling flags
	CastlingRight castle {no_castle};

	// en passant file + 1
	std::uint8_t enpassant {0};

	// fifty move rule
	std::uint8_t fifty {0};

	ChessMoveRec() = default;

	ChessMoveRec(HashKey k, HashKey kp, CastlingRight cst, std::uint8_t ep, 
		std::uint8_t f) : hashKey(k), hashKeyPawns(kp), castle(cst),
		enpassant(ep), fifty(f)
	{ }
};

////////////////////////////////////////////////////////////////////////////////
// Define a position on the Chess board
class ChessBoard {

	// current position hashkey
	HashKey hashKey {0};

	// current position pawns hashkey
	HashKey hashKeyPawns {0};

	// all pieces bitboard
	BitBoard bbAllPiece {0};

	// all black & whites pieces bitboards
	std::array<BitBoard, nb_sides> bbColorPiece {};

	// pieces positions bitboards : for each color, each pieces
	std::array<std::array<BitBoard, nb_pieces_type>, nb_sides> bbPiece {};

	// needed for fast find piece type
	// note : possible to add piece color
	std::array<std::uint8_t, nb_squares> pieces {};

	// incremental material evaluation
	std::array<std::int32_t, nb_sides> evalMaterial {};

	// game phase count for each side
	std::array<ChessGamePhase, nb_sides> phaseCount {};

	// kings positions
	std::array<Square, nb_sides> posKing {};

	// pieces count mask
	MaterialMask matCount {0};

	// current playing side
	Side player {black};

	// current ply (used only by FEN)
	int ply {0};

	// castle possibility for white & black
	CastlingRight castle {no_castle};

	// en passant file + 1, or 0 if none
	std::uint8_t enpassant {0};

	// fifty moves rule
	std::uint8_t fifty {0};

	// * FRC :

	// squares between rook initial and final position
	std::array<BitBoard, nb_sides> squaresCastleShort {};

	std::array<BitBoard, nb_sides> squaresCastleLong {};

	// Initial position of king
	std::array<Square, nb_sides> sqKingStart {};

	// Initial position of King rook
	std::array<Square, nb_sides> sqCastleRookShort {};

	// Initial position of Queen rook
	std::array<Square, nb_sides> sqCastleRookLong {};

	// classic vs FRC support
	ChessRules rules {ChessRules::classic};

public:

	ChessBoard() = default;

	ChessBoard(const ChessBoard &) = default;

	ChessBoard & operator= (const ChessBoard &) = default;

	// clear board
	void clear();

	[[nodiscard]]
	BitBoard bitboardPiece(Side s, Piece p) const;

	[[nodiscard]]
	BitBoard bitboardColor(Side s) const;

	[[nodiscard]]
	BitBoard bitboardAll() const;

	[[nodiscard]]
	BitBoard bitboardPieceType(Piece p) const;

	[[nodiscard]]
	Piece piece(Square sq) const;

	[[nodiscard]]
	std::int32_t materialScore(Side s) const;

	[[nodiscard]]
	MaterialMask materialCount() const;

	[[nodiscard]]
	Square kingPosition(Side s) const;

	[[nodiscard]]
	bool canCastle(CastlingRight f) const;

	// if en passant is possible for the current siode
	[[nodiscard]]
	bool isEnPassant() const;

	[[nodiscard]]
	File enPassantFile() const;

	[[nodiscard]]
	Square enPassantSquare(Side s) const;

	// initialize king and rook positions, and castling path bitboard
	void initCastlingRights();

	// try to find castling rook
	[[nodiscard]]
	bool findCastlingRook(Side side, Square src, Square dst, Square &sq) const;

	[[nodiscard]]
	ChessGamePhase gamePhaseCount(Side s) const;

	// get current game phase
	[[nodiscard]]
	ChessGamePhase gamePhase() const;

	// set chess default position
	void setStartPosition();

	// set custom position
	void setPosition(const std::array<int, nb_squares> &pc, Side s, 
		CastlingRight cst, int ep, int fft);

	// check if a move is valid
	[[nodiscard]]
	bool validMove(ChessMove move) const;

	// create Zobrist HashKey from current position
	[[nodiscard]]
	HashKey createHashKey() const;

	// create Zobrist Hashkey for pawns
	[[nodiscard]]
	HashKey createHashKeyPawns() const;

	// make a move
	ChessMoveRec makeMove(ChessMove move);

	// unmake a move
	void unMakeMove(ChessMove move, const ChessMoveRec &rec);

	// make a null move
	ChessMoveRec makeNullMove();

	// unmake a null move
	void unMakeNullMove(const ChessMoveRec &rec);

	// test if draw by insufficient material
	[[nodiscard]]
	bool evaluateDraw() const;

	[[nodiscard]]
	Square squareKingStart(Side s) const;

	[[nodiscard]]
	BitBoard bitboardCastleShort(Side s) const;

	[[nodiscard]]
	BitBoard bitboardCastleLong(Side s) const;

	[[nodiscard]]
	Square squareRookShort(Side s) const;

	[[nodiscard]]
	Square squareRookLong(Side s) const;

	// return true if the king is attacked
	[[nodiscard]]
	bool inCheck() const;

	// get current position hashkey
	[[nodiscard]]
	HashKey getHashKey() const;

	// get current position pawns hashkey
	[[nodiscard]]
	HashKey getHashKeyPawns() const;

	// get current playing side
	[[nodiscard]]
	Side side() const;

	[[nodiscard]]
	std::uint8_t fiftyRule() const;

	[[nodiscard]]
	int getPly() const;

	void setPly(int p);

	// check if a square is attacked by a given color
	[[nodiscard]]
	bool isAttacked(Square sq, Side s) const;

	[[nodiscard]]
	bool isAttackedOcc(Square sq, Side s, BitBoard occ) const;

	// SEE : Static Exchange Evalutaion
	[[nodiscard]]
	int staticExchangeEvaluation(ChessMove move) const;

	// test if the current position is legal after playing a move
	// (opponent king not in check)
	// (used after makemove)
	[[nodiscard]]
	bool isPositionLegal() const;

	void setChessRules(ChessRules r);

	[[nodiscard]]
	ChessRules getChessRules() const;

	[[nodiscard]]
	BitBoard findPinnedPieces(Side s, Square dst) const;

	// test if multiple pieces of the same type are attacking the same square
	// usefull for SAN move parsing
	[[nodiscard]]
	SameAttack findSamePieceAttack(Side s, Piece p, Square src, 
		Square dst) const;

	// find the square of the piece attacking dst square
	// assume only one piece is attacking dst (using mask)
	[[nodiscard]]
	Square findPieceAttackFor(Side s, Piece p, Square dst, BitBoard mask) const;

	[[nodiscard]]
	Square findPieceAttackOnFile(Side s, Piece p, Square dst, File f) const;

	[[nodiscard]]
	Square findPieceAttackOnRank(Side s, Piece p, Square dst, Rank r) const;

	[[nodiscard]]
	Square findPieceAttack(Side s, Piece p, Square dst) const;

private:

	void clearBoardPiece(Side s, Square sq, Piece pc);

	void setBoardPiece(Side s, Square sq, Piece pc);

};

// print board
std::ostream &operator<<(std::ostream &os, const ChessBoard &board);

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline bool isGamePhaseOpening(ChessGamePhase p)
{
	return (p >= gamephase_opening);
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline bool isGamePhaseMiddle(ChessGamePhase p)
{
	return ((p >= gamephase_middle_game) &&
			(p < gamephase_opening));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline bool isGamePhaseEnding(ChessGamePhase p)
{
	return ((p >= gamephase_ending) &&
			(p < gamephase_middle_game));
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline bool isGamePhasePawnEnding(ChessGamePhase p)
{
	return (p == gamephase_ending_pawns);
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline bool isGamePhaseBeforeEnding(ChessGamePhase p)
{
	return (p >= gamephase_middle_game);
}

////////////////////////////////////////////////////////////////////////////////
[[nodiscard]]
inline bool isGamePhaseAfterMiddle(ChessGamePhase p)
{
	return (p < gamephase_middle_game);
}

////////////////////////////////////////////////////////////////////////////////
inline BitBoard ChessBoard::bitboardPiece(Side s, Piece p) const
{
	return bbPiece[s][p];
}

////////////////////////////////////////////////////////////////////////////////
inline BitBoard ChessBoard::bitboardColor(Side s) const
{
	return bbColorPiece[s];
}

////////////////////////////////////////////////////////////////////////////////
inline BitBoard ChessBoard::bitboardAll() const
{
	return bbAllPiece;
}

////////////////////////////////////////////////////////////////////////////////
inline BitBoard ChessBoard::bitboardPieceType(Piece p) const
{
	return (bbPiece[black][p] | bbPiece[white][p]);
}

////////////////////////////////////////////////////////////////////////////////
inline Piece ChessBoard::piece(Square sq) const
{
	return static_cast<Piece>(pieces[sq]);
}

////////////////////////////////////////////////////////////////////////////////
inline std::int32_t ChessBoard::materialScore(Side s) const
{
	return evalMaterial[s];
}

////////////////////////////////////////////////////////////////////////////////
inline MaterialMask ChessBoard::materialCount() const
{
	return matCount;
}

////////////////////////////////////////////////////////////////////////////////
inline Square ChessBoard::kingPosition(Side s) const
{
	return posKing[s];
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessBoard::canCastle(CastlingRight f) const
{
	return ((castle & f) != no_castle);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessBoard::isEnPassant() const
{
	return (enpassant != 0);
}

////////////////////////////////////////////////////////////////////////////////
inline File ChessBoard::enPassantFile() const
{
	return static_cast<File>(enpassant) - 1;
}

////////////////////////////////////////////////////////////////////////////////
inline Square ChessBoard::enPassantSquare(Side s) const
{
	return makeSquare(enPassantFile(), (s == white) ? rank_6 : rank_3);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessBoard::clearBoardPiece(Side s, Square sq, Piece pc)
{
	const BitBoard bb = ~BitBoards::square[sq];
	bbAllPiece &= bb;
	bbColorPiece[s] &= bb;
	bbPiece[s][pc] &= bb;
	pieces[sq] = static_cast<std::uint8_t>(no_piece);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessBoard::setBoardPiece(Side s, Square sq, Piece pc)
{
	const BitBoard bb = BitBoards::square[sq];
	bbAllPiece |= bb;
	bbColorPiece[s] |= bb;
	bbPiece[s][pc] |= bb;
	pieces[sq] = static_cast<std::uint8_t>(pc);
}

////////////////////////////////////////////////////////////////////////////////
inline ChessGamePhase ChessBoard::gamePhaseCount(Side s) const
{
	return phaseCount[s];
}

////////////////////////////////////////////////////////////////////////////////
inline ChessGamePhase ChessBoard::gamePhase() const
{
	return (phaseCount[black] + phaseCount[white]);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessBoard::evaluateDraw() const
{
	// no pawns and only one knight or bishop is draw
	return ((!havePawns(matCount)) &&
			((evalMaterial[black] + evalMaterial[white]) <=
				minor_piece_max_value));
}

////////////////////////////////////////////////////////////////////////////////
inline Square ChessBoard::squareKingStart(Side s) const
{
	return sqKingStart[s];
}

////////////////////////////////////////////////////////////////////////////////
inline BitBoard ChessBoard::bitboardCastleShort(Side s) const
{
	return squaresCastleShort[s];
}

////////////////////////////////////////////////////////////////////////////////
inline BitBoard ChessBoard::bitboardCastleLong(Side s) const
{
	return squaresCastleLong[s];
}

////////////////////////////////////////////////////////////////////////////////
inline Square ChessBoard::squareRookShort(Side s) const
{
	return sqCastleRookShort[s];
}

////////////////////////////////////////////////////////////////////////////////
inline Square ChessBoard::squareRookLong(Side s) const
{
	return sqCastleRookLong[s];
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessBoard::inCheck() const
{
	return isAttacked(kingPosition(player), ~player);
}

////////////////////////////////////////////////////////////////////////////////
inline HashKey ChessBoard::getHashKey() const
{
	return hashKey;
}

////////////////////////////////////////////////////////////////////////////////
inline HashKey ChessBoard::getHashKeyPawns() const
{
	return hashKeyPawns;
}

////////////////////////////////////////////////////////////////////////////////
inline Side ChessBoard::side() const
{
	return player;
}

////////////////////////////////////////////////////////////////////////////////
inline std::uint8_t ChessBoard::fiftyRule() const
{
	return fifty;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessBoard::getPly() const
{
	return ply;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessBoard::setPly(int p)
{
	ply = p;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessBoard::isPositionLegal() const
{
	return !isAttacked(kingPosition(~player), player);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessBoard::setChessRules(ChessRules r)
{
	rules = r;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessRules ChessBoard::getChessRules() const
{
	return rules;
}

extern const std::array<int, nb_pieces_type> piece_value_see;

} // namespace Cheese

#endif //CHEESE_BOARD_H_
