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

#ifndef CHEESE_PROTOCOLCECP_H_
#define CHEESE_PROTOCOLCECP_H_

#include "config.h"

#include "protocol.h"
#include "util/tokenizer.h"

namespace Cheese {

class ChessEngine;
class ChessModule;

////////////////////////////////////////////////////////////////////////////////
class ChessProtocolCECP final : public ChessProtocol {

	// version of protocol
	int protocolversion {0};

	int countHalfMove {0};

	// cecp force mode
	bool forcemode {false};

	bool firstThink {false};

	bool showthinking {true};

	std::string pondernewmove;

public:

	ChessProtocolCECP(ChessEngine &e, ChessModule &m);

	~ChessProtocolCECP() override = default;

	void init() override;

	bool parse(const std::string &str) override;

private:

	void initFeatures();

	void parseOption(Tokenizer &tk);

	void parseMove(const std::string_view &command);

	void onEndSearch(ChessMove bm, ChessMove pm,
		int depth, int ply, std::uint64_t cntnodes, 
		std::uint64_t searchtime) override;

	void onSendMultiPV(int num, int depth, int score, 
		const ChessPVMoveList &mlist, int count, std::uint64_t cntnodes, 
		std::uint64_t searchtime, int maxply, int bound, 
		std::uint64_t tbhits, std::uint64_t hashfull) override;

	void onSearchRootMove([[maybe_unused]] int depth, 
		[[maybe_unused]] ChessMove move,
		[[maybe_unused]] int num) override
	{}

	void onSendInfoMessage(const std::string &msg) override;

};

} // namespace Cheese

#endif //CHEESE_PROTOCOLCECP_H_
