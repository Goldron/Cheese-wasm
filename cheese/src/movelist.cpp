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

#include "movelist.h"

#include <algorithm>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
void ChessMoveSortList::moveFront(const ChessMove &pvmove)
{
	auto *it = std::find_if(begin(), end(), 
		[pvmove](const ChessMoveSort &m) { return (m.move == pvmove); });
	if (it != end()) {
		std::rotate(begin(), it, std::next(it));
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessMoveSortList::moveBestToFront(int p)
{
	auto *ib = std::next(begin(), p);
	auto *it = std::max_element(ib, end(), 
		[](const ChessMoveSort &a, const ChessMoveSort &b) {
		return a.value < b.value;
	});
	if (it != end()) {
		std::swap(*it, *ib);
	}
}

////////////////////////////////////////////////////////////////////////////////
ChessMoveSortList & ChessMoveSortList::operator=(const ChessMoveSortList &mo)
{
	if (this != &mo) {
		std::copy(mo.begin(), mo.end(), begin());
		pos = std::next(begin(), mo.size());
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
void ChessMoveSortList::remove(const ChessMove &move)
{
	auto *it = std::remove_if(begin(), end(), 
		[move](const ChessMoveSort &m) { return (m.move == move); });
	if (it != end()) {
		pos = it;
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessMoveSortList::sort(int p, int cnt)
{
	//auto cmp = [](const ChessMoveSort &a, const ChessMoveSort &b) {
	//	return a.getScore() > b.getScore(); };
	//std::stable_sort(std::next(moves.begin(), p), 
	//                 std::next(moves.begin(), p + cnt), cmp);

	const auto *ms = moves.data() + p;
	const auto *me = ms + cnt;
	auto *mi = moves.data() + p;
	while (++mi < me) {

		auto mm = mi->move.data();
		auto mv = mi->value;

		auto *mj = mi - 1;
		while ((mj >= ms) &&
			   (mj->value < mv)) {
			*(mj + 1) = *mj;
			--mj;
		}

		(mj + 1)->move.set(mm);
		(mj + 1)->value = mv;
	}
}

////////////////////////////////////////////////////////////////////////////////
ChessMoveList & ChessMoveList::operator=(const ChessMoveList &mo)
{
	if (this != &mo) {
		std::copy(mo.begin(), mo.end(), begin());
		pos = std::next(begin(), mo.size());
	}
	return *this;
}


} // namespace Cheese

