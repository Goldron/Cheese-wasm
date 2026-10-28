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

#ifndef CHEESE_EVAL_H_
#define CHEESE_EVAL_H_

#include "config.h"

#include "bitboard.h"
#include "evalscore.h"
#include "move.h"
#include "tune.h"

#include <array>
#include <string>
#include <vector>

namespace Cheese {

inline constexpr bool use_lazy_eval = true;

// infinity score value
inline constexpr int eval_infinity = 32767;

// mate value returned by search (+/-)
inline constexpr int mate_value = 32000;

// default draw value returned by search
inline constexpr int draw_value = 0;

// minimum mate score : mated in N ply
inline constexpr int min_score_mate = -mate_value + 256;

// maximum mate score : mate in N ply
inline constexpr int max_score_mate = mate_value - 256;

// tempo bonus for evaluation
inline constexpr int eval_tempo = 10;

inline constexpr int nb_squares_half = nb_squares / 2;

inline constexpr int nb_files_half = nb_files / 2;

inline constexpr int nb_storm_type = 2;

inline constexpr int max_mobility_knight = 9;
inline constexpr int max_mobility_bishop = 14;
inline constexpr int max_mobility_rook   = 15;
inline constexpr int max_mobility_queen  = 28;

////////////////////////////////////////////////////////////////////////////////
// different version of evaluation function for personalities and tuning
enum class ChessEvalMode {
	normal,
	personality,
	tuning
};

class ChessBoard;
class ChessPawnHashTable;

////////////////////////////////////////////////////////////////////////////////
// chess evaluation personality parameters
struct ChessEvalPersonality {

	std::int32_t materialPawn {0};

	std::int32_t materialKnight {0};

	std::int32_t materialBishop {0};

	std::int32_t materialRook {0};

	std::int32_t materialQueen {0};

	std::int32_t materialPawnEnd {0};

	std::int32_t materialKnightEnd {0};

	std::int32_t materialBishopEnd {0};

	std::int32_t materialRookEnd {0};

	std::int32_t materialQueenEnd {0};

	std::int32_t bishopPairMid {0};

	std::int32_t bishopPairEnd {0};

	std::int32_t pawnDoubledMid {0};

	std::int32_t pawnDoubledEnd {0};

	std::int32_t pawnIsolatedMid {0};

	std::int32_t pawnIsolatedEnd {0};

	std::int32_t pawnBackwardMid {0};

	std::int32_t pawnBackwardEnd {0};

	std::int32_t rookOpenFileMid {0};

	std::int32_t rookOpenFileEnd {0};

	std::int32_t rookSemiOpenFileMid {0};

	std::int32_t rookSemiOpenFileEnd {0};

	std::int32_t rook7thRankMid {0};

	std::int32_t rook7thRankEnd {0};

	std::int32_t outpostKnight {0};

	std::int32_t outpostBishop {0};

	std::int32_t passedPawns {0};

	std::int32_t pawnShield {0};

	std::int32_t kingSafetyOwn {0};

	std::int32_t kingSafetyOpp {0};

	std::int32_t mobilityOwn {0};

	std::int32_t mobilityOpp {0};

	std::int32_t randomEval {0};

};

////////////////////////////////////////////////////////////////////////////////
// parameters passed to evaluation functions
struct ChessEvalParam {

	std::array<std::array<BitBoard, nb_pieces_type>, nb_sides> bbAttackByPiece;

	std::array<BitBoard, nb_sides> bbPassed;

	std::array<EvalScore, nb_sides> scoremob;

	std::array<EvalScore, nb_sides> kingSafety;

};

// eval terms for all evaluation parameters
struct ChessEvalTerms {

	// pieces square tables [piece][square]
	std::array<std::array<EvalTerm, nb_squares_half>, nb_pieces_type> psqt;

	// knight outpost
	std::array<EvalTerm, nb_squares_half> outpost_knight;

	// bishop outpost
	std::array<EvalTerm, nb_squares_half> outpost_bishop;

	// knight mobility
	std::array<EvalTerm, max_mobility_knight> mobility_knight;

	// bishop mobility
	std::array<EvalTerm, max_mobility_bishop> mobility_bishop;

	// rook mobility
	std::array<EvalTerm, max_mobility_rook> mobility_rook;

	// queen mobility
	std::array<EvalTerm, max_mobility_queen> mobility_queen;

	// passed pawns base score
	std::array<EvalTerm, nb_ranks> passed_base;

	// passed pawns free to advance bonus
	std::array<EvalTerm, nb_ranks> passed_free;

	std::array<EvalTerm, nb_ranks> passed_free_attacked;

	// passed pawns file bonus
	std::array<EvalTerm, nb_files_half> passed_file;

	// own king distance to passed pawn bonus
	std::array<EvalTerm, nb_ranks> passed_king_own;

	// opp king distance to passed pawn bonus
	std::array<EvalTerm, nb_ranks> passed_king_opp;

	// eval for missing or moved pawn shield
	std::array<std::array<EvalTerm, nb_ranks>, nb_files_half> pawn_shield;

	// pawn storm bonus
	std::array<std::array<std::array<EvalTerm, nb_ranks>, nb_files_half>, 
		nb_storm_type> pawn_storm;

	// material score for each pieces
	std::array<EvalTerm, nb_pieces_type> pieces_score;

	// bishop pair (bonus = half a pawn)
	EvalTerm bishop_pair;

	// malus for doubled pawns
	EvalTerm doubled_pawn;

	// isolated pawn
	EvalTerm isolated_pawn;

	// backward pawn
	EvalTerm backward_pawn;

	// open file
	EvalTerm rook_open_file;

	// semi open file
	EvalTerm rook_semi_open_file;

	// rook on 7th file
	EvalTerm rook_on_7th;

	// blocked pawns
	EvalTerm blocked_pawn;

	// blocked bishops
	EvalTerm trapped_bishop;

	// blocked knights
	EvalTerm trapped_knight;

	// blocked rooks
	EvalTerm blocked_rook;
};

////////////////////////////////////////////////////////////////////////////////
// Evaluation class
// need tables initialised in BitBoards::init()
class ChessEval {

private:

	// pieces square tables [side][piece][square]
	std::array<std::array<std::array<EvalScore, nb_squares>, nb_pieces_type>, 
		nb_sides> psqt;

	// knight outpost
	std::array<std::array<EvalScore, nb_squares>, nb_sides> outpost_knight;

	// bishop outpost
	std::array<std::array<EvalScore, nb_squares>, nb_sides> outpost_bishop;

	// knight mobility
	std::array<EvalScore, max_mobility_knight> mobility_knight;

	// bishop mobility
	std::array<EvalScore, max_mobility_bishop> mobility_bishop;

	// rook mobility
	std::array<EvalScore, max_mobility_rook> mobility_rook;

	// queen mobility
	std::array<EvalScore, max_mobility_queen> mobility_queen;

	// passed pawns base score
	std::array<EvalScore, nb_ranks> passed_base;

	// passed pawns free to advance bonus
	std::array<EvalScore, nb_ranks> passed_free;

	std::array<EvalScore, nb_ranks> passed_free_attacked;

	// passed pawns file bonus
	std::array<EvalScore, nb_files_half> passed_file;

	// own king distance to passed pawn bonus
	std::array<EvalScore, nb_ranks> passed_king_own;

	// opp king distance to passed pawn bonus
	std::array<EvalScore, nb_ranks> passed_king_opp;

	// eval for missing or moved pawn shield
	std::array<std::array<EvalScore, nb_ranks>, nb_files_half> pawn_shield;

	// pawn storm bonus
	std::array<std::array<std::array<EvalScore, nb_ranks>, nb_files_half>, 
		nb_storm_type> pawn_storm;

	// material score for each pieces
	std::array<EvalScore, nb_pieces_type> pieces_score;

	// bishop pair (bonus = half a pawn)
	EvalScore bishop_pair;

	// malus for doubled pawns
	EvalScore doubled_pawn;

	// isolated pawn
	EvalScore isolated_pawn;

	// backward pawn
	EvalScore backward_pawn;

	// open file
	EvalScore rook_open_file;

	// semi open file
	EvalScore rook_semi_open_file;

	// rook on 7th file
	EvalScore rook_on_7th;

	// blocked pawns
	EvalScore blocked_pawn;

	// blocked bishops
	EvalScore trapped_bishop;

	// blocked knights
	EvalScore trapped_knight;

	// blocked rooks
	EvalScore blocked_rook;

	// king safety : attackers weight
	std::array<std::int32_t, nb_pieces_type> king_attackers_weight;

	// engine playing side (used for personality)
	Side engineSide {white};

	// Contempt factor returned at draw score
	int drawValue {draw_value};

	// lazy evaluation margin
	int lazyEvalMargin {325};

	// total material value at the begining
	std::int32_t startMaterialValue {0};

	bool usePerso {false};

	// personality parameters
	ChessEvalPersonality perso;

	// tuning evaluation parameters
	std::vector<Tunable> parameters;

	// terms use count for tuning
	ChessEvalTerms terms;

public:

	ChessEval() = default;

	// evaluate position
	[[nodiscard]]
	int evaluate(const ChessBoard &board, ChessPawnHashTable &htp, int alpha,
		int beta);

	[[nodiscard]]
	int evaluateTuning(const ChessBoard &board, ChessPawnHashTable &htp);

	void setEngineSide(Side s);

	void clearEvalData();

	// set default parameters
	void initEvalData();

	// initialize evaluation
	void initEval();

	void initPersonality();

	[[nodiscard]]
	int getDrawValue() const;

	void setDrawValue(int v);

	[[nodiscard]]
	bool loadPersonality(const std::string &fname);

	void initTuning();

	[[nodiscard]]
	const std::vector<Tunable> &getTuningParameters() const;

	void addTuningPSQT(const std::string &name, Piece piece);

	// print evaluation parameters in C++ code, to use it in evalparams.h
	[[nodiscard]]
	std::string printEval() const;

	[[nodiscard]]
	bool saveEval(const std::string &filename) const;

	[[nodiscard]]
	bool loadEval(const std::string &filename);

private:

	template<ChessEvalMode evalmode>
	[[nodiscard]]
	int evaluatePosition(const ChessBoard &board, ChessPawnHashTable &htp, 
		int alpha, int beta);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluateKnights(const ChessBoard &board, ChessEvalParam &ev);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluateBishops(const ChessBoard &board, ChessEvalParam &ev);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluateRooks(const ChessBoard &board, ChessEvalParam &ev);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluateQueens(const ChessBoard &board, ChessEvalParam &ev);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluateKing(const ChessBoard &board, ChessEvalParam &ev);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluatePawnShield(const ChessBoard &board);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluatePawns(const ChessBoard &board, ChessEvalParam &ev);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluatePassedPawns(const ChessBoard &board, ChessEvalParam &ev);

	template<ChessEvalMode evalmode, Side side>
	[[nodiscard]]
	EvalScore evaluateKingFile(const ChessBoard &board, BitBoard bm, File f);
};

////////////////////////////////////////////////////////////////////////////////
inline const std::vector<Tunable> &ChessEval::getTuningParameters() const
{
	return parameters;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEval::setEngineSide(Side s)
{
	engineSide = s;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessEval::getDrawValue() const
{
	return drawValue;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessEval::setDrawValue(int v)
{
	drawValue = v;
}

// pieces score for middle game / end game
// used by lazy evaluation, endgame and king safety
extern std::array<std::int32_t, nb_pieces_type> pieces_value;
extern std::array<std::int32_t, nb_pieces_type> pieces_value_end;
extern std::int32_t minor_piece_max_value;

} // namespace Cheese

#endif //CHEESE_EVAL_H_

