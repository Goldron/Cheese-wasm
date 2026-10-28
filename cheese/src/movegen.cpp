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

#include "movegen.h"

#include "bitboard.h"

namespace Cheese::ChessMoveGen {

////////////////////////////////////////////////////////////////////////////////
// generate castling moves
template<Side side>
void generateMovesCastling(
	const ChessBoard &board, ChessMoveSortList &moves)
{
	if (!board.isAttacked(board.kingPosition(side), ~side)) {

		static constexpr CastlingRight csflag1 = (side == white) ? 
			castle_white_short : castle_black_short;
		static constexpr Square sqmax1 = (side == white) ? g1 : g8;

		if (board.canCastle(csflag1) &&
			(!(board.bitboardAll() & board.bitboardCastleShort(side)))) {

			bool safe = true;
			for (Square i=board.squareKingStart(side) + dir_right; i<=sqmax1; 
				++i) {
				if (board.isAttacked(i, ~side)) {
					safe = false;
					break;
				}
			}
			if (safe) {
				moves.emplace_back(board.squareKingStart(side), 
					board.squareRookShort(side), king, no_piece, no_piece, 1);
			}
		}

		static constexpr CastlingRight csflag2 = (side == white) ? 
			castle_white_long : castle_black_long;
		static constexpr Square sqmax2 = (side == white) ? c1 : c8;

		if (board.canCastle(csflag2) &&
			(!(board.bitboardAll() & board.bitboardCastleLong(side)))) {

			bool safe = true;
			for (Square i=sqmax2; i<board.squareKingStart(side); ++i) {
				if (board.isAttacked(i, ~side)) {
					safe = false;
					break;
				}
			}
			if (safe) {
				moves.emplace_back(board.squareKingStart(side),
					board.squareRookLong(side), king, no_piece, no_piece, 1);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
template<Side side, bool capture>
void generateMovesPieces(const ChessBoard &board,
	ChessMoveSortList &moves, BitBoard bbmask)
{
	BitBoard bbSrc = board.bitboardPiece(side, knight);

	BitBoards::scan(bbSrc, [&](Square src) {

		BitBoard bbDst = BitBoards::attackKnight(src) & bbmask;

		BitBoards::scan(bbDst, [&](Square dst) {
			if constexpr (capture) {
				moves.emplace_back(src, dst, knight, board.piece(dst));
			} else {
				moves.emplace_back(src, dst, knight);
			}
		});
	});

	bbSrc = board.bitboardPiece(side, bishop);

	BitBoards::scan(bbSrc, [&](Square src) {

		BitBoard bbDst = BitBoards::attackBishop(src, board.bitboardAll()) 
			& bbmask;

		BitBoards::scan(bbDst, [&](Square dst) {
			if constexpr (capture) {
				moves.emplace_back(src, dst, bishop, board.piece(dst));
			} else {
				moves.emplace_back(src, dst, bishop);
			}
		});
	});

	bbSrc = board.bitboardPiece(side, rook);

	BitBoards::scan(bbSrc, [&](Square src) {

		BitBoard bbDst = BitBoards::attackRook(src, board.bitboardAll()) 
			& bbmask;

		BitBoards::scan(bbDst, [&](Square dst) {
			if constexpr (capture) {
				moves.emplace_back(src, dst, rook, board.piece(dst));
			} else {
				moves.emplace_back(src, dst, rook);
			}
		});
	});

	bbSrc = board.bitboardPiece(side, queen);

	BitBoards::scan(bbSrc, [&](Square src) {

		BitBoard bbDst = BitBoards::attackQueen(src, board.bitboardAll()) 
			& bbmask;

		BitBoards::scan(bbDst, [&](Square dst) {
			if constexpr (capture) {
				moves.emplace_back(src, dst, queen, board.piece(dst));
			} else {
				moves.emplace_back(src, dst, queen);
			}
		});
	});
}

////////////////////////////////////////////////////////////////////////////////
template<Side side, bool capture>
void generateMovesKing(const ChessBoard &board, ChessMoveSortList &moves, 
	BitBoard bbmask)
{
	Square src = board.kingPosition(side);
	BitBoard bbDst = BitBoards::attackKing(src) & bbmask;

	BitBoards::scan(bbDst, [&](Square dst) {
		if constexpr (capture) {
			moves.emplace_back(src, dst, king, board.piece(dst));
		} else {
			moves.emplace_back(src, dst, king);
		}
	});
}

////////////////////////////////////////////////////////////////////////////////
template<Side side, bool evasion>
void generateMovesPawn(const ChessBoard &board,
	ChessMoveSortList &moves, BitBoard bbmask)
{
	// 1 up
	static constexpr Rank r7 = relRank<side>(rank_7);
	BitBoard bbDst = BitBoards::shiftUp<side>(board.bitboardPiece(side, pawn) &
					~BitBoards::rank[r7]) & ~board.bitboardAll();
	if constexpr (evasion) {
		bbDst &= bbmask;
	}

	BitBoards::scan(bbDst, [&](Square dst) {
		moves.emplace_back(relSquareDown<side>(dst), dst, pawn);
	});

	// 2 up
	static constexpr Rank r2 = relRank<side>(rank_2);
	bbDst = BitBoards::shiftUp2<side>(board.bitboardPiece(side, pawn) &
			BitBoards::rank[r2]) & (~(board.bitboardAll() |
			BitBoards::shiftUp<side>(board.bitboardAll())));
	if constexpr (evasion) {
		bbDst &= bbmask;
	}

	BitBoards::scan(bbDst, [&](Square dst) {
		moves.emplace_back(relSquareDown2<side>(dst), dst, pawn);
	});
}

////////////////////////////////////////////////////////////////////////////////
template<Side side>
void generateCapturesPawn(
	const ChessBoard &board, ChessMoveSortList &moves, BitBoard bbmask)
{
	// attack
	static constexpr Rank r = relRank<side>(rank_7);
	const BitBoard bb = board.bitboardPiece(side, pawn) & ~BitBoards::rank[r];

	BitBoard bbDst = BitBoards::allPawnsAttacksLeft<side>(bb) & bbmask;

	BitBoards::scan(bbDst, [&](Square dst) {
		moves.emplace_back(relSquareDownRight<side>(dst), dst, pawn,
			board.piece(dst));
	});

	bbDst = BitBoards::allPawnsAttacksRight<side>(bb) & bbmask;

	BitBoards::scan(bbDst, [&](Square dst) {
		moves.emplace_back(relSquareDownLeft<side>(dst), dst, pawn,
			board.piece(dst));
	});

	// en passant
	if (board.isEnPassant()) {

		const Square sqep = board.enPassantSquare(side);

		if (BitBoards::square[relSquareDown<side>(sqep)] & bbmask) {
			BitBoard bbSrc = board.bitboardPiece(side, pawn) &
					BitBoards::attackPawn(~side, sqep);
			BitBoards::scan(bbSrc, [&](Square src) {
				moves.emplace_back(src, sqep, pawn, pawn, no_piece, 0, 1);
			});
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
template<Side side, GenPromotion mode>
void generateMovesPromotions(
	const ChessBoard &board, ChessMoveSortList &moves, BitBoard bbmask)
{
	static constexpr Rank r = relRank<side>(rank_7);
	BitBoard bbDst = BitBoards::shiftUp<side>(
						board.bitboardPiece(side, pawn) &
						BitBoards::rank[r]) & bbmask;
	BitBoards::scan(bbDst, [&](Square dst) {

		Square src = relSquareDown<side>(dst);

		if constexpr ((mode == GenPromotion::all) ||
			(mode == GenPromotion::queen)) {
			moves.emplace_back(src, dst, pawn, no_piece, queen);
		}
		if constexpr ((mode == GenPromotion::all) ||
			(mode == GenPromotion::under)) {
			moves.emplace_back(src, dst, pawn, no_piece, rook);
			moves.emplace_back(src, dst, pawn, no_piece, knight);
			moves.emplace_back(src, dst, pawn, no_piece, bishop);
		}
	});
}

////////////////////////////////////////////////////////////////////////////////
template<Side side, GenPromotion mode>
void generateCapturesPromotions(
	const ChessBoard &board, ChessMoveSortList &moves, BitBoard bbmask)
{
static constexpr Rank r = relRank<side>(rank_7);
	const BitBoard bb = board.bitboardPiece(side, pawn) & BitBoards::rank[r];

	BitBoard bbDst = BitBoards::allPawnsAttacksLeft<side>(bb) & bbmask;

	BitBoards::scan(bbDst, [&](Square dst) {

		Square src = relSquareDownRight<side>(dst);

		if constexpr ((mode == GenPromotion::all) ||
			(mode == GenPromotion::queen)) {
			moves.emplace_back(src, dst, pawn, board.piece(dst), queen);
		}
		if constexpr ((mode == GenPromotion::all) ||
			(mode == GenPromotion::under)) {
			moves.emplace_back(src, dst, pawn, board.piece(dst), rook);
			moves.emplace_back(src, dst, pawn, board.piece(dst), knight);
			moves.emplace_back(src, dst, pawn, board.piece(dst), bishop);
		}
	});

	bbDst = BitBoards::allPawnsAttacksRight<side>(bb) & bbmask;

	BitBoards::scan(bbDst, [&](Square dst) {

		Square src = relSquareDownLeft<side>(dst);

		if constexpr ((mode == GenPromotion::all) ||
			(mode == GenPromotion::queen)) {
			moves.emplace_back(src, dst, pawn, board.piece(dst), queen);
		}
		if constexpr ((mode == GenPromotion::all) ||
			(mode == GenPromotion::under)) {
			moves.emplace_back(src, dst, pawn, board.piece(dst), rook);
			moves.emplace_back(src, dst, pawn, board.piece(dst), knight);
			moves.emplace_back(src, dst, pawn, board.piece(dst), bishop);
		}
	});
}

////////////////////////////////////////////////////////////////////////////////
// generate non capture moves (and no promotion), and castling
template<Side side>
void generateAllQuiets(const ChessBoard &board,
	ChessMoveSortList &moves)
{
	generateMovesCastling<side>(board, moves);

	BitBoard bb = ~board.bitboardAll();

	generateMovesPieces<side, false>(board, moves, bb);
	generateMovesKing<side, false>(board, moves, bb);
	generateMovesPawn<side, false>(board, moves, 0);
}

////////////////////////////////////////////////////////////////////////////////
// generate captures moves + promotion moves
template<Side side>
void generateMovesCaptureAll(const ChessBoard &board,
	ChessMoveSortList &moves)
{
	const BitBoard bb = board.bitboardColor(~side);

	generateMovesPieces<side, true>(board, moves, bb);
	generateMovesKing<side, true>(board, moves, bb);
	generateCapturesPawn<side>(board, moves, bb);
	generateCapturesPromotions<side, GenPromotion::all>(board,
		moves, bb);
	generateMovesPromotions<side, GenPromotion::all>(board, moves,
			~board.bitboardAll());
}

////////////////////////////////////////////////////////////////////////////////
// generate captures moves + promotion moves for queen only
template<Side side>
void generateMovesCaptureQuiescenceAll(const ChessBoard &board,
	ChessMoveSortList &moves)
{
	const BitBoard bb = board.bitboardColor(~side);

	generateMovesPieces<side, true>(board, moves, bb);
	generateMovesKing<side, true>(board, moves, bb);
	generateCapturesPawn<side>(board, moves, bb);
	generateCapturesPromotions<side, GenPromotion::queen>(board,
		moves, bb);
	generateMovesPromotions<side, GenPromotion::queen>(board, moves,
			~board.bitboardAll());
}

////////////////////////////////////////////////////////////////////////////////
// generate non capture moves (and no promotion), and castling
void generateQuiets(const ChessBoard &board, ChessMoveSortList &moves)
{
	if (board.side() == black) {
		generateAllQuiets<black>(board, moves);
	} else {
		generateAllQuiets<white>(board, moves);
	}
}

////////////////////////////////////////////////////////////////////////////////
// generate captures moves + promotion moves
void generateCaptures(const ChessBoard &board, ChessMoveSortList &moves)
{
	if (board.side() == black) {
		generateMovesCaptureAll<black>(board, moves);
	} else {
		generateMovesCaptureAll<white>(board, moves);
	}
}

////////////////////////////////////////////////////////////////////////////////
// generate captures moves + promotion moves for queen only
void generateQuietCaptures(const ChessBoard &board,
	ChessMoveSortList &moves)
{
	if (board.side() == black) {
		generateMovesCaptureQuiescenceAll<black>(board, moves);
	} else {
		generateMovesCaptureQuiescenceAll<white>(board, moves);
	}
}

////////////////////////////////////////////////////////////////////////////////
// generate all legal moves when king must evade check
template<GenPromotion mode>
void generateKingEvasions(const ChessBoard &board, ChessMoveSortList &moves)
{
const Side notplayer = ~board.side();

	const Square src = board.kingPosition(board.side());

	// create a bitboard with pieces attacking the king
	BitBoard bbAtt = (BitBoards::attackPawn(board.side(), src) &
						board.bitboardPiece(notplayer, pawn)) |
					 (BitBoards::attackKnight(src) &
						board.bitboardPiece(notplayer, knight)) |
					 (BitBoards::attackRook(src, board.bitboardAll()) &
						(board.bitboardPiece(notplayer, rook) |
							board.bitboardPiece(notplayer, queen))) |
					 (BitBoards::attackBishop(src, board.bitboardAll()) &
						(board.bitboardPiece(notplayer, bishop) |
							board. bitboardPiece(notplayer, queen)));

	// * king move to evade check (by moving or attacking)
	//	- only solution when there are more than one attacker
	BitBoard bbDst = BitBoards::attackKing(src) &
					~(board.bitboardColor(board.side()));
	BitBoard bbAll = board.bitboardAll() ^
					BitBoards::square[board.kingPosition(board.side())];

	BitBoards::scan(bbDst, [&](Square dst) {
		if (!board.isAttackedOcc(dst, notplayer, bbAll)) {
			moves.emplace_back(src, dst, king, board.piece(dst));
		}
	});

	// more than one attacker, the king must move
	int nbattacker = BitBoards::bitCount(bbAtt);
	if (nbattacker != 1) {
		return;
	}

	// get attacker type and position :
	Square attdst = BitBoards::getFirstSquare(bbAtt);

	// move a piece between sliding attacker and king, or capture the attacker

	// bitboard of squares between attacker and king + the attacker
	BitBoard bbAttKing =
		BitBoards::between[board.kingPosition(board.side())][attdst];

	BitBoard bbmask = ~(board.bitboardColor(board.side())) &
		(bbAttKing | bbAtt);

	if (board.side() == black) {
		generateMovesPieces<black, true>(board, moves, bbmask);

		generateCapturesPawn<black>(board, moves, bbAtt);
		generateCapturesPromotions<black, mode>(board, moves, bbAtt);

		generateMovesPawn<black, true>(board, moves, bbAttKing);
		generateMovesPromotions<black, mode>(board, moves,
				~board.bitboardAll() & bbAttKing);
	} else {
		generateMovesPieces<white, true>(board, moves, bbmask);

		generateCapturesPawn<white>(board, moves, bbAtt);
		generateCapturesPromotions<white, mode>(board, moves, bbAtt);

		generateMovesPawn<white, true>(board, moves, bbAttKing);
		generateMovesPromotions<white, mode>(board, moves,
				~board.bitboardAll() & bbAttKing);
	}
}

////////////////////////////////////////////////////////////////////////////////
// generate all legal moves when king must evade check
void generateKingEvasions(const ChessBoard &board, ChessMoveSortList &moves)
{
	generateKingEvasions<GenPromotion::all>(board, moves);
}

////////////////////////////////////////////////////////////////////////////////
// generate all legal moves when king must evade check
void generateQuietKingEvasions(const ChessBoard &board, ChessMoveSortList &moves)
{
	generateKingEvasions<GenPromotion::queen>(board, moves);
}

////////////////////////////////////////////////////////////////////////////////
// Generate non capture moves that give check
template<Side side>
void generateQuietChecks(const ChessBoard &board, ChessMoveSortList &moves)
{
	// discovered checks
	// moving one of our piece pinned on ennemy king

	// pinned piece will not give direct check in 2nd part
	BitBoard bbPinned = 0ULL;

	const Square pk = board.kingPosition(~side);

	// possible pinner rook & bishop & queen
	BitBoard bbPinner = (BitBoards::file[squareFile(pk)] |
						BitBoards::rank[squareRank(pk)]) &
						(board.bitboardPiece(side, rook) |
						board.bitboardPiece(side, queen));
	bbPinner |= (BitBoards::diag_45r[pk] | BitBoards::diag_45l[pk]) &
				(board.bitboardPiece(side, bishop) |
				board.bitboardPiece(side, queen));

	BitBoards::scan(bbPinner, [&](Square pp) {

		// path between pinner and king
		BitBoard bbp = BitBoards::between[pk][pp];

		// pinned pieces
		BitBoard bb = bbp & board.bitboardAll();

		// one piece behind king and pinner
		if ((bb) && (!BitBoards::bitCountMoreThanOne(bb))) {

			bb = bbp & board.bitboardColor(side);
			if (bb) {

				Square src = BitBoards::getFirstSquare(bb);

				BitBoard bbAtt = ~(bbp | board.bitboardAll());

				BitBoard bbDst;

				switch (board.piece(src)) {

					case	pawn:

							// 1 up
							bbDst = BitBoards::shiftUp<side>(bb &
								~BitBoards::rank[relRank<side>(rank_7)]) &
								bbAtt;
							if (bbDst) {
								moves.emplace_back(src, relSquareUp<side>(src), 
									pawn);
							}

							// 2 up
							bbDst = BitBoards::shiftUp2<side>(bb &
								BitBoards::rank[relRank<side>(rank_2)]) &
								bbAtt & ~(BitBoards::shiftUp<side>(
								board.bitboardAll()));
							if (bbDst) {
								moves.emplace_back(src, relSquareUp2<side>(src), 
									pawn);
							}
							bbPinned |= bb;
							break;

					case	knight:
							bbDst = BitBoards::attackKnight(src) & ~bbp &
								~board.bitboardAll();
							BitBoards::scan(bbDst, [&](Square dst) {
								moves.emplace_back(src, dst, knight);
							});
							bbPinned |= bb;
							break;

					case	bishop:
							bbDst = BitBoards::attackBishop(src,
								board.bitboardAll()) & bbAtt;
							BitBoards::scan(bbDst, [&](Square dst) {
								moves.emplace_back(src, dst, bishop);
							});
							bbPinned |= bb;
							break;

					case	rook:
							bbDst = BitBoards::attackRook(src,
								board.bitboardAll()) & bbAtt;
							BitBoards::scan(bbDst, [&](Square dst) {
								moves.emplace_back(src, dst, rook);
							});
							bbPinned |= bb;
							break;

					case	queen:
							bbDst = BitBoards::attackQueen(src,
								board.bitboardAll()) & bbAtt;
							BitBoards::scan(bbDst, [&](Square dst) {
								moves.emplace_back(src, dst, queen);
							});
							bbPinned |= bb;
							break;

					case	king:
							bbDst = BitBoards::attackKing(src) & bbAtt;
							BitBoards::scan(bbDst, [&](Square dst) {
								moves.emplace_back(src, dst, king);
							});
							bbPinned |= bb;
							break;

					default:
						break;
				}
			}
		}
	});

	// direct checks

	// knights
	BitBoard bbAtt = BitBoards::attackKnight(pk) & ~board.bitboardAll();
	BitBoard bbSrc = board.bitboardPiece(side, knight) & ~bbPinned;

	BitBoards::scan(bbSrc, [&](Square src) {

		BitBoard bbDst = BitBoards::attackKnight(src) & bbAtt;

		BitBoards::scan(bbDst, [&](Square dst) {
			moves.emplace_back(src, dst, knight);
		});
	});

	// bishops
	bbAtt = BitBoards::attackBishop(pk, board.bitboardAll()) &
			~board.bitboardAll();
	bbSrc = board.bitboardPiece(side, bishop) & ~bbPinned;

	BitBoards::scan(bbSrc, [&](Square src) {

		BitBoard bbDst = BitBoards::attackBishop(src,
			board.bitboardAll()) & bbAtt;

		BitBoards::scan(bbDst, [&](Square dst) {
			moves.emplace_back(src, dst, bishop);
		});
	});

	// rooks
	bbAtt = BitBoards::attackRook(pk, board.bitboardAll()) &
			~board.bitboardAll();
	bbSrc = board.bitboardPiece(side, rook) & ~bbPinned;

	BitBoards::scan(bbSrc, [&](Square src) {

		BitBoard bbDst = BitBoards::attackRook(src,
				board.bitboardAll()) & bbAtt;

		BitBoards::scan(bbDst, [&](Square dst) {
			moves.emplace_back(src, dst, rook);
		});
	});

	// queens
	bbAtt = BitBoards::attackQueen(pk, board.bitboardAll()) &
			~board.bitboardAll();
	bbSrc = board.bitboardPiece(side, queen) & ~bbPinned;

	BitBoards::scan(bbSrc, [&](Square src) {

		BitBoard bbDst = BitBoards::attackQueen(src, board.bitboardAll()) &
				bbAtt;

		BitBoards::scan(bbDst, [&](Square dst) {
			moves.emplace_back(src, dst, queen);
		});
	});

	// pawns

	bbAtt = BitBoards::attackPawn(~side, pk) & ~board.bitboardAll();

	// 1 up
	bbSrc = BitBoards::shiftUp<side>(board.bitboardPiece(side, pawn) &
			~BitBoards::rank[relRank<side>(rank_7)]) & bbAtt & ~bbPinned;

	BitBoards::scan(bbSrc, [&](Square src) {
		moves.emplace_back(relSquareDown<side>(src) , src, pawn);
	});

	// 2 up
	bbSrc = BitBoards::shiftUp2<side>(board.bitboardPiece(side, pawn) &
			BitBoards::rank[relRank<side>(rank_2)]) & bbAtt &
			~BitBoards::shiftUp<side>(board.bitboardAll()) & ~bbPinned;

	BitBoards::scan(bbSrc, [&](Square src) {
		moves.emplace_back(relSquareDown2<side>(src), src, pawn);
	});

	// TODO : castle : rook can check the ennemy king
}

////////////////////////////////////////////////////////////////////////////////
// Generate non capture moves that give check
void generateQuietChecks(const ChessBoard &board, ChessMoveSortList &moves)
{
	if (board.side() == black) {
		generateQuietChecks<black>(board, moves);
	} else {
		generateQuietChecks<white>(board, moves);
	}
}

////////////////////////////////////////////////////////////////////////////////
// generate all possible moves (pseudo legal moves)
void generateMoves(const ChessBoard &board, ChessMoveSortList &moves,
	bool incheck)
{
	if (incheck) {
		generateKingEvasions(board, moves);
	} else {
		generateCaptures(board, moves);
		generateQuiets(board, moves);
	}
}

////////////////////////////////////////////////////////////////////////////////
// count legal moves
int countLegalMoves(ChessBoard &board)
{
	ChessMoveSortList mlist;
	generateMoves(board, mlist, board.inCheck());

	int nblegalmoves = 0;

	for (const auto &m : mlist) {
		const ChessMoveRec rec = board.makeMove(m.move);
		if (board.isPositionLegal()) {
			++nblegalmoves;
		}
		board.unMakeMove(m.move, rec);
	}

	return nblegalmoves;
}

////////////////////////////////////////////////////////////////////////////////
// generate legal moves
void generateLegalMoves(ChessBoard &board, ChessMoveSortList &moves)
{
	ChessMoveSortList mlist;

	moves.clear();
	generateMoves(board, mlist, board.inCheck());

	for (const auto &m : mlist) {
		const ChessMoveRec rec = board.makeMove(m.move);
		if (board.isPositionLegal()) {
			moves.push(m.move);
		}
		board.unMakeMove(m.move, rec);		
	}
}

} // namespace Cheese::ChessMoveGen

