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

#ifndef CHEESE_MODULE_H_
#define CHEESE_MODULE_H_

#include "config.h"

#include "util/configfile.h"
#include "engine.h"
#include "listener.h"
#include "protocol.h"
#include "protocoluci.h"
#include "protocolcecp.h"

#include "util/tokenizer.h"

#include <sstream>
#include <string>
#include <vector>

#include <condition_variable>
#include <mutex>
#include <queue>

namespace Cheese {

// Commands queue for emscripten version
#ifdef __EMSCRIPTEN__
class CmdQueue {

	std::mutex mutex;
	std::condition_variable cond;
	std::queue<std::string> commands;

public:
	void push(std::string str) {
		std::unique_lock<std::mutex> lock(mutex);
		commands.push(std::move(str));
		lock.unlock();
		cond.notify_one();
	}

	std::string pop() {
		std::unique_lock<std::mutex> lock(mutex);
		while (commands.empty()) cond.wait(lock);
		std::string s = std::move(commands.front());
		commands.pop();
		return s;
	}
};
#endif

// GUI protocols
enum class GuiProtocol {

	// unknown protocol (default)
	none,

	// UCI protocol
	uci,

	// CECP protocol (xboard)
	cecp
};

////////////////////////////////////////////////////////////////////////////////
// options loaded from config file
struct ChessModuleConfig {

	// SMP algorithm : ybwc or lazy
	std::string smpMode;

	std::string bookFile;

	std::string personalityFile;

	std::string pathTableBase;

	// default hash size
	int hashSize;

	// default strength in ELO
	int elo;

	// engine level instead of ELO
	int engineLevel;

	// default max number of threads
	int threads;

	// minimum split depth
	int minSplitDepth;

	// default log levels
	int logLevel;

	// contempt factor
	int drawScore;

	int tb50MoveRule;

	int tbProbeLimit;

	int tbProbeDepth;

	// 0 = reduce speed, 1 = recuce nodes
	ELOLimitMode uciLimitMode;

	// default use of opening book
	bool useBook;

	// default use of strength
	bool useStrength;

	// use personality file
	bool usePersonality;

	// use null moves pruning
	bool nullMoves;

	// use LMR
	bool lmr;

};

////////////////////////////////////////////////////////////////////////////////
// Chess module is the main object, to use chess engine with command line
// or gui
class ChessModule : public ChessEngineListener {

public:

	ChessEngine engine;

	std::unique_ptr<ChessProtocol> protocol;

	// configuration options
	ChessModuleConfig cfg;

	// current ponder move
	ChessMove movePonder;

#ifdef __EMSCRIPTEN__
	CmdQueue cmdQueue;
#endif

	GuiProtocol guiMode {GuiProtocol::none};

	Logger::LogLevel level_gui { Logger::LogLevel::custom };

	int remainingMoves {0};

	int maxGameMoves {0};

	bool quit {false};

	// if pondering option is enable
	bool ponderingActive {false};

	// if we are pondering
	bool pondernextmove {false};

	static const std::string name_archi;

	static const std::string name_author;

	static const std::string name_program;


	ChessModule() = default;

	bool init(const std::vector<std::string> &args);

#ifdef __EMSCRIPTEN__
	void initModuleEmscripten();

	CmdQueue *getCmdQueue() { return &cmdQueue; }
#endif

	void initEngine();

	// main loop
	void run();

	void free();

	void exit();

	void setProtocol(GuiProtocol p);

	ChessModuleConfig &getConfig();

	// send a command string to gui
	void sendCommand(const std::string &str) const;

	void doPerftd(int depth);

	void doPerft(int depth);

	void doDivide(int depth);

	void startSearch(bool ponder);

	// convert string to int
	// return 0 if the string is not a number
	[[nodiscard]]
	static int parseInt(const std::string &str);

	// convert string to double
	// return 0.0 if the string is not a double
	[[nodiscard]]
	static double parseDouble(const std::string &str);

	[[nodiscard]]
	static int parseInt(const std::string_view &str)
	{
		return parseInt(std::string(str));
	}

	[[nodiscard]]
	static double parseDouble(const std::string_view &str)
	{
		return parseDouble(std::string(str));
	}

private:

	void parse();

	void parseDefault(const std::string &str);

	// parse special commands 
	void parseModuleCommand(const std::string &cmd);

	bool parseArguments(const std::vector<std::string> &args, bool &err);

	bool loadConfig();

	void printHelp();

	bool cmdRunTestSuite(const std::vector<std::string> &args);

	bool cmdPerftd(const std::vector<std::string> &args);

	bool cmdPerft(const std::vector<std::string> &args);

	bool cmdDivide(const std::vector<std::string> &args);

	bool cmdProbeTB(const std::vector<std::string> &args);

	bool cmdProbeBook(const std::vector<std::string> &args);

	bool cmdBenchmark(const std::vector<std::string> &args);

	bool cmdAnalyze(const std::vector<std::string> &args);

	void onEndSearch(ChessEngineNotifier &notifier,
		ChessMove bm, ChessMove pm, int depth, int ply, std::uint64_t cntnodes,
		std::uint64_t searchtime) override;

	void onSendMultiPV(ChessEngineNotifier &notifier, int num, int depth,
		int score, const ChessPVMoveList &mlist, int count, 
		std::uint64_t cntnodes, std::uint64_t searchtime, int maxply, int bound, 
		std::uint64_t tbhits, std::uint64_t hashfull) override;

	void onSearchRootMove(ChessEngineNotifier &notifier, int depth, 
		ChessMove move, int num) override;

	void onSendInfoMessage(ChessEngineNotifier &notifier,
		const std::string &msg) override;
};

////////////////////////////////////////////////////////////////////////////////
inline ChessModuleConfig &ChessModule::getConfig()
{
	return cfg;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessModule::exit()
{
	quit = true;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessModule::onEndSearch(
	[[maybe_unused]] ChessEngineNotifier &notifier,
	ChessMove bm, ChessMove pm, int depth, int ply,
	std::uint64_t cntnodes, std::uint64_t searchtime)
{
	protocol->onEndSearch(bm, pm, depth, ply, cntnodes, searchtime);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessModule::onSendMultiPV(
	[[maybe_unused]] ChessEngineNotifier &notifier,
	int num, int depth, int score, const ChessPVMoveList &mlist, int count,
	std::uint64_t cntnodes, std::uint64_t searchtime, int maxply, int bound,
	std::uint64_t tbhits, std::uint64_t hashfull)
{
	protocol->onSendMultiPV(num, depth, score, mlist, count, cntnodes,
		searchtime, maxply, bound, tbhits, hashfull);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessModule::onSearchRootMove(
	[[maybe_unused]] ChessEngineNotifier &notifier,
	int depth, ChessMove move, int num)
{
	protocol->onSearchRootMove(depth, move, num);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessModule::onSendInfoMessage(
	[[maybe_unused]] ChessEngineNotifier &notifier,
	const std::string &msg)
{
	protocol->onSendInfoMessage(msg);
}

} // namespace Cheese

#endif //CHEESE_MODULE_H_
