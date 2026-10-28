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

#include "listener.h"

#include "util/logfile.h"

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
void ChessEngineNotifier::registerListener(ChessEngineListener *l)
{
	listeners.push_back(l);
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngineNotifier::unregisterListener(ChessEngineListener *l)
{
	listeners.erase(std::remove(listeners.begin(), listeners.end(), l),
		listeners.end());
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngineNotifier::onEndSearch(ChessMove bm,
	ChessMove pm, int depth, int ply, std::uint64_t cntnodes, 
	std::uint64_t searchtime)
{
	for (auto &l : listeners) {
		l->onEndSearch(*this, bm, pm, depth, ply, cntnodes, searchtime);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngineNotifier::onSendMultiPV(int num, int depth, int score,
	const ChessPVMoveList &mlist, int count, std::uint64_t cntnodes, 
	std::uint64_t searchtime, int maxply, int bound, std::uint64_t tbhits,
	std::uint64_t hashfull)
{
	for (auto &l : listeners) {
		l->onSendMultiPV(*this, num, depth, score, mlist, count, cntnodes,
			searchtime, maxply, bound, tbhits, hashfull);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngineNotifier::onSearchRootMove(int depth, ChessMove move, int num)
{
	for (auto &l : listeners) {
		l->onSearchRootMove(*this, depth, move, num);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessEngineNotifier::onSendInfoMessage(const std::string &msg)
{
	for (auto &l : listeners) {
		l->onSendInfoMessage(*this, msg);
	}
}

} // namespace Cheese
