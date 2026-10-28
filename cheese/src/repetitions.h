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

#ifndef CHEESE_REPETITIONS_H_
#define CHEESE_REPETITIONS_H_

#include "config.h"

#include "zobrist.h"

#include <array>

namespace Cheese {

// maximum number of ply in move repetition history
// maximum 100 for the game + 128 for the search
// (in the current game repetitions are cleared at non reversible moves)
inline constexpr int max_ply_history = 256;

////////////////////////////////////////////////////////////////////////////////
// 3-Fold repetitions
class ChessRepetitions {

	// number of keys in the repetition table
	int nbRep {0};

	// repetition table
	std::array<HashKey, max_ply_history> repetitions;

public:

	ChessRepetitions() = default;

	ChessRepetitions(const ChessRepetitions &r) = default;

	ChessRepetitions & operator= (const ChessRepetitions &r) = default;

	// add a key to repetition table
	void push(HashKey h);

	// remove last repetition table entry
	void pop();

	// clear repetition table
	void clear();

	// check for 3 fold repetition
	// (note: return true if the position is found twice)
	[[nodiscard]]
	bool check(HashKey h, int fifty) const;

	// get the number of repetitions
	[[nodiscard]]
	int getCount(HashKey h, int fifty) const;

	void print();
};

////////////////////////////////////////////////////////////////////////////////
inline void ChessRepetitions::push(HashKey h)
{
	repetitions[nbRep++] = h;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessRepetitions::pop()
{
	--nbRep;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessRepetitions::clear()
{
	nbRep = 0;
}

} // namespace Cheese

#endif // CHEESE_REPETITIONS_H_
