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

#include "pv.h"
#include "moveparser.h"
#include "util/logfile.h"

#include <algorithm>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
ChessPV::ChessPV()
{
	pvsize = {};
}

////////////////////////////////////////////////////////////////////////////////
void ChessPV::insertPV(int depth, ChessMove cm, ChessPV &pv)
{
	pvmove[depth][0] = cm;
	int sz = std::max(static_cast<int>(pv.pvsize[depth + 1]), max_depth_pv - 1);
	std::copy_n(pv.pvmove[depth + 1].begin(), sz, 
		std::next(pvmove[depth].begin()));
	pvsize[depth] = std::min(static_cast<int>(pv.pvsize[depth + 1]) + 1,
		max_depth_pv - 1);
}

////////////////////////////////////////////////////////////////////////////////
void ChessPV::print(int depth)
{
	std::string str;
	for (auto n=0U; n<pvsize[depth]; n++) {
		str += ChessMoveParser::move_to_string_lan(pvmove[depth][n]) + ' ';
	}
	Logger::debug() << "(" << pvsize[depth] << ") : " << str;
}

////////////////////////////////////////////////////////////////////////////////
void ChessMultiPVEntry::copypv(const ChessPVMoveList &mlist, int count)
{
	std::copy_n(mlist.begin(), count, move.begin());
	nbmove = count;
}

////////////////////////////////////////////////////////////////////////////////
void ChessMultiPVEntry::clear()
{
	score = 0;
	scorePrev = 0;
	depth = 0;
	nbmove = 0;
	move[0].clear();
}

////////////////////////////////////////////////////////////////////////////////
void ChessMultiPV::init(int n)
{
	info.resize(n);
	clear();
}

////////////////////////////////////////////////////////////////////////////////
void ChessMultiPV::clear()
{
	for (auto &pv : info) {
		pv.clear();
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessMultiPV::updateScores()
{
	for (auto &pv : info) {
		pv.score = pv.scorePrev;
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessMultiPV::sortMultiPVLines()
{
	auto cmp = [](const ChessMultiPVEntry &a, const ChessMultiPVEntry &b) {
		return ((a.depth > b.depth) ||
			    ((a.depth == b.depth) && (a.scorePrev > b.scorePrev))); };
		
	std::stable_sort(info.begin(), info.end(), cmp);
}

} // namespace Cheese
