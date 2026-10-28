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

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "bitboard.h"
#include "board.h"
#include "endgame.h"
#include "engine.h"
#include "eval.h"
#include "fen.h"
#include "movegen.h"
#include "moveorder.h"
#include "moveparser.h"
#include "tools.h"

#include "util/tokenizer.h"

using namespace Cheese;

// TODO : test tablebases, FRC castling, repetitions, 50 moves rule...

////////////////////////////////////////////////////////////////////////////////
// test basic move functions
TEST(ChessMove, TestMoveFunctions) {

	EXPECT_EQ(makeSquare(file_e, rank_4), e4);

	EXPECT_EQ(flipSquareFile(c3), f3);
	EXPECT_EQ(flipSquareRank(c3), c6);

	EXPECT_EQ(squareRank(b5), rank_5);
	EXPECT_EQ(squareFile(b5), file_b);

	EXPECT_EQ(fileDistance(g2, b7), 5);
	EXPECT_EQ(rankDistance(e4, f5), 1);
	EXPECT_EQ(squareDistance(g8, a1), 7);

	EXPECT_EQ(relSquareUp<white>(e4), e5);
	EXPECT_EQ(relSquareUp<black>(e5), e4);
	
	EXPECT_EQ(relSquareDown<white>(e4), e3);
	EXPECT_EQ(relSquareDown<black>(e5), e6);

	EXPECT_EQ(relSquareUp2<white>(e2), e4);
	EXPECT_EQ(relSquareUp2<black>(d7), d5);
	
	EXPECT_EQ(relSquareDown2<white>(e4), e2);
	EXPECT_EQ(relSquareDown2<black>(d5), d7);
	
	EXPECT_EQ(relSquareUpLeft<white>(e4), d5);
	EXPECT_EQ(relSquareUpLeft<black>(e5), f4);
	
	EXPECT_EQ(relSquareUpRight<white>(e4), f5);
	EXPECT_EQ(relSquareUpRight<black>(e5), d4);
	
	EXPECT_EQ(relSquareRank<white>(e4), rank_4);
	EXPECT_EQ(relSquareRank<black>(e5), rank_4);

	EXPECT_EQ(isMoveTwiceUp(e2, e4), true);
	EXPECT_EQ(isMoveTwiceUp(a2, a3), false);
}

////////////////////////////////////////////////////////////////////////////////
// test chess move format
TEST(ChessMove, TestChessMove) {
	for (Square src=a1; src<=h8; ++src) {
		for (Square dst=a1; dst<=h8; ++dst) {
			for (Piece mv=pawn; mv<=king; ++mv) {
				for (Piece cap=no_piece; cap<=king; ++cap) {
					for (Piece pro=knight; pro<=queen; ++pro) {
						for (int cas=0; cas<2; cas++) {
							for (int ep=0; ep<2; ep++) {
								Piece promote = (mv == pawn) ? pro : no_piece;
								ChessMove move(src, dst, mv,cap, promote, cas, ep);
								EXPECT_EQ(move.src(), src);
								EXPECT_EQ(move.dst(), dst);
								EXPECT_EQ(move.piece(), mv);
								EXPECT_EQ(move.capture(), cap);
								EXPECT_EQ(move.promotion(), promote);
								EXPECT_EQ(move.isCastle(), (cas == 1));
								EXPECT_EQ(move.isCapture(), (cap != no_piece));
								EXPECT_EQ(move.isCaptureOrPromotion(), 
									(cap != no_piece) || (promote != no_piece));
								EXPECT_EQ(move.isEnPassant(), (ep == 1));
							}
						}
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// test basic bitboards functions
TEST(BitBoards, TestBitBoards) {
	EXPECT_EQ(BitBoards::bitCount(0ULL), 0);
	EXPECT_EQ(BitBoards::bitCount(0xFFFFFFFFFFFFFFFFULL), 64);

	EXPECT_EQ(BitBoards::getFirstBit(3ULL), 0);
	EXPECT_EQ(BitBoards::getFirstBit(0x8000000000000000ULL), 63);	
	EXPECT_EQ(BitBoards::getFirstBit(0x80000000ULL), 31);

	EXPECT_EQ(BitBoards::getLastBit(3ULL), 1);
	EXPECT_EQ(BitBoards::getLastBit(0x8000000000000000ULL), 63);
	EXPECT_EQ(BitBoards::getLastBit(0x80000000ULL), 31);				

	EXPECT_EQ(BitBoards::isolateFirstBit(0xFFFFFFFF80000000ULL), 0x80000000ULL);
	
	EXPECT_EQ(BitBoards::shiftUp<white>(BitBoards::square[e2]), 
		   BitBoards::square[e3]);
	EXPECT_EQ(BitBoards::shiftUp2<white>(BitBoards::square[e2]), 
		   BitBoards::square[e4]);
	EXPECT_EQ(BitBoards::shiftUpLeft<white>(BitBoards::square[e2]), 
		   BitBoards::square[d3]);
	EXPECT_EQ(BitBoards::shiftUpRight<white>(BitBoards::square[e2]), 
		   BitBoards::square[f3]);

	EXPECT_EQ(BitBoards::bitCountMoreThanOne(0xFFFFFFFFFFFFFFFFULL), true);
	EXPECT_EQ(BitBoards::bitCountMoreThanOne(0x1000000000000001ULL), true);
	EXPECT_EQ(BitBoards::bitCountMoreThanOne(0x1000000000000000ULL), false);

	const BitBoard bb1 = BitBoards::square[e2] | BitBoards::square[e3] |
							 BitBoards::square[e4] | BitBoards::square[e5] |
							 BitBoards::square[e6] | BitBoards::square[e7] |
							 BitBoards::square[e8];
	EXPECT_EQ(BitBoards::fillFront<white>(BitBoards::square[e2]), bb1);
	EXPECT_EQ(BitBoards::fillBack<black>(BitBoards::square[e2]), bb1);

	const BitBoard bb2 = BitBoards::square[d7] | BitBoards::square[d6] |
							 BitBoards::square[d5] | BitBoards::square[d4] |
							 BitBoards::square[d3] | BitBoards::square[d2] |
							 BitBoards::square[d1];
	EXPECT_EQ(BitBoards::fillBack<white>(BitBoards::square[d7]), bb2);
	EXPECT_EQ(BitBoards::fillFront<black>(BitBoards::square[d7]), bb2);

	const BitBoard bb3 = BitBoards::square[d2] | BitBoards::square[e7] | 
					   BitBoards::square[a3] | BitBoards::square[h6];

	EXPECT_EQ(BitBoards::allPawnsAttacks<white>(bb3), 
		BitBoards::square[c3] | BitBoards::square[e3] | 
		BitBoards::square[d8] | BitBoards::square[f8] | 
		BitBoards::square[b4] | BitBoards::square[g7]);

	EXPECT_EQ(BitBoards::allPawnsAttacks<black>(bb3), 
		BitBoards::square[c1] | BitBoards::square[e1] | 
		BitBoards::square[d6] | BitBoards::square[f6] | 
		BitBoards::square[b2] | BitBoards::square[g5]);

	const BitBoard bb4 = BitBoards::square[g3] | BitBoards::square[g4] |  
					   BitBoards::square[g6] | BitBoards::square[g5];

	EXPECT_EQ(BitBoards::getNearestSquare(white, bb4), g3);
	EXPECT_EQ(BitBoards::getFarthestSquare(white, bb4), g6);
	EXPECT_EQ(BitBoards::getNearestSquare(black, bb4), g6);
	EXPECT_EQ(BitBoards::getFarthestSquare(black, bb4), g3);
}

////////////////////////////////////////////////////////////////////////////////
TEST(ChessBoard, TestMaterialMask) {
	
	const auto m1 = makeMaterialMask("KNKB");
	EXPECT_EQ(countMaterial(m1, black, bishop), 1);
	EXPECT_EQ(countMaterial(m1, white, knight), 1);

	const auto m2 = makeMaterialMask(black, pawn, 15) |
					makeMaterialMask(white, pawn, 15) |
					makeMaterialMask(black, bishop, 15) |
					makeMaterialMask(white, bishop, 15) |
					makeMaterialMask(black, queen, 15) |
					makeMaterialMask(white, queen, 15);
	EXPECT_EQ(countMaterial(m2, black, pawn), 15);
	EXPECT_EQ(countMaterial(m2, white, pawn), 15);
	EXPECT_EQ(countMaterial(m2, black, bishop), 15);
	EXPECT_EQ(countMaterial(m2, white, bishop), 15);
	EXPECT_EQ(countMaterial(m2, black, knight), 0);
	EXPECT_EQ(countMaterial(m2, white, knight), 0);
	EXPECT_EQ(countMaterial(m2, black, queen), 15);
	EXPECT_EQ(countMaterial(m2, white, queen), 15);
	
	const auto m3 = makeMaterialMask(white, pawn, 2);
	EXPECT_EQ(havePawns(m3), true);

	const auto m4 = makeMaterialMask(black, pawn, 2);
	EXPECT_EQ(havePawns(m4), true);

	const auto m5 = makeMaterialMask(black, knight, 15) |
					makeMaterialMask(white, knight, 15);
	EXPECT_EQ(havePawns(m5), false);
}

////////////////////////////////////////////////////////////////////////////////
TEST(ChessBoard, TestEndgame) 
{
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();

	constexpr auto mat_kpk = makeMaterialMask("KPK");
	bool rf = engine.setFENPosition("4k3/8/8/8/P7/8/8/K7 w - - 0 1 ");
	EXPECT_EQ(rf, true);

	EXPECT_EQ(board.materialCount(), mat_kpk);
}

////////////////////////////////////////////////////////////////////////////////
// test move generation
TEST(MoveGen, TestMoveGen1) {
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();
	ChessMoveSortList moves;

	const std::vector<std::pair<std::string, int>> positions = {
		{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ", 20},
		{"R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1 w - - 0 1 ", 218},
		{"3qr1Qk/pbpp2pp/1p5N/6b1/2P1P3/P7/1PP2PPP/R4RK1 b - - 1 1 ", 1},
		{"rnb1k1nr/ppp2ppp/8/8/8/2N3b1/PPPPP3/R1BQKBNR w KQkq - 0 1 ", 0}
	};

	for (const auto & [fen, count] : positions) {
		bool rf = engine.setFENPosition(fen);
		EXPECT_EQ(rf, true);
		ChessMoveGen::generateLegalMoves(board, moves);
		EXPECT_EQ(moves.size(), count);
	}
}

////////////////////////////////////////////////////////////////////////////////
// test fen reverse function 
TEST(FEN, TestFENReverse) {
	ChessEngine engine;
	engine.init(1);	
	
    const std::vector<std::pair<std::string, std::string>> positions = {
		{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ",
		 "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1 "},
		 		
		{"r4rk1/1b1n1pb1/3p2p1/1p1Pq1Bp/2p1P3/2P2RNP/1PBQ2P1/5R1K w - - 0 1 ",
		 "5r1k/1pbq2p1/2p2rnp/2P1p3/1P1pQ1bP/3P2P1/1B1N1PB1/R4RK1 b - - 0 1 "},

		{"8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - ",
         "k7/8/8/p2p1p2/P2p1P2/3P4/K7/8 b - - "},
		 
		{"r1bqkbr1/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/1RBQKB1R w Kq - 0 1 ",		 
		 "1rbqkb1r/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/R1BQKBR1 b Qk - 0 1 "}
	};
	
	for (const auto &p : positions) {
		std::string strtmp;
		bool r = ChessFENParser::reverse(p.first, strtmp);
		EXPECT_EQ(r, true);
		EXPECT_EQ(strtmp, p.second);
		r = ChessFENParser::reverse(p.second, strtmp);
		EXPECT_EQ(r, true);
		EXPECT_EQ(strtmp, p.first);
	}	
}

////////////////////////////////////////////////////////////////////////////////
TEST(FEN, TestFENGet) {
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();
	
	const std::vector<std::string> positions = {
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"r4rk1/1b1n1pb1/3p2p1/1p1Pq1Bp/2p1P3/2P2RNP/1PBQ2P1/5R1K w - - 0 1", 
		"r1bqkbr1/pppp1ppp/2n2n2/4p3/4P3/2N2N2/PPPP1PPP/1RBQKB1R w Kq - 0 1",
		"1r2rbk1/1p1n1p2/p3b1p1/q2NpNPp/4P2Q/1P5R/6BP/5R1K w - h6 0 1"
	};

	for (const auto &p : positions) {
		bool rf = engine.setFENPosition(p);
		EXPECT_EQ(rf, true);
		std::string tmp = ChessFENParser::get(board);
		EXPECT_EQ(p, tmp);
	}

	const std::vector<std::string> badfens = {
		"rnbqkbn1/ppppppppr/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR K KQkq - 0 1",
		"rnbqkbnr.pppppppp.8.8.8.8.PPPPPPPP.RNBQKBNR w KQkq - 0 1",
		"rnbqkbnr/ppppippp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"rnbqkbnr/pppppppp/8/6/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"N7/8/2KQ2rp/6k1/4p3p/2p4P/4PP2/5N2 w - - 0 1",
		"rnkqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"rnbq1bnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
		"pnbqkbnr/ppprpppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
	};

	for (const auto &p : badfens) {
		bool rf = engine.setFENPosition(p);
		EXPECT_EQ(rf, false);
	}
}

////////////////////////////////////////////////////////////////////////////////
// test staged move generation
TEST(MoveOrder, TestMoveOrder1) {
	
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();
	
	ChessMoveSortList moves;
	
	const std::string fen = 
		"r4rk1/1b1n1pb1/3p2p1/1p1Pq1Bp/2p1P3/2P2RNP/1PBQ2P1/5R1K w - - 0 1 ";
	bool rf = engine.setFENPosition(fen);
	EXPECT_EQ(rf, true);
	ChessMoveGen::generateLegalMoves(board, moves);
		
	ChessMoveSortList movelist;
	ChessMoveOrder moveorder( 
		engine.getThreads().getMainThread()->history, movelist, 
		MoveOrderPhase::hash,  
		ChessMove(g3, f5, knight, no_piece, no_piece, 0, 0));
		
	ChessMoveSortList moves2;
	ChessMove move;
	
	while ((move = moveorder.getNextMove(board)) != ChessMove(0)) {
		
		const ChessMoveRec rec = board.makeMove(move);

		if (!board.isPositionLegal()) {
			board.unMakeMove(move, rec);
			continue;
		}

		moves2.push(move);
		
		board.unMakeMove(move, rec);
	}
	
	EXPECT_EQ(moves.size(), moves2.size());
	
	for (const auto &m1 : moves) {
		bool f = false;
		for (const auto &m2 : moves2) {		
			if (m2.move == m1.move) {
				f = true;
			}
		}
		EXPECT_EQ(f, true);
	}
}

////////////////////////////////////////////////////////////////////////////////
// test staged move generation phases
TEST(MoveOrder, TestMoveOrder2) {
	
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();
	
	ChessMoveSortList moves;
		
	const std::vector<std::tuple<std::string, ChessMove, ChessMove,
		ChessMove, ChessMove, MoveOrderPhase, 
		std::vector<std::pair<MoveOrderPhase, int>>>> positions = {
		{
			"8/5pk1/p4npp/1pPN4/1P2p3/1P4PP/5P2/5K2 w - - 0 1",
			ChessMove(f2, f3, pawn, no_piece), 
			ChessMove(d5, c3, knight, no_piece), 
			ChessMove(d5, e3, knight, no_piece), 
			ChessMove(d5, f4, knight, no_piece),
			MoveOrderPhase::hash, 
			{ 	
				{ MoveOrderPhase::hash, 1 },
				{ MoveOrderPhase::next_capture, 1 },
				{ MoveOrderPhase::killer1, 1 },
				{ MoveOrderPhase::killer2, 1 },
				{ MoveOrderPhase::countermove, 1 },
				{ MoveOrderPhase::next_quiet, 11 }
			}
		},
		{
			"8/5pk1/p4npp/1pPN4/1P2p3/1P4PP/5P2/5K2 w - - 0 1",
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0),
			MoveOrderPhase::hash, 
			{ 	
				{ MoveOrderPhase::next_capture, 1 },
				{ MoveOrderPhase::next_quiet, 15 }
			}
		},
		{
			"3q1r1k/p1pn1pp1/1p2p3/6b1/3PB3/5N2/PPP2PbP/2K4R w - - 0 1",
			ChessMove(f3, g5, knight, bishop), 
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0),
			MoveOrderPhase::hash_check,
			{ 	
				{ MoveOrderPhase::hash_check, 1 },
				{ MoveOrderPhase::next_check, 3 }
			}
		},
		{
			"N3k3/1p2bpp1/2p3p1/2P5/1p2p3/P3P1Pq/3B4/R2nR1K1 w - - 0 1",
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0),
			MoveOrderPhase::init_quiescence_capture_checks,
			{ 	
				{ MoveOrderPhase::next_quiescence_capture_checks, 4 },
				{ MoveOrderPhase::next_quiescence_check, 1 }
			}
		},
		{
			"2rr4/2Rnkp1p/p3p3/1p1pP2p/1n1P4/Q2B1P2/PP3P1P/1K4R1 b - - 0 1",
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0),
			MoveOrderPhase::init_quiescence_capture,
			{ 	
				{ MoveOrderPhase::next_quiescence_capture, 1 }
			}
		},
		{
			"3q1r1k/p1pn1pp1/1p2p3/6b1/3PB2p/5N2/PPP1QPP1/2KR3R w - - 1 1",
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0), 
			ChessMove(0),
			MoveOrderPhase::init_quiescence_evasion,
			{ 	
				{ MoveOrderPhase::next_quiescence_evasion, 6 }
			}
		}
			
	};
	
	ChessHistory &history = engine.getThreads().getMainThread()->history;
	
	for (const auto &p : positions) {

		const auto &fen = std::get<0>(p);
		const auto &move_hash = std::get<1>(p);
		const auto &move_killer1 = std::get<2>(p);
		const auto &move_killer2 = std::get<3>(p);
		const auto &move_counter = std::get<4>(p);
		const auto &phase_init = std::get<5>(p);
		const auto &phases = std::get<6>(p);
	
		bool rf = engine.setFENPosition(fen);
		EXPECT_EQ(rf, true);
		
		history.clear();
		history.clearKillers();
		
		std::vector<std::pair<ChessMove, MoveOrderPhase>> genmoves;
		ChessMoveSortList movelist;
		ChessMoveOrder moveorder(history, movelist, phase_init, move_hash, 
			move_killer1, move_killer2, move_counter);

		ChessMove move;
		
		while ((move = moveorder.getNextMove(board)) != ChessMove(0)) {
			
			const ChessMoveRec rec = board.makeMove(move);

			if (!board.isPositionLegal()) {
				board.unMakeMove(move, rec);
				continue;
			}

			genmoves.emplace_back(move, moveorder.getPhase());		
			
			board.unMakeMove(move, rec);
		}
				
		int j = 0;
		for (const auto & [phase, phase_count] : phases) {
			for (int n=0; n<phase_count; n++) {
				EXPECT_EQ(phase, genmoves[j].second);
				++j;
			}
		}

		EXPECT_EQ(genmoves.size(), j);		
	}
}

////////////////////////////////////////////////////////////////////////////////
// test eval symmetry
TEST(ChessEval, TestEval1) {
	ChessEngine engine;
	engine.init(1);	
	
	const std::vector<std::string> positions = {
		"r1bqkbnr/1ppp1ppp/p1n5/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 1 4",
		"8/6p1/P1b1pp2/2p1p3/1k4P1/3PP3/1PK5/5B2 w - - 0 1",
		"2rr3k/2qnbppp/p1n1p3/1p1pP3/3P1N2/1Q1BBP2/PP3P1P/1KR3R1 w - - 0 1",
		"rb3qk1/pQ3ppp/4p3/3P4/8/1P3N2/1P3PPP/3R2K1 w - - 0 1",
		"kr2R3/p4r2/2pq4/2N2p1p/3P2p1/Q5P1/5P1P/5BK1 w - - 0 1",
	};
	
	for (const auto &p : positions) {
		bool rf = engine.setFENPosition(p);
		EXPECT_EQ(rf, true);
		int v = engine.evaluate(min_score_mate, max_score_mate);		
		std::string rstr;
		bool r = ChessFENParser::reverse(p, rstr);
		EXPECT_EQ(r, true);
		rf = engine.setFENPosition(rstr);
		EXPECT_EQ(rf, true);
		int rv = engine.evaluate(min_score_mate, max_score_mate);	
		EXPECT_EQ(v, rv);
	}
}

////////////////////////////////////////////////////////////////////////////////
// test SEE
TEST(ChessBoard, TestSEE) {
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();	
	
	const std::vector<std::tuple<std::string, ChessMove, int>> positions = {
		{ "rb3qk1/pQ3ppp/4p3/3P4/8/1P3N2/1P3PPP/3R2K1 w - - 0 1 ", 
			ChessMove(b7, a8, queen, rook), 
			piece_value_see[rook] },
		{ "2b3k1/4rrpp/p2p4/2pP2RQ/1pP1Pp1N/1P3P1P/1q6/6RK w - - 0 1 ",
			ChessMove(h5, h7, queen, pawn), 
			piece_value_see[pawn] - piece_value_see[queen] },
		{ "r4rk1/pp2ppbp/1qp2np1/n2P4/1Q2P3/2N2B2/PP3PPP/R1B2RK1 w - - 0 1 ",
			ChessMove(b4, b6, queen, queen),
			0 }
	};
	
	for (auto p : positions) {
		bool rf = engine.setFENPosition(std::get<0>(p));
		EXPECT_EQ(rf, true);
		int r = board.staticExchangeEvaluation(std::get<1>(p));		
		EXPECT_EQ(r, std::get<2>(p));
	}
}

// setoption name UCI_Chess960 value true

// position fen qnnrkrbb/pppppppp/8/8/8/8/PPPPPPPP/QNNRKRBB w FDfd - 0 1 moves f2f4 f7f5 d2d4 d7d5 c1d3 c8d6 e1d1 e8d8 g2g3

// position fen rnkqrbbn/pppppppp/8/8/8/8/PPPPPPPP/RNKQRBBN w EAea - 0 1 moves f2f4 f7f5 h1g3 h8g6 g3f5 e7e6 f5g3 g6f4 d2d4 e6e5 e2e3 f4g6 d4d5 f8b4 c2c3 b4c5 c3c4 d7d6 b1c3 b8d7 f1d3 g6h4 d1d2 d8e7 g1f2 c8a8 c1e1 e8f8

////////////////////////////////////////////////////////////////////////////////
TEST(ChessBoard, TestCastling) {
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();	

	engine.getBoardRef().setChessRules(ChessRules::frc);

	bool rf = engine.setFENPosition("rqnkrbbn/pppppppp/8/8/8/8/PPPPPPPP/RQNKRBBN w KQkq - 0 1 ");
	EXPECT_EQ(rf, true);
	EXPECT_EQ(board.squareRookShort(white), e1);
	EXPECT_EQ(board.squareRookLong(white), a1);
	EXPECT_EQ(board.squareRookShort(black), e8);
	EXPECT_EQ(board.squareRookLong(black), a8);
	EXPECT_EQ(board.squareKingStart(white), d1);
	EXPECT_EQ(board.squareKingStart(black), d8);

	EXPECT_EQ(board.canCastle(castle_white_short), true);
	EXPECT_EQ(board.canCastle(castle_black_short), true);
	EXPECT_EQ(board.canCastle(castle_white_long), true);
	EXPECT_EQ(board.canCastle(castle_black_long), true);

	EXPECT_EQ(board.bitboardCastleShort(white), BitBoards::square[f1] | 
		BitBoards::square[g1]);
	EXPECT_EQ(board.bitboardCastleLong(white), BitBoards::square[b1] | 
		BitBoards::square[c1]);
	EXPECT_EQ(board.bitboardCastleShort(black), BitBoards::square[f8] | 
		BitBoards::square[g8]);
	EXPECT_EQ(board.bitboardCastleLong(black), BitBoards::square[b8] | 
		BitBoards::square[c8]);

	rf = engine.setFENPosition("bqnrnbkr/pppppppp/8/8/8/8/PPPPPPPP/BQNRNBKR w KQkq - 0 1 ");
	EXPECT_EQ(rf, true);
	EXPECT_EQ(board.squareRookShort(white), h1);
	EXPECT_EQ(board.squareRookLong(white), d1);
	EXPECT_EQ(board.squareRookShort(black), h8);
	EXPECT_EQ(board.squareRookLong(black), d8);
	EXPECT_EQ(board.squareKingStart(white), g1);
	EXPECT_EQ(board.squareKingStart(black), g8);

	EXPECT_EQ(board.canCastle(castle_white_short), true);
	EXPECT_EQ(board.canCastle(castle_black_short), true);
	EXPECT_EQ(board.canCastle(castle_white_long), true);
	EXPECT_EQ(board.canCastle(castle_black_long), true);

	EXPECT_EQ(board.bitboardCastleShort(white), BitBoards::square[f1]);
	EXPECT_EQ(board.bitboardCastleLong(white), BitBoards::square[c1] | 
		BitBoards::square[e1] | BitBoards::square[f1]);
	EXPECT_EQ(board.bitboardCastleShort(black), BitBoards::square[f8]);
	EXPECT_EQ(board.bitboardCastleLong(black), BitBoards::square[c8] | 
		BitBoards::square[e8] | BitBoards::square[f8]);

	rf = engine.setFENPosition("nrkrbbnq/pppppppp/8/8/8/8/PPPPPPPP/NRKRBBNQ w KQkq - 0 1 ");
	EXPECT_EQ(rf, true);
	EXPECT_EQ(board.squareRookShort(white), d1);
	EXPECT_EQ(board.squareRookLong(white), b1);
	EXPECT_EQ(board.squareRookShort(black), d8);
	EXPECT_EQ(board.squareRookLong(black), b8);
	EXPECT_EQ(board.squareKingStart(white), c1);
	EXPECT_EQ(board.squareKingStart(black), c8);

	EXPECT_EQ(board.canCastle(castle_white_short), true);
	EXPECT_EQ(board.canCastle(castle_black_short), true);
	EXPECT_EQ(board.canCastle(castle_white_long), true);
	EXPECT_EQ(board.canCastle(castle_black_long), true);

	EXPECT_EQ(board.bitboardCastleShort(white), BitBoards::square[e1] | 
		BitBoards::square[f1] | BitBoards::square[g1]);
	EXPECT_EQ(board.bitboardCastleLong(white), BitBoards::square[d1]);
	EXPECT_EQ(board.bitboardCastleShort(black), BitBoards::square[e8] | 
		BitBoards::square[f8] | BitBoards::square[g8]);
	EXPECT_EQ(board.bitboardCastleLong(black), BitBoards::square[d8]);

	rf = engine.setFENPosition("rnkqrbbn/pppppppp/8/8/8/8/PPPPPPPP/RNKQRBBN w EAea - 0 1 ");
	EXPECT_EQ(rf, true);
	EXPECT_EQ(board.squareRookShort(white), e1);
	EXPECT_EQ(board.squareRookLong(white), a1);
	EXPECT_EQ(board.squareRookShort(black), e8);
	EXPECT_EQ(board.squareRookLong(black), a8);
	EXPECT_EQ(board.squareKingStart(white), c1);
	EXPECT_EQ(board.squareKingStart(black), c8);

	EXPECT_EQ(board.canCastle(castle_white_short), true);
	EXPECT_EQ(board.canCastle(castle_black_short), true);
	EXPECT_EQ(board.canCastle(castle_white_long), true);
	EXPECT_EQ(board.canCastle(castle_black_long), true);

	EXPECT_EQ(board.bitboardCastleShort(white), BitBoards::square[d1] | 
		BitBoards::square[f1] | BitBoards::square[g1]);
	EXPECT_EQ(board.bitboardCastleLong(white), BitBoards::square[b1] | 
		BitBoards::square[d1]);
	EXPECT_EQ(board.bitboardCastleShort(black), BitBoards::square[d8] | 
		BitBoards::square[f8] | BitBoards::square[g8]);
	EXPECT_EQ(board.bitboardCastleLong(black), BitBoards::square[b8] | 
		BitBoards::square[d8]);
}

////////////////////////////////////////////////////////////////////////////////
// test bitbases
TEST(ChessBitBases, TestBitBases1) {
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();	
	
	const std::vector<std::pair<std::string, bool>> positions = {
		{ "4k3/8/8/8/P7/8/8/K7 w - - 0 1 ", false },
		{ "k7/8/8/p7/8/8/8/4K3 b - - 0 1 ", false },
		{ "4k3/8/2K5/8/8/8/3P4/8 w - - 0 1 ", true },
		{ "8/3p4/8/8/8/2k5/8/4K3 b - - 0 1 ", true },
		{ "3k4/8/5K2/8/8/8/4P3/8 w - - 0 1 ", true }
	};
	
	for (const auto &p : positions) {
		bool rf = engine.setFENPosition(p.first);
		EXPECT_EQ(rf, true);
			
		int v = ChessEndGames::evalKPK(board.side(), board);
		
		if (p.second) {
			EXPECT_NE(v, 0);
		} else {
			EXPECT_EQ(v, 0);
		}				
	}
}

////////////////////////////////////////////////////////////////////////////////
// test perft counts
std::uint64_t testPerft(const std::string &str, int d)
{
	ChessEngine engine;
	engine.init(1);	
	bool rf = engine.setFENPosition(str);
	if (!rf) {
		return 0ULL;
	}
	ChessTools tools(engine, engine.getBoardRef());
	tools.perft(d);
	return tools.countnode;
}

TEST(Perft, TestPerft1) {	
    EXPECT_EQ(4865609, testPerft(
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ", 5));
}

TEST(Perft, TestPerft2) {
	EXPECT_EQ(193690690, testPerft(
	"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", 5));
}

TEST(Perft, TestPerft3) {
	EXPECT_EQ(11030083, testPerft(
	"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ", 6));
}

TEST(Perft, TestPerft4) {
	EXPECT_EQ(15833292, testPerft(
	"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1 ", 5));
}

TEST(Perft, TestPerft5) {
	EXPECT_EQ(15833292, testPerft(
	"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1 ", 5));
}

TEST(Perft, TestPerft6) {
	EXPECT_EQ(89941194, testPerft(
	"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ", 5));	
}

////////////////////////////////////////////////////////////////////////////////
// test hash table entry format : type
TEST(Hash, TestHashType) {
	ChessEngine engine;
	ChessHashTable &ht = engine.getHashTable();
	engine.init(1);	
		
	for (int i=0; i<3; i++) {
        int depth = 0;
    	int score = 0;
    	unsigned int age = 0;
        if (i == 1) {
		    depth = 255;
		    score = 32767;
		    age = 63;
	    } 
	    if (i == 2) {
		    depth = 255;
		    score = -32768;
		    age = 63;
	    } 

		for (int n=0; n<4; n++) {

			auto k = (static_cast<HashKey>(i) * 4 + n) * nb_hash_cluster;

			ChessHashNode *hash = ht.getHashNode(k);

			ChessHashNode node(k, ChessMove(0), 
				static_cast<HashNodeType>(n), depth, score, age);
			hash->write(node);
        }
    }

	for (int i=0; i<3; i++) {
        int depth = 0;
    	int score = 0;
    	unsigned int age = 0;
        if (i == 1) {
		    depth = 255;
		    score = 32767;
		    age = 63;
	    } 
	    if (i == 2) {
		    depth = 255;
		    score = -32768;
		    age = 63;
	    } 

        for (int n=0; n<4; n++) {

            auto k = (static_cast<HashKey>(i) * 4 + n) * nb_hash_cluster;

			ChessHashNode *hash = ht.getHashNode(k);

			ChessHashNode node2;
			hash->read(node2);

			EXPECT_EQ(node2.getKey(), k);
			EXPECT_EQ(node2.getMove(), ChessMove(0));
			EXPECT_EQ(node2.getType(), static_cast<HashNodeType>(n));
			EXPECT_EQ(node2.getDepth(), depth);
			EXPECT_EQ(node2.getScore(), score);
			EXPECT_EQ(node2.getAge(), age);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// test hash table entry format : depth
TEST(Hash, TestHashDepth) {
	ChessEngine engine;
	ChessHashTable &ht = engine.getHashTable();
	engine.init(1);	

	for (int i=0; i<3; i++) {
		for (int n=0; n<256; n++) {

			auto k = (static_cast<HashKey>(i) * 256 + n) * nb_hash_cluster;

			ChessHashNode *hash = ht.getHashNode(k);
			HashNodeType type = HashNodeType::exact;
			
            int score = 0;
			unsigned int age = 0;
			if (i == 1) {
				type = HashNodeType::not_found;
				score = 32767;
				age = 63;
			}
			if (i == 2) {
				type = HashNodeType::not_found;
				score = -32768;
				age = 63;
			}

			ChessHashNode node(k, ChessMove(0), 
				type, n, score, age);
			hash->write(node);
		}
	}

	for (int i=0; i<3; i++) {
		for (int n=0; n<256; n++) {

            auto k = (static_cast<HashKey>(i) * 256 + n) * nb_hash_cluster;

			ChessHashNode *hash = ht.getHashNode(k);
			HashNodeType type = HashNodeType::exact;

			int score = 0;
			unsigned int age = 0;
			if (i == 1) {
				type = HashNodeType::not_found;
				score = 32767;
				age = 63;
			}
			if (i == 2) {
				type = HashNodeType::not_found;
				score = -32768;
				age = 63;
			}

			ChessHashNode node2;
			hash->read(node2);

			EXPECT_EQ(node2.getKey(), k);
			EXPECT_EQ(node2.getMove(), ChessMove(0));
			EXPECT_EQ(node2.getType(), type);
			EXPECT_EQ(node2.getDepth(), n);
			EXPECT_EQ(node2.getScore(), score);
			EXPECT_EQ(node2.getAge(), age);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// test hash table entry format : score
TEST(Hash, TestHashScore) {
	ChessEngine engine;
	ChessHashTable &ht = engine.getHashTable();
	engine.init(8);	
	
	for (int i=0; i<2; i++) {
		for (int n=-32768; n<=32767; n++) {

			auto k = (static_cast<HashKey>(i) * 65536 + n + 32768) * 
				nb_hash_cluster;

			ChessHashNode *hash = ht.getHashNode(k);
			HashNodeType type = HashNodeType::exact;
			int depth = 0; 
			unsigned int age = 0;
			if (i == 1) {
				type = HashNodeType::not_found;
				depth = 255;
				age = 63;
			}
			ChessHashNode node(k, ChessMove(0), 
				type, depth, n, age);
			hash->write(node);
		}
	}

	for (int i=0; i<2; i++) {
		for (int n=-32768; n<=32767; n++) {

			auto k = (static_cast<HashKey>(i) * 65536 + n + 32768) * 
				nb_hash_cluster;

			ChessHashNode *hash = ht.getHashNode(k);
			HashNodeType type = HashNodeType::exact;
			int depth = 0; 
			unsigned int age = 0;
			if (i == 1) {
				type = HashNodeType::not_found;
				depth = 255;
				age = 63;
			}

			ChessHashNode node2;
			hash->read(node2);
			EXPECT_EQ(node2.getKey(), k);
			EXPECT_EQ(node2.getMove(), ChessMove(0));
			EXPECT_EQ(node2.getType(), type);
			EXPECT_EQ(node2.getDepth(), depth);
			EXPECT_EQ(node2.getScore(), n);
			EXPECT_EQ(node2.getAge(), age);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// test hash table entry format : age
TEST(Hash, TestHashAge) {
	ChessEngine engine;
	ChessHashTable &ht = engine.getHashTable();
	engine.init(1);	
	
	for (int i=0; i<3; i++) {
		for (unsigned int n=0; n<64; n++) {

            auto k = (static_cast<HashKey>(i) * nb_squares + n) * 
				nb_hash_cluster;

			ChessHashNode *hash = ht.getHashNode(k);
			HashNodeType type = HashNodeType::exact;
			int depth = 0; 
			int score = 0;
			if (i == 1) {
				type = HashNodeType::not_found;
				depth = 255;
				score = 32767;
			}
			if (i == 2) {
				type = HashNodeType::not_found;
				depth = 255;
				score = -32768;
			}
			ChessHashNode node(k, ChessMove(0), 
				type, depth, score, n);
			hash->write(node);
		}
	}

	for (int i=0; i<3; i++) {
		for (unsigned int n=0; n<64; n++) {

            auto k = (static_cast<HashKey>(i) * nb_squares + n) * 
				nb_hash_cluster;

			ChessHashNode *hash = ht.getHashNode(k);
			HashNodeType type = HashNodeType::exact;
			int depth = 0; 
			int score = 0;
			if (i == 1) {
				type = HashNodeType::not_found;
				depth = 255;
				score = 32767;
			}
			if (i == 2) {
				type = HashNodeType::not_found;
				depth = 255;
				score = -32768;
			}

			ChessHashNode node2;
			hash->read(node2);
			EXPECT_EQ(node2.getKey(), k);
			EXPECT_EQ(node2.getMove(), ChessMove(0));
			EXPECT_EQ(node2.getType(), type);
			EXPECT_EQ(node2.getDepth(), depth);
			EXPECT_EQ(node2.getScore(), score);
			EXPECT_EQ(node2.getAge(), n);
		}
	}
}


TEST(MoveParser, TestMoveParser) {

	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();
	

	EXPECT_EQ(ChessMoveParser::is_char_file('a'), true);
	EXPECT_EQ(ChessMoveParser::is_char_file('h'), true);
	EXPECT_EQ(ChessMoveParser::is_char_file('i'), false);

	EXPECT_EQ(ChessMoveParser::is_char_rank('1'), true);
	EXPECT_EQ(ChessMoveParser::is_char_rank('8'), true);
	EXPECT_EQ(ChessMoveParser::is_char_rank('9'), false);

	EXPECT_EQ(ChessMoveParser::is_char_piece('p'), true);
	EXPECT_EQ(ChessMoveParser::is_char_piece('n'), true);
	EXPECT_EQ(ChessMoveParser::is_char_piece('z'), false);

	EXPECT_EQ(ChessMoveParser::is_char_side('w'), true);
	EXPECT_EQ(ChessMoveParser::is_char_side('b'), true);
	EXPECT_EQ(ChessMoveParser::is_char_side('r'), false);

	EXPECT_EQ(ChessMoveParser::file_from_char('a'), file_a);
	EXPECT_EQ(ChessMoveParser::file_from_char('h'), file_h);

	EXPECT_EQ(ChessMoveParser::rank_from_char('1'), rank_1);
	EXPECT_EQ(ChessMoveParser::rank_from_char('8'), rank_8);

	EXPECT_EQ(ChessMoveParser::piece_from_char('n'), knight);
	EXPECT_EQ(ChessMoveParser::piece_from_char('Q'), queen);

	EXPECT_EQ(ChessMoveParser::side_from_char('b'), black);
	EXPECT_EQ(ChessMoveParser::side_from_char('w'), white);

	EXPECT_EQ(ChessMoveParser::file_to_char(file_a), 'a');
	EXPECT_EQ(ChessMoveParser::file_to_char(file_h), 'h');

	EXPECT_EQ(ChessMoveParser::rank_to_char(rank_1), '1');
	EXPECT_EQ(ChessMoveParser::rank_to_char(rank_8), '8');

	EXPECT_EQ(ChessMoveParser::piece_to_char_upper(bishop), 'B');
	EXPECT_EQ(ChessMoveParser::piece_to_char_lower(rook), 'r');

	EXPECT_EQ(ChessMoveParser::side_to_char(black), 'b');
	EXPECT_EQ(ChessMoveParser::side_to_char(white), 'w');

	EXPECT_EQ(ChessMoveParser::piece_color_from_char('p'), black);
	EXPECT_EQ(ChessMoveParser::piece_color_from_char('P'), white);

	EXPECT_EQ(ChessMoveParser::move_to_string(
		ChessMove(e4, e5, pawn, no_piece, no_piece, 0, 0)), "e4e5");

	ChessMove move;
	bool r = ChessMoveParser::move_from_string("e2e4", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(e2, e4, pawn, no_piece, no_piece, 0, 0));

	r = ChessMoveParser::move_from_string("g1f3", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(g1, f3, knight, no_piece, no_piece, 0, 0));
}

////////////////////////////////////////////////////////////////////////////////
TEST(ChessBoard, TestFindPieceAttack) {
	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();

	bool rf = engine.setFENPosition("2bq1rk1/rpb2p2/2p1p1p1/p1N3Np/P2P1P1P/1Q2R1P1/1P3P2/3R2K1 w - - ");
	EXPECT_EQ(rf, true);

	EXPECT_EQ(board.findSamePieceAttack(white, knight, g5, e6), SameAttack::rank);
	EXPECT_EQ(board.findSamePieceAttack(white, knight, c5, b7), SameAttack::none);
	EXPECT_EQ(board.findSamePieceAttack(white, rook, e3, e1), SameAttack::other);
	EXPECT_EQ(board.findSamePieceAttack(white, knight, g5, e7), SameAttack::error);

	EXPECT_EQ(board.findPieceAttackOnFile(white, knight, e6, file_g), g5);
	EXPECT_EQ(board.findPieceAttackOnRank(white, queen, e6, rank_3), b3);
	EXPECT_EQ(board.findPieceAttack(black, bishop, f4), c7);

	EXPECT_EQ(board.findPieceAttackOnFile(white, pawn, f3, file_f), f2);
	EXPECT_EQ(board.findPieceAttackOnFile(black, pawn, b5, file_b), b7);

	rf = engine.setFENPosition("r1r3k1/p3bppp/2bp3Q/q2pP1P1/1p1BP3/8/PPP1B2P/2KR2R1 w - - ");
	EXPECT_EQ(rf, true);
	EXPECT_EQ(board.findPieceAttackOnFile(white, pawn, d6, file_e), e5);

}

////////////////////////////////////////////////////////////////////////////////
TEST(MoveParser, TestMoveParserSAN) {

	ChessEngine engine;
	engine.init(1);	
	ChessBoard &board = engine.getBoardRef();

	bool rf = engine.setFENPosition("2bq1rk1/rpb2p2/2p1p1p1/p1N3Np/P2P1P1P/1Q2R1P1/1P3P2/3R2K1 w - - ");
	EXPECT_EQ(rf, true);

	EXPECT_EQ(ChessMoveParser::move_to_string(ChessMove(g5, e6, knight, 
		pawn, no_piece, 0, 0)), "g5e6");
		
	EXPECT_EQ(ChessMoveParser::move_to_string_san(ChessMove(g5, e6, knight, 
		pawn, no_piece, 0, 0), board), "Ngxe6");
	EXPECT_EQ(ChessMoveParser::move_to_string_san(ChessMove(c5, b7, knight, 
		pawn, no_piece, 0, 0), board), "Nxb7");
	EXPECT_EQ(ChessMoveParser::move_to_string_san(ChessMove(f2, f3, pawn, 
		no_piece, no_piece, 0, 0), board), "f3");
	EXPECT_EQ(ChessMoveParser::move_to_string_san(ChessMove(b3, d5, queen, 
		no_piece, no_piece, 0, 0), board), "Qd5");
	EXPECT_EQ(ChessMoveParser::move_to_string_lan(ChessMove(e3, d3, rook, 
		no_piece, no_piece, 0, 0)), "Re3d3");

	ChessMove move;
	bool r = ChessMoveParser::move_from_string("g5e6", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(g5, e6, knight, pawn, no_piece, 0, 0));

	r = ChessMoveParser::move_from_string_san("Ngxe6", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(g5, e6, knight, pawn, no_piece, 0, 0));

	r = ChessMoveParser::move_from_string_san("Nxb7", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(c5, b7, knight, pawn, no_piece, 0, 0));

	r = ChessMoveParser::move_from_string_san("f3", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(f2, f3, pawn, no_piece, no_piece, 0, 0));

	r = ChessMoveParser::move_from_string_san("Qd5", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(b3, d5, queen, no_piece, no_piece, 0, 0));

	r = ChessMoveParser::move_from_string_san("Re3d3", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(e3, d3, rook, no_piece, no_piece, 0, 0));

	rf = engine.setFENPosition("r1r2k2/pp2bpp1/2bppn1p/6B1/2qNPPPP/2N5/PPPQ4/1K1RR3 w - - ");
	EXPECT_EQ(rf, true);

	EXPECT_EQ(ChessMoveParser::move_to_string_san(ChessMove(d4, b5, knight, 
		no_piece, no_piece, 0, 0), board), "Ndb5");

	r = ChessMoveParser::move_from_string_san("Ndb5", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(d4, b5, knight, no_piece, no_piece, 0, 0));

	rf = engine.setFENPosition("3r4/2q5/5pk1/p3n1p1/N3Pp1p/1PPr1P1P/2Q1R1P1/5R1K b - - ");
	EXPECT_EQ(rf, true);

	EXPECT_EQ(ChessMoveParser::move_to_string_san(ChessMove(d3, d7, rook, 
		no_piece, no_piece, 0, 0), board), "R3d7");

	r = ChessMoveParser::move_from_string_san("R3d7", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(d3, d7, rook, no_piece, no_piece, 0, 0));

	rf = engine.setFENPosition("3r2k1/6p1/B2R1p1p/1pPr1P2/3P4/8/1P3nP1/2KR4 b - - 1 1 ");
	EXPECT_EQ(rf, true);

	EXPECT_EQ(ChessMoveParser::move_to_string_san(ChessMove(d5, d6, rook, 
		rook, no_piece, 0, 0), board), "R5xd6");

	r = ChessMoveParser::move_from_string_san("R5xd6", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(d5, d6, rook, rook, no_piece, 0, 0));

	rf = engine.setFENPosition("r1r3k1/p3bppp/2bp3Q/q2pP1P1/1p1BP3/8/PPP1B2P/2KR2R1 w - - ");
	EXPECT_EQ(rf, true);

	EXPECT_EQ(ChessMoveParser::move_to_string_san(ChessMove(e5, d6, pawn, 
		pawn, no_piece, 0, 0), board), "exd6");

	r = ChessMoveParser::move_from_string_san("exd6", board, move);
	EXPECT_EQ(r, true);
	EXPECT_EQ(move, ChessMove(e5, d6, pawn, pawn, no_piece, 0, 0));	
}

////////////////////////////////////////////////////////////////////////////////
TEST(Tokenizer, TestTokenizer) {
	std::string str1 = "Bxa8 g4  Ne5 Bg5 f4 gxf3 * ";
	Tokenizer tk1(str1);
	EXPECT_EQ(tk1.hasMoreTokens(), true);
	EXPECT_EQ(tk1.nextToken(), "Bxa8");
	EXPECT_EQ(tk1.nextToken(), "g4");
	EXPECT_EQ(tk1.nextToken(), "Ne5");
	EXPECT_EQ(tk1.getCurrentString(), "Bg5 f4 gxf3 * ");
	EXPECT_EQ(tk1.nextToken(), "Bg5");
	EXPECT_EQ(tk1.nextToken(), "f4");
	EXPECT_EQ(tk1.nextToken(), "gxf3");
	EXPECT_EQ(tk1.nextToken(), "*");
	EXPECT_EQ(tk1.hasMoreTokens(), false);

	std::string str2 = "Bxa8";
	Tokenizer tk2(str2);
	EXPECT_EQ(tk2.hasMoreTokens(), true);
	EXPECT_EQ(tk2.nextToken(), "Bxa8");
	EXPECT_EQ(tk2.hasMoreTokens(), false);

	std::string str3 = "";
	Tokenizer tk3(str3);
	EXPECT_EQ(tk3.hasMoreTokens(), false);

	std::string str4 = " ";
	Tokenizer tk4(str4);
	EXPECT_EQ(tk4.hasMoreTokens(), false);

	std::string_view str5 = "  Bxa8  g4   ";
	Tokenizer tk5(str5);
	EXPECT_EQ(tk5.hasMoreTokens(), true);
	EXPECT_EQ(tk5.nextToken(), "Bxa8");
	EXPECT_EQ(tk5.nextToken(), "g4");
	EXPECT_EQ(tk5.hasMoreTokens(), false);
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);

  	ChessEngine::precalcTables();

  	return RUN_ALL_TESTS();
}

