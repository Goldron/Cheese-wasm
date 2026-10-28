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

#ifndef CHEESE_PV_H_
#define CHEESE_PV_H_

#include "config.h"
#include "move.h"

#include <array>
#include <vector>

namespace Cheese {

// maximum number of multipv
inline constexpr int max_multipv = 256;

// maximum depth used for PV
inline constexpr int max_depth_pv = 128;

using ChessPVMoveList = std::array<ChessMove, max_depth_pv>;

////////////////////////////////////////////////////////////////////////////////
// Principal variation (PV)
class ChessPV {

	// size of pv move list for each depth
	std::array<std::uint32_t, max_depth_pv> pvsize;

	// move list for each depth
	std::array<ChessPVMoveList, max_depth_pv> pvmove;

public:

	ChessPV();

	[[nodiscard]]
	int size(int depth) const;

	[[nodiscard]]
	bool isEmpty(int depth) const;

	void clear(int depth);

	ChessPVMoveList &getPV(int depth);

	[[nodiscard]]
	ChessMove getMove(int depth, int n) const;

	void set(int depth, int n, ChessMove cm);

	void insertPV(int depth, ChessMove cm, ChessPV &pv);

	void print(int depth);
};

////////////////////////////////////////////////////////////////////////////////
// multi pv infos
class ChessMultiPVEntry {

public:

	// current score
	int score {0};

	// previous score
	int scorePrev {0};

	// depth
	int depth {0};

	// number of moves
	int nbmove {0};

	// moves
	ChessPVMoveList move;


	ChessMultiPVEntry() = default;

	void clear();

	[[nodiscard]]
	int size() const;

	[[nodiscard]]
	ChessMove getMove(int n) const;

	void copypv(const ChessPVMoveList &mlist, int count);
};

////////////////////////////////////////////////////////////////////////////////
class ChessMultiPV {

	// multipv info for each moves
	std::vector<ChessMultiPVEntry> info;

public:

	ChessMultiPV() = default;

	void clear();

	void init(int n);

	[[nodiscard]]
	int getCountMoves() const;

	[[nodiscard]]
	const ChessMultiPVEntry &getInfo(int n) const;

	void updateScores();

	void setScorePrev(int n, int score);

	void copyPV(int n, const ChessPVMoveList &pv, int sz, int depth);

	void sortMultiPVLines();
};

////////////////////////////////////////////////////////////////////////////////
inline int ChessPV::size(int depth) const
{
	return static_cast<int>(pvsize[depth]);
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessPV::isEmpty(int depth) const
{
	return (pvsize[depth] == 0);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessPV::clear(int depth)
{
	pvsize[depth] = 0;
	pvmove[depth][0].clear();
}

////////////////////////////////////////////////////////////////////////////////
inline ChessPVMoveList &ChessPV::getPV(int depth)
{
	return pvmove[depth];
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove ChessPV::getMove(int depth, int n) const
{
	return pvmove[depth][n];
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessPV::set(int depth, int n, ChessMove cm)
{
	pvmove[depth][n] = cm;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessMultiPVEntry::size() const
{
	return nbmove;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove ChessMultiPVEntry::getMove(int n) const
{
	return move[n];
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessMultiPV::getCountMoves() const
{
	return static_cast<int>(info.size());
}

////////////////////////////////////////////////////////////////////////////////
inline const ChessMultiPVEntry &ChessMultiPV::getInfo(int n) const
{
	return info[n];
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessMultiPV::setScorePrev(int n, int score)
{
	info[n].scorePrev = score;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessMultiPV::copyPV(int n, const ChessPVMoveList &pv, int sz, 
	int depth)
{
	info[n].copypv(pv, sz);
	info[n].depth = depth;
}

} // namespace Cheese

#endif //CHEESE_PV_H_
