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

#ifndef CHEESE_MOVEORDER_H_
#define CHEESE_MOVEORDER_H_

#include "config.h"

#include "board.h"
#include "history.h"
#include "move.h"
#include "movegen.h"

#include "util/logfile.h"

#include <array>

namespace Cheese {

// sort root moves by quiescence search result
inline constexpr bool sort_root_quiescence = false;

////////////////////////////////////////////////////////////////////////////////
// move ordering phases
enum class MoveOrderPhase {

	// end of move ordering
	end,

	// normal search
	hash,

	init_capture,
	next_capture,

	killer1,
	killer2,
	countermove,

	init_quiet,
	next_quiet,
	next_bad_capture,

	// normal search in check
	hash_check,
	init_check,
	next_check,

	// quiescence search captures only
	init_quiescence_capture,
	next_quiescence_capture,

	init_quiescence_capture_checks,
	next_quiescence_capture_checks,

	// quiescence search moves that check
	init_quiescence_check,
	next_quiescence_check,

	// quiescence search check evasion
	init_quiescence_evasion,
	next_quiescence_evasion

};

inline auto operator++(MoveOrderPhase &p)
{
	return p = static_cast<MoveOrderPhase>(static_cast<unsigned int>(p) + 1);
}

inline auto operator++(MoveOrderPhase &p, int)
{
	auto mp = p;
	p = static_cast<MoveOrderPhase>(static_cast<unsigned int>(p) + 1);
	return mp;
}

class ChessSearch;

////////////////////////////////////////////////////////////////////////////////
// moves generation and ordering
// move list is a reference because we don't want to initialize it in the 
// search function
class ChessMoveOrder {

	ChessHistory &history;
	
	// move list reference
	ChessMoveSortList &movelist;

	// current phase
	MoveOrderPhase phase { MoveOrderPhase::end };

	// next phase
	MoveOrderPhase phaseNext;

	// current position in move list
	int position {0};

	// current position in capture list
	int pos_capture {0};

	// number of moves in current phase
	int count;

	// hash move
	ChessMove moveHash;

	// first killer move
	ChessMove moveKiller1;

	// second killer move
	ChessMove moveKiller2;

	ChessMove moveCounter;

public:

	ChessMoveOrder(ChessHistory &h, ChessMoveSortList &moves, MoveOrderPhase ph,
		ChessMove mh = ChessMove(0), ChessMove mk1 = ChessMove(0), 
		ChessMove mk2 = ChessMove(0), ChessMove mc = ChessMove(0));

	ChessMoveOrder & operator=(const ChessMoveOrder &mo) = delete;

	// get current phase
	[[nodiscard]]
	MoveOrderPhase getPhase() const;

	// update move generation and return current move, or empty move at end
	[[nodiscard]]
	ChessMove getNextMove(const ChessBoard &board);

private:

	template<MoveOrderPhase phase>
	void orderMoves(const ChessBoard &board, ChessMoveSortList &moves, 
		int pos, int nbmoves);
};

////////////////////////////////////////////////////////////////////////////////
inline MoveOrderPhase ChessMoveOrder::getPhase() const
{
	return phase;
}

} // namespace Cheese

#endif //CHEESE_MOVEORDER_H_
