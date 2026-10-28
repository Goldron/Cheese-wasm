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

#include "history.h"

#include <algorithm>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
ChessHistory::ChessHistory()
{
	clear();
	clearKillers();
}

////////////////////////////////////////////////////////////////////////////////
void ChessHistory::clear()
{
	for (int s=0; s<nb_sides; s++) {
		for (int p=0; p<nb_pieces_type; p++) {
			for (int sq=0; sq<nb_squares; sq++) {
				history[s][p][sq] = 0;
			}
		}
	}

	for (int s=0; s<nb_sides; s++) {
		for (int src=0; src<nb_squares; src++) {
			for (int dst=0; dst<nb_squares; dst++) {
				counterMoves[s][src][dst] = ChessMove(0);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessHistory::clearKillers()
{
	for (int d=0; d<max_killer_depth; d++) {
		killers[d].killer1 = ChessMove(0);
		killers[d].killer2 = ChessMove(0);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessHistory::copyKillers(const ChessHistory &hist, int ply, int count)
{
	std::copy_n(std::next(hist.killers.begin(), ply), count,
				std::next(killers.begin(), ply));
}

} // namespace Cheese
