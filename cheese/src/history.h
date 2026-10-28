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

#ifndef CHEESE_HISTORY_H_
#define CHEESE_HISTORY_H_

#include "config.h"
#include "move.h"

#include <array>

namespace Cheese {

// an entry in history heuristic table
using HistoryEntry = std::uint16_t;

// maximum value in history heuristic table
inline constexpr HistoryEntry max_history_value = 64000;

// maximum quiet moves used when reducing history
inline constexpr int max_history_quiets = 64;

// maximum depth the engine can search
inline constexpr int max_killer_depth = 128;

////////////////////////////////////////////////////////////////////////////////
// killer moves
struct ChessMoveKiller {

	ChessMove killer1;

	ChessMove killer2;
};

////////////////////////////////////////////////////////////////////////////////
// history heuristic, killer moves, and counter moves
class ChessHistory {

	// history heuristic [side][piece][dst]
	std::array<std::array<std::array<HistoryEntry, nb_squares>, nb_pieces_type>,
		nb_sides> history;

	// counter moves [side][src][dst]
	std::array<std::array<std::array<ChessMove, nb_squares>, nb_squares>,
		nb_sides> counterMoves;

	// killer moves [ply]
	std::array<ChessMoveKiller, max_killer_depth> killers;

public:

	ChessHistory();

	void clear();

	void setHistoryMove(Side side, ChessMove lastmove, ChessMove move);

	[[nodiscard]]
	ChessMove getHistoryMove(Side side, ChessMove lastmove) const;

	// add a move to history table
	void add(ChessMove move, Side side,  int depth);

	// * test : sub history for some move ?...
	void sub(ChessMove move, Side side,  int depth);

	// get history heuristic value
	[[nodiscard]]
	HistoryEntry get(ChessMove move, Side side) const;

	// clear all killers
	void clearKillers();

	// add a move to killer table
	void addKillerMove(ChessMove move, int ply);

	[[nodiscard]]
	ChessMove getKiller1(int ply) const;

	[[nodiscard]]
	ChessMove getKiller2(int ply) const;

	void copyKillers(const ChessHistory &hist, int ply, int count);
};

////////////////////////////////////////////////////////////////////////////////
inline void ChessHistory::setHistoryMove(Side side, ChessMove lastmove,
	ChessMove move)
{
	Square src = lastmove.src();
	Square dst = lastmove.dst();

	counterMoves[side][src][dst] = move;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove ChessHistory::getHistoryMove(Side side,
	ChessMove lastmove) const
{
	Square src = lastmove.src();
	Square dst = lastmove.dst();

	return counterMoves[side][src][dst];
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHistory::add(ChessMove move, Side side,  int depth)
{
	Piece pc = move.piece();
	Square dst = move.dst();

	auto v = static_cast<HistoryEntry>(depth * depth);
	if ((history[side][pc][dst] + v) < max_history_value) {
		history[side][pc][dst] += v;
	}
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHistory::sub(ChessMove move, Side side,  int depth)
{
	Piece pc = move.piece();
	Square dst = move.dst();

	auto v = static_cast<HistoryEntry>(depth * depth);
	if ((history[side][pc][dst] - v) >= 0) {
		history[side][pc][dst] -= v;
	}
}

////////////////////////////////////////////////////////////////////////////////
inline HistoryEntry ChessHistory::get(ChessMove move, Side side) const
{
	Piece pc = move.piece();
	Square dst = move.dst();

	return history[side][pc][dst];
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHistory::addKillerMove(ChessMove move, int ply)
{
	if (killers[ply].killer1 != move) {
		killers[ply].killer2 = killers[ply].killer1;
		killers[ply].killer1 = move;
	}
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove ChessHistory::getKiller1(int ply) const
{
	return killers[ply].killer1;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove ChessHistory::getKiller2(int ply) const
{
	return killers[ply].killer2;
}

} // namespace Cheese

#endif //CHEESE_HISTORY_H_
