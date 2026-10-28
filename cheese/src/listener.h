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

#ifndef CHEESE_LISTENER_H_
#define CHEESE_LISTENER_H_

#include "config.h"

#include "move.h"
#include "pv.h"

#include <string>
#include <vector>

namespace Cheese {

class ChessEngineNotifier;

////////////////////////////////////////////////////////////////////////////////
class ChessEngineListener {

	friend class ChessEngineNotifier;

public:

	ChessEngineListener() = default;

	virtual ~ChessEngineListener() = default;

private:

	virtual	void onEndSearch(ChessEngineNotifier &notifier,
		ChessMove bm, ChessMove pm, int depth, int ply, std::uint64_t cntnodes,
		std::uint64_t searchtime) = 0;

	virtual void onSendMultiPV(ChessEngineNotifier &notifier, int num,
		int depth, int score, const ChessPVMoveList &mlist, int count,
		std::uint64_t cntnodes, std::uint64_t searchtime, int maxply, int bound,
		std::uint64_t tbhits, std::uint64_t hashfull) = 0;

	virtual void onSearchRootMove(ChessEngineNotifier &notifier, 
		int depth, ChessMove move, int num) = 0;

	virtual void onSendInfoMessage(ChessEngineNotifier &notifier,
		const std::string &msg) = 0;
};

////////////////////////////////////////////////////////////////////////////////
class ChessEngineNotifier {

	std::vector<ChessEngineListener *> listeners;

public:

	ChessEngineNotifier() = default;

	virtual ~ChessEngineNotifier() = default;

	void registerListener(ChessEngineListener *l);

	void unregisterListener(ChessEngineListener *l);

	void onEndSearch(ChessMove bm, ChessMove pm, int depth,
		int ply, std::uint64_t cntnodes, std::uint64_t searchtime);

	void onSendMultiPV(int num, int depth, int score, 
		const ChessPVMoveList &mlist, int count, std::uint64_t cntnodes, 
		std::uint64_t searchtime, int maxply, int bound, std::uint64_t tbhits,
		std::uint64_t hashfull);

	void onSearchRootMove(int depth, ChessMove move, int num);

	void onSendInfoMessage(const std::string &msg);
};

} // namespace Cheese

#endif //CHEESE_LISTENER_H_
