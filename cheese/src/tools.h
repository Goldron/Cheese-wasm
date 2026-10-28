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

#ifndef CHEESE_TOOLS_H_
#define CHEESE_TOOLS_H_

#include "config.h"

#include "engine.h"
#include "listener.h"
#include "movegen.h"

#include <array>
#include <string>
#include <vector>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
// information used in divide function
struct ChessDivide {

	std::uint64_t count {0};

	std::string name;
};

////////////////////////////////////////////////////////////////////////////////
struct ChessSearchResult {

	std::uint64_t countNodes {0};

	std::uint64_t searchTime {0};

	ChessMove bestMove {0};

	ChessMove ponderMove {0};

	int maxDepth {0};

	int maxPly {0};
};

////////////////////////////////////////////////////////////////////////////////
// all tools to test / benchmark
class ChessTools : public ChessEngineListener
{
public:

	// pointer to the chess engine
	ChessEngine &engine;

	// pointer to the board position
	ChessBoard &board;

	std::array<ChessDivide, max_play_moves> listDivide;

	int nbDivide {0};

	std::uint64_t countnode {0};

	std::uint64_t countcapture {0};

	std::uint64_t countep {0};

	std::uint64_t countpromotion {0};

	std::uint64_t countcastle {0};

	std::uint64_t countcheck {0};

	std::uint64_t countmate {0};

	// results from last search
	ChessSearchResult searchResult;

	bool muted {false};


	ChessTools(ChessEngine &e, ChessBoard &b);

	~ChessTools() override;

	void divide(int depth);

	[[nodiscard]]
	std::uint64_t searchAllMoves(int depth);

	void perftd(int depth);

	void perft(int depth);

	void doPerft(int maxdepth);

	// run a test suite, from a .EPD file
	bool runTestSuite(const std::string &filename);

	void runBenchmark(const std::vector<std::string> &fens, int mode);

	void analyze(const std::string &fen, const std::vector<std::string> 
		&excludemoves);

	[[nodiscard]]
	static std::string formatNumber(std::uint64_t v);

	void onEndSearch(ChessEngineNotifier &notifier,
		ChessMove bm, ChessMove pm, int depth, int ply, std::uint64_t cntnodes,
		std::uint64_t searchtime) override;

	void onSendMultiPV(ChessEngineNotifier &notifier, int num, int depth,
		int score, const ChessPVMoveList &mlist, int count, 
		std::uint64_t cntnodes, std::uint64_t searchtime, int maxply, 
		int bound, std::uint64_t tbhits, std::uint64_t hashfull) override;

	void onSearchRootMove([[maybe_unused]]ChessEngineNotifier &notifier,
		[[maybe_unused]] int depth, [[maybe_unused]] ChessMove move, 
		[[maybe_unused]] int num) override
	{}

	void onSendInfoMessage([[maybe_unused]] ChessEngineNotifier &notifier,
		[[maybe_unused]] const std::string &msg) override
	{}

};

} // namespace Cheese

#endif //CHEESE_TOOLS_H_
