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

#ifndef CHEESE_MOVELIST_H_
#define CHEESE_MOVELIST_H_

#include "config.h"

#include "move.h"

#include <array>
#include <algorithm>
#include <utility>

namespace Cheese {

// max number of moves each turn (maximum = 218)
inline constexpr int max_play_moves = 256;

////////////////////////////////////////////////////////////////////////////////
class ChessMoveSortList {

	std::array<ChessMoveSort, max_play_moves> moves;

	ChessMoveSort *pos;

public:

	ChessMoveSortList() : pos(moves.data())
	{}

	ChessMoveSortList & operator=(const ChessMoveSortList &mo);

	[[nodiscard]] 
	const auto *begin() const
	{
		return moves.data();
	}

	[[nodiscard]] 
	const auto *end() const
	{
		return pos;
	}

	[[nodiscard]] 
	auto *begin()
	{
		return moves.data();
	}

	[[nodiscard]] 
	auto *end()
	{
		return pos;
	}

	[[nodiscard]] 
	const auto & front() const
	{
		return *pos;
	}

	[[nodiscard]] 
	int size() const
	{
		return static_cast<int>(pos - moves.data());
	}

	[[nodiscard]] 
	bool empty() const
	{
		return (pos == moves.data());
	}

	void clear()
	{
		pos = moves.data();
	}

	[[nodiscard]] 
	auto & operator[](int index)
	{
		return moves[index];
	}

	[[nodiscard]] 
	auto & operator[](int index) const
	{
		return moves[index];
	}

	void push(const ChessMoveSort &move)
	{
		*(pos++) = move;
	}

	// note : value is uninitialized but set later in move ordering
	void push(const ChessMove &move)
	{
		(pos++)->move = move;
	}

	template<class... Args>
	void emplace_back(Args&&... args)
	{
		push(ChessMove(std::forward<Args>(args)...));
	}

	void sort(int p, int cnt);

	void moveFront(const ChessMove &pvmove);

	void moveBestToFront(int p);

	void remove(const ChessMove &move);
};

////////////////////////////////////////////////////////////////////////////////
class ChessMoveList {

	std::array<ChessMove, max_play_moves> moves;

	ChessMove *pos;

public:

	ChessMoveList() : pos(moves.data())
	{}

	ChessMoveList & operator=(const ChessMoveList &mo);

	[[nodiscard]] 
	const auto *begin() const
	{
		return moves.data();
	}

	[[nodiscard]] 
	const auto *end() const
	{
		return pos;
	}

	[[nodiscard]] 
	auto *begin()
	{
		return moves.data();
	}

	[[nodiscard]] 
	auto *end()
	{
		return pos;
	}

	[[nodiscard]] 
	const auto & front() const
	{
		return *pos;
	}

	[[nodiscard]] 
	int size() const
	{
		return static_cast<int>(pos - moves.data());
	}

	[[nodiscard]] 
	bool empty() const
	{
		return (pos == moves.data());
	}

	void clear()
	{
		pos = moves.data();
	}

	[[nodiscard]] 
	auto & operator[](int index)
	{
		return moves[index];
	}

	[[nodiscard]] 
	auto & operator[](int index) const
	{
		return moves[index];
	}

	void push(const ChessMove &move)
	{
		*(pos++) = move;
	}

	template<class... Args>
	void emplace_back(Args&&... args)
	{
		push(ChessMove(std::forward<Args>(args)...));
	}
};

} // namespace Cheese

#endif // CHEESE_MOVELIST_H_

