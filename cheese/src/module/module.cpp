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

#include "module.h"

#include "fen.h"
#include "util/logfile.h"
#include "move.h"
#include "moveparser.h"
#include "search.h"
#include "tools.h"

#include "tablebases.h"

#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <memory>

namespace Cheese {

const std::string ChessModule::name_author = "Patrice Duhamel";

const std::string ChessModule::name_program = "Cheese";

#ifdef SYSTEM64BIT
const std::string ChessModule::name_archi = "64 bits";
#else
const std::string ChessModule::name_archi = "32 bits";
#endif

const std::string nameFileConfig = "cheese.ini";

const std::string defaultOpeningBook = "cheesebook-30.bin";

const std::string logfilename = "logfile.txt";

////////////////////////////////////////////////////////////////////////////////
void ChessModule::parseDefault(const std::string &str)
{
	Tokenizer tk(str);

	if (!tk.hasMoreTokens()) {
		return;
	}

	auto token = tk.nextToken();

	if (token == "uci") {
		setProtocol(GuiProtocol::uci);
		engine.registerListener(this);
		return;
	}

	if (token == "xboard") {
		setProtocol(GuiProtocol::cecp);
		engine.registerListener(this);
		return;
	}

	if (token == "quit") {
		quit = true;
		return;
	}

	Logger::error() << "unknown protocol '" << token << "'";
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::printHelp()
{
	std::cout << name_program << " "
			  << Cheese::project_ver << " by "
			  << name_author << "\n\n"
			  << name_program
			  << " -testsuite HASH_SIZE NB_THREADS {depth|time} VALUE"
		      << " FILENAME.epd" << '\n'
			  << name_program
			  << " -perft DEPTH [-fen \"FEN_STRING\"]" << '\n'
			  << name_program
			  << " -perftd DEPTH [-fen \"FEN_STRING\"]" << '\n'
			  << name_program
			  << " -divide DEPTH [-fen \"FEN_STRING\"]" << '\n'
			  << name_program
			  << " -benchmark HASH_SIZE NB_THREADS {depth|time} VALUE"
			  << " [FILENAME.epd]" << '\n'
			  << name_program
		  	  << " -analyze -fen \"FEN_STRING\" [-hash HASH_SIZE] "
			  << "[-threads NB_THREADS] [-depth DEPTH] [-time TIME] "
			  << "[-nodes NODES] [-multipv MULTIPV] [-exclude MOVELIST] " 
			  << "[-mate VALUE]" << '\n'
			  << name_program
			  << " -probetb \"FEN_STRING\"" << '\n'
			  << name_program
			  << " -probebook \"FEN_STRING\""
			  << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// parse command line
bool ChessModule::parseArguments(const std::vector<std::string> &args,
	bool &err)
{
	err = false;
	if (args.size() == 1) {
		return false;
	}

	const std::string &name = args[1];

	if ((name == "-help") ||
		(name == "-h") ||
		(name == "--help")) {
		printHelp();
		return true;
	}

	if (name == "--version") {
		std::cout << name_program << " "
			      << Cheese::project_ver << " by "
			      << name_author << "\n" 
				  << std::endl;
		return true;
	}

	if (name == "-testsuite") {
		err = !cmdRunTestSuite(args);
		return true;
    }

	if (name == "-perftd") {
		err = !cmdPerftd(args);
		return true;
    }

	if (name == "-perft") {
		err = !cmdPerft(args);
		return true;
    }

	if (name == "-divide") {
		err = !cmdDivide(args);
		return true;
    }

	if (name == "-probetb") {
		err = !cmdProbeTB(args);
		return true;
	}

	if (name == "-probebook") {
		err = !cmdProbeBook(args);
		return true;
	}

	if (name == "-benchmark") {
		err = !cmdBenchmark(args);
		return true;
	} 

	if (name == "-analyze") {
		err = !cmdAnalyze(args);
		return true;
	}

	std::cout << "Unknown command line parameter '"
			  << name << "'"
			  << std::endl;
	err = true;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::loadConfig()
{
	// default values
	cfg.smpMode = "ybwc";
	cfg.bookFile.assign(defaultOpeningBook);
	cfg.personalityFile.assign("");
	cfg.pathTableBase.assign("<empty>");
	cfg.hashSize = hashtable_size_default;
	cfg.elo = max_strength_elo;
	cfg.engineLevel = max_engine_level;
	cfg.threads = 1;
	cfg.minSplitDepth = 4;
	cfg.logLevel = 7; // default = info + warning + error
	cfg.drawScore = 0;
	cfg.tb50MoveRule = 1;
	cfg.tbProbeLimit = 7;
	cfg.tbProbeDepth = 1;
	cfg.uciLimitMode = ELOLimitMode::speed;
	cfg.useBook = false;
	cfg.useStrength = false;
	cfg.usePersonality = false;
	cfg.nullMoves = true;
	cfg.lmr = true;

	ConfigFile conf;
	if (!conf.load(nameFileConfig)) {
		return false;
	}
	
	cfg.smpMode = conf.get("SMPMode", "ybwc");
	if ((cfg.smpMode != "ybwc") && (cfg.smpMode != "lazy")) {
		cfg.smpMode = "ybwc";
		Logger::warning() << "wrong SMPMode value, using default value";		
	}
	cfg.bookFile = conf.get("BookFile", defaultOpeningBook);
	cfg.personalityFile = conf.get("PersonalityFile", "");
	cfg.pathTableBase = conf.get("SyzygyPath", "<empty>");
	cfg.hashSize = conf.getInt("Hash", hashtable_size_default);
	if (cfg.hashSize < 1) {
		cfg.hashSize = hashtable_size_default;
		Logger::warning() << "wrong hash size value, using default value";
	}
	int b = conf.getInt("UseStrength", 0);
	if ((b != 0) && (b != 1)) {
		b = 0;
		Logger::warning() << "wrong UseStrength value, using default value";
	}
	cfg.useStrength = (b == 1);
	cfg.elo = cfg.useStrength ? 
		conf.getInt("StrengthELO", max_strength_elo) : max_strength_elo;

	cfg.engineLevel = conf.getInt("EngineLevel", max_engine_level);
	if ((cfg.engineLevel < 1) || (cfg.engineLevel > max_engine_level)) {
		cfg.engineLevel = max_engine_level;
	}

	cfg.threads = conf.getInt("MaxThreads", 1);
	if (cfg.threads < 1) {
		cfg.threads = 1;
		Logger::warning() << "wrong number of threads, using default value";
	}
	cfg.minSplitDepth = conf.getInt("MinSplitDepth", 4);
	if (cfg.minSplitDepth < 1) {
		cfg.minSplitDepth = 4;
		Logger::warning() << "wrong minSplitDepth value, using default value";
	}
	cfg.logLevel = conf.getInt("LogLevel", 0);
	cfg.drawScore = conf.getInt("DrawScore", 0);
	cfg.tb50MoveRule = conf.getInt("Syzygy50MoveRule", 1);
	if ((cfg.tb50MoveRule != 0) && (cfg.tb50MoveRule != 1)) {
		cfg.tb50MoveRule = 1;
		Logger::warning() << "wrong Syzygy50MoveRule value, using default value";		
	}
	cfg.tbProbeDepth = conf.getInt("SyzygyProbeDepth", 1);
	if (cfg.tbProbeDepth < 0) {
		cfg.tbProbeDepth = 1;
		Logger::warning() << "wrong tbProbeDepth value, using default value";
	}
	cfg.tbProbeLimit = conf.getInt("SyzygyProbeLimit", 7);
	if (cfg.tbProbeLimit < 0) {
		cfg.tbProbeLimit = 1;
		Logger::warning() << "wrong tbProbeLimit value, using default value";
	}
	b = conf.getInt("UCILimitMode", 0);
	if ((b != 0) && (b != 1)) {
		b = 0;
		Logger::warning() << "wrong UCILimitMode value, using default value";		
	}
	cfg.uciLimitMode = (b == 0) ? ELOLimitMode::speed : ELOLimitMode::nodes;
	b = conf.getInt("Book", 0);
	if ((b != 0) && (b != 1)) {
		b = 0;
		Logger::warning() << "wrong Book value, using default value";		
	}
	cfg.useBook = (b == 1);
	b = conf.getInt("UsePersonality", 0);
	if ((b != 0) && (b != 1)) {
		b = 0;
		Logger::warning() << "wrong UsePersonality value, using default value";		
	}
	cfg.usePersonality = (b == 1);
	b = conf.getInt("UseNullMovePruning", 1);
	if ((b != 0) && (b != 1)) {
		b = 1;
		Logger::warning() << "wrong UseNullMovePruning value, using default value";		
	}
	cfg.nullMoves = (b == 1);
	b = conf.getInt("UseLMR", 1);
	if ((b != 0) && (b != 1)) {
		b = 1;
		Logger::warning() << "wrong UseLMR value, using default value";		
	}
	cfg.lmr = (b == 1);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::init(const std::vector<std::string> &args)
{
	if constexpr (Logger::use_log) {
		level_gui = Logger::add_custom_level("GUI");
		Logger::enable_level(Logger::LogLevel::warning, true);
		Logger::enable_level(Logger::LogLevel::error, true);
		Logger::open(logfilename);
	}

	// read config file
	bool cfgFound = loadConfig();

	if (!cfgFound) {
		Logger::warning() << "Config file " << nameFileConfig
					      << " not found, using default parameters";
	}

	Logger::enable((cfg.logLevel != 0));

	// logfile levels
	//   0 = disabled
	//  +1 = info
	//  +2 = warning
	//  +4 = error
	//  +8 = debug
	// +16 = gui
	//  31 = all
	if constexpr (Logger::use_log) {
		if ((cfg.logLevel & 1) == 0) {
			Logger::enable_level(Logger::LogLevel::info, false);
		}
		if ((cfg.logLevel & 2) == 0) {
			Logger::enable_level(Logger::LogLevel::warning, false);
		}
		if ((cfg.logLevel & 4) == 0) {
			Logger::enable_level(Logger::LogLevel::error, false);
		}
		if ((cfg.logLevel & 8) == 0) {
			Logger::enable_level(Logger::LogLevel::debug, false);
		}
		if ((cfg.logLevel & 16) == 0) {
			Logger::enable_level(level_gui, false);
		}
	}

	// program name + version + architecture
	Logger::info() << name_program << " " << Cheese::project_ver
#ifdef _DEBUG
                   << " debug"
#endif
                   << ' ' << name_archi
#ifdef USE_POPCOUNT
                   << " popcount"
#endif
#ifdef USE_PEXT
                   << " pext"
#endif
                   << ' ';

	Logger::info() << "Compiled on " __DATE__ " at " __TIME__;

	// compiler
	Logger::info() << "Compiled by "
#if defined(_MSC_VER)
		"MSVC " << _MSC_FULL_VER << '.' 
                << _MSC_BUILD;
#elif defined(__clang__)
		"Clang " << __clang_major__ << '.' 
                 << __clang_minor__ << '.'
				 << __clang_patchlevel__;
#elif defined(__MINGW64__) 
		"MINGW64 " << __MINGW64_MAJOR_VERSION << '.' 
                   << __MINGW64_MINOR_VERSION;
#elif defined(__MINGW32__) 
		"MINGW32 " << __MINGW32_MAJOR_VERSION << '.' 
                   << __MINGW32_MINOR_VERSION;
#elif defined(__GNUC__)
		"GCC " << __GNUC__ << '.' 
               << __GNUC_MINOR__ << '.'
			   << __GNUC_PATCHLEVEL__;
#else
		"unknonwn";
#endif

	quit = false;

	// precalc some tables for bitboards, search
	ChessEngine::precalcTables();

	if (cfg.smpMode == "ybwc") {
		engine.setSMPMode(SMPMode::ybwc);
	} else
	if (cfg.smpMode == "lazy") {
		engine.setSMPMode(SMPMode::lazy);
	} else {
		cfg.smpMode = "ybwc";
		engine.setSMPMode(SMPMode::ybwc);
	}

	// parse command line
	bool err = false;
	if (parseArguments(args, err)) {
		// quit after command line tools
		if (err) {
			printHelp();
		}
		quit = true;
		return err;
	}

	sendCommand(name_program + ' ' + std::string(Cheese::project_ver) +
		" by " + name_author);

	return true;
}

#ifdef __EMSCRIPTEN__
void ChessModule::initModuleEmscripten()
{
	quit = false;

	cfg.smpMode = "lazy";
	cfg.bookFile.assign("");
	cfg.personalityFile.assign("");
	cfg.pathTableBase.assign("<empty>");
	cfg.hashSize = 16;
	cfg.elo = max_strength_elo;
	cfg.engineLevel = max_engine_level;
	cfg.threads = 1;
	cfg.minSplitDepth = 4;
	cfg.logLevel = 0;
	cfg.drawScore = 0;
	cfg.tb50MoveRule = 1;
	cfg.tbProbeLimit = 7;
	cfg.tbProbeDepth = 1;
	cfg.uciLimitMode = ELOLimitMode::speed;
	cfg.useBook = false;
	cfg.useStrength = false;
	cfg.usePersonality = false;
	cfg.nullMoves = true;
	cfg.lmr = true;


	// precalc some tables for bitboards, search
	ChessEngine::precalcTables();

	// force lazy SMP
	engine.setSMPMode(SMPMode::lazy);
	
	sendCommand(name_program + ' ' + std::string(Cheese::project_ver) +
		" by " + name_author);

	engine.init(cfg.hashSize, cfg.threads);
}
#endif

////////////////////////////////////////////////////////////////////////////////
void ChessModule::free()
{
	// stop and release engine
	ponderingActive = false;

	engine.unregisterListener(this);
	engine.free();

	TableBases::free();

	if constexpr (Logger::use_log) {
		Logger::info() << "End.";
		Logger::close();
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::initEngine()
{
	engine.init(cfg.hashSize, cfg.threads);

	engine.setRemainingMoves(remainingMoves);

	if (cfg.engineLevel == max_engine_level) {
		engine.setELOLimitMode(cfg.uciLimitMode);
		engine.setStrengthELO((cfg.useStrength) ? cfg.elo : 
			max_strength_elo);
	} else {
		engine.setStrengthLevel(cfg.engineLevel);
	}

	engine.setMinSplitDepth(cfg.minSplitDepth);
	engine.setDrawValue(cfg.drawScore);
	engine.setTBProbeDepth(cfg.tbProbeDepth);
	engine.setTB50MoveRule((cfg.tb50MoveRule) ? true : false);
	engine.setTBProbeLimit(cfg.tbProbeLimit);

	TableBases::init(cfg.pathTableBase);

	// load opening book, if one is used
	if (cfg.useBook) {
		if (!engine.loadOpeningBook(cfg.bookFile)) {
			cfg.useBook = false;
		}
	}

	if (cfg.usePersonality) {
		engine.setPersonality(cfg.personalityFile);
	}

	engine.newGame();
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::setProtocol(GuiProtocol p)
{
	guiMode = p;

	switch (p) {
		case	GuiProtocol::uci:
				Logger::info() << "using protocol UCI";
				protocol = std::make_unique<ChessProtocolUCI>(engine, *this);
				protocol->init();
				break;

		case	GuiProtocol::cecp:
				Logger::info() << "using protocol CECP";
				protocol = std::make_unique<ChessProtocolCECP>(engine, *this);
				protocol->init();
				break;

		default:
				protocol = nullptr;
				break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// main loop
void ChessModule::run()
{
	while (!quit) {
		parse();
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::parse()
{
#ifndef __EMSCRIPTEN__
	std::string cmd;
	std::getline(std::cin, cmd);
#else
	std::string cmd = cmdQueue.pop();
#endif

	if (!cmd.empty()) {

		Logger::logger(level_gui) << "<< " << cmd;

		if (guiMode != GuiProtocol::none) {
			if (!protocol->parse(cmd)) {
				parseModuleCommand(cmd);
			}
		} else {
			parseDefault(cmd);
		}
    }
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::sendCommand(const std::string &str) const
{
	std::cout << str << std::endl;

	Logger::logger(level_gui) << ">> " << str;
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::startSearch(bool ponder)
{
	engine.startSearch(ponder);
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::parseModuleCommand(const std::string &cmd)
{
	Tokenizer tk(cmd);

	if (!tk.hasMoreTokens()) {
		return;
	}

	auto token = tk.nextToken();

	if (token == "perftd") {
		if (tk.hasMoreTokens()) {
			auto param = tk.nextToken();
			doPerftd(ChessModule::parseInt(param));
		}
	} else
	if (token == "perft") {
		if (tk.hasMoreTokens()) {
			auto param = tk.nextToken();
            doPerft(ChessModule::parseInt(param));
		}
	} else
	if (token == "divide") {
		if (tk.hasMoreTokens()) {
			auto param = tk.nextToken();
            doDivide(ChessModule::parseInt(param));
		}
	} else
	if (token == "printBoard") {
		std::cout << engine.getBoardRef();
	} else
	if (token == "eval") {
		int ev = engine.evaluate(-mate_value, mate_value);
		std::stringstream ss;
		ss << "eval = "
		   << std::fixed << std::setprecision(2) << (ev / 100.0);
		sendCommand(ss.str());
	} else
	if (token == "printHashKey") {
		sendCommand(Zobrist::hashKeyToString(
			engine.getBoardRef().getHashKey()));
	} else
	if (token == "printFEN") {
		std::string str = engine.getFENPosition(engine.getBoardRef());
		sendCommand(str);
	} else
	if (token == "showBookMoves") {
		engine.showBookMoves();
	} else {
		Logger::debug() << "unknown command : " << token;
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::doPerftd(int depth)
{
	ChessTools tools(engine, engine.getBoardRef());

	Timer timer;
	timer.start();

	tools.perftd(depth);

	auto timesearch = timer.getElapsedTime();

	std::string str = Timer::timeToString(timesearch);

	std::uint64_t nps = (tools.countnode * 1000) / (timesearch + 1);

	sendCommand("Depth " + std::to_string(depth));
	sendCommand("Nodes           = " +
		ChessTools::formatNumber(tools.countnode));
	sendCommand("Nodes captures  = " +
		ChessTools::formatNumber(tools.countcapture));
	sendCommand("Nodes e.p       = " +
		ChessTools::formatNumber(tools.countep));
	sendCommand("Nodes check     = " +
		ChessTools::formatNumber(tools.countcheck));
	sendCommand("Nodes mate      = " +
		ChessTools::formatNumber(tools.countmate));
	sendCommand("Nodes promotion = " +
		ChessTools::formatNumber(tools.countpromotion));
	sendCommand("Nodes castle    = " +
		ChessTools::formatNumber(tools.countcastle));
	sendCommand("Time            = " + str);
	sendCommand("Speed           = " + ChessTools::formatNumber(nps));
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::doPerft(int depth)
{
	ChessTools tools(engine, engine.getBoardRef());

	Timer timer;
	timer.start();

	tools.perft(depth);

	auto timesearch = timer.getElapsedTime();

	std::string str = Timer::timeToString(timesearch);
	std::uint64_t nps = (tools.countnode * 1000) / (timesearch + 1);

	sendCommand("Depth "  + std::to_string(depth));
	sendCommand("Nodes = " + ChessTools::formatNumber(tools.countnode));
	sendCommand("Time  = " + str);
	sendCommand("Speed = " + ChessTools::formatNumber(nps));
}

////////////////////////////////////////////////////////////////////////////////
void ChessModule::doDivide(int depth)
{
	ChessTools tools(engine, engine.getBoardRef());

	Timer timer;
	timer.start();

	tools.divide(depth);

	auto timesearch = timer.getElapsedTime();

	std::uint64_t total = 0ULL;

	std::stringstream ss;
	for (int n=0; n<tools.nbDivide; n++) {

		ss << std::setw(10) << tools.listDivide[n].name << " : "
		   << ChessTools::formatNumber(tools.listDivide[n].count) << '\n';

		total += tools.listDivide[n].count;
	}

	ss << "\ntotal : " << ChessTools::formatNumber(total);
	sendCommand(ss.str());

	std::string str = Timer::timeToString(timesearch);

	std::uint64_t nps = (total * 1000) / (timesearch + 1);

	sendCommand("Time  = " + str);
	sendCommand("Speed = " + ChessTools::formatNumber(nps));
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::cmdRunTestSuite(const std::vector<std::string> &args)
{
	if (args.size() != 7) {
		std::cout << "bad parameter" << std::endl;
		return false;			
	}

	if ((!ConfigFile::isNumber(args[2])) ||
  	    (!ConfigFile::isNumber(args[3])) ||
        (!ConfigFile::isNumber(args[5]))) {
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	int mode = 0;
	int htsize = std::stoi(args[2]);
	int nbcpu = std::stoi(args[3]);
	if (args[4] == "depth") {
		mode = 0;
	} else
	if (args[4] == "time") {
		mode = 1;
	} else {
		std::cout << "bad parameter" << std::endl;
		return false;			
	}	
	int value = std::stoi(args[5]);
	const std::string &fname = args[6];

	if (htsize < 1) {
		std::cout << "wrong hash table size" << std::endl;
		return false;
	}
	if (nbcpu < 1) {
		std::cout << "wrong number of threads" << std::endl;
		return false;
	}
	if (mode == 0) {
		if (value < 1) {
			std::cout << "wrong depth value" << std::endl;
			return false;
		}
	} else {
		if (value < 1) {
			std::cout << "wrong time value" << std::endl;
			return false;
		}
	}

	ChessTools tools(engine, engine.getBoardRef());

	engine.init(htsize, nbcpu);

	engine.setTBProbeDepth(cfg.tbProbeDepth);
	engine.setTB50MoveRule((cfg.tb50MoveRule) ? true : false);
	engine.setTBProbeLimit(cfg.tbProbeLimit);
	TableBases::init(cfg.pathTableBase);

	engine.newGame();

	engine.setModeRunTestSuite(true);
	if (mode == 0) {
		engine.setSearchFixedDepth(value);
		engine.setInfiniteSearch(true);
	} else {
		engine.setSearchFixedTime(value * 1000);
	}

    if (!tools.runTestSuite(fname)) {
		std::cout << "file not found " << fname << std::endl;
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::cmdPerftd(const std::vector<std::string> &args)
{
	if ((args.size() != 3) && (args.size() != 5)){
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	if (!ConfigFile::isNumber(args[2])) {
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	int depth = std::stoi(args[2]);
	
	std::string txtfen;
	if (args.size() == 5) {
		if (args[3] == "-fen") {
			txtfen = args[4];
		} else {
			std::cout << "bad parameter" << std::endl;
			return false;
		}
	}

	if (depth < 1) {
		std::cout << "wrong depth value" << std::endl;
		return false;
	}

	engine.init(hashtable_size_min);

    if (!txtfen.empty()) {
        if (!engine.setFENPosition(txtfen)) {
            std::cout << "Bad FEN string !" << std::endl;
            return false;
        }
    }
    doPerftd(depth);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::cmdPerft(const std::vector<std::string> &args)
{
	if ((args.size() != 3) && (args.size() != 5)){
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	if (!ConfigFile::isNumber(args[2])) {
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	int depth = std::stoi(args[2]);
	
	std::string txtfen;
	if (args.size() == 5) {
		if (args[3] == "-fen") {
			txtfen = args[4];
		} else {
			std::cout << "bad parameter" << std::endl;
			return false;
		}
	}

	if (depth < 1) {
		std::cout << "wrong depth value" << std::endl;
		return false;
	}

    engine.init(hashtable_size_min);

    if (!txtfen.empty()) {
		if (!engine.setFENPosition(txtfen)) {
            std::cout << "Bad FEN string !" << std::endl;
            return false;
        }
    }
    doPerft(depth);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::cmdDivide(const std::vector<std::string> &args)
{
	if ((args.size() != 3) && (args.size() != 5)){
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	if (!ConfigFile::isNumber(args[2])) {
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	int depth = std::stoi(args[2]);
	
	std::string txtfen;
	if (args.size() == 5) {
		if (args[3] == "-fen") {
			txtfen = args[4];
		} else {
			std::cout << "bad parameter" << std::endl;
			return false;
		}
	}

	if (depth < 1) {
		std::cout << "wrong depth value" << std::endl;
		return false;
	}

    engine.init(hashtable_size_min);

    if (!txtfen.empty()) {
        if (!engine.setFENPosition(txtfen)) {
            std::cout << "bad FEN string !" << std::endl;
            return false;
        }
    }
    doDivide(depth);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::cmdProbeTB(const std::vector<std::string> &args)
{
	if (args.size() != 3) {
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	const std::string &fen = args[2];

	engine.init(1);
	engine.setMaxThreads(1);
	engine.newGame();

	if (!engine.setFENPosition(fen)) {
		std::cout << "bad FEN string !" << std::endl;
		return false;
    }

	engine.setTBProbeDepth(cfg.tbProbeDepth);
	engine.setTB50MoveRule((cfg.tb50MoveRule) ? true : false);
	engine.setTBProbeLimit(cfg.tbProbeLimit);
	TableBases::init(cfg.pathTableBase);

	if (TableBases::largest() == 0) {
		std::cout << "Tablebases not loaded." << std::endl;
		return false;
	}

	std::cout << "Tablebases loaded = " <<
				TableBases::largest() << std::endl;
	std::cout << "FEN : " << fen << std::endl;

	int score = 0;;
	int r = TableBases::probe_wdl(engine.getBoardRef(), score);
	std::string str = "no result";
	if (r != 0) {
		str = TableBases::wdl_to_string(score);
	}
	std::cout << "WDL = " << str << std::endl;

	ChessMoveSortList moves;
	score = 0;
	r = TableBases::probe_root(engine.getBoardRef(), score, moves, true);
	str = "no result";
	if (r != 0) {
		str = TableBases::wdl_to_string(score);
	}

	moves.sort(0, moves.size());
	for (const auto &move : moves) {
		std::cout << ChessMoveParser::move_to_string_lan(move.move)
				  << " = "
			      << TableBases::wdl_to_string(move.value) << '\n';
	}
	
	std::cout << std::endl;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::cmdProbeBook(const std::vector<std::string> &args)
{
	if (args.size() != 3) {
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	const std::string &fen = args[2];

	engine.init(1);
	engine.setMaxThreads(1);
	engine.newGame();

	if (!engine.setFENPosition(fen)) {
		std::cout << "bad FEN string !" << std::endl;
		return false;
    }

	if (cfg.useBook) {
		if (!engine.loadOpeningBook(cfg.bookFile)) {
			std::cout << "book file not found" << std::endl;
			return false;
		}
	} else {
		std::cout << "book not loaded" << std::endl;
		return false;
	}

	engine.showBookMoves();

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::cmdBenchmark(const std::vector<std::string> &args)
{
	if ((args.size() < 6) || (args.size() > 7)) {
		std::cout << "bad parameter" << std::endl;
		return false;			
	}

	if ((!ConfigFile::isNumber(args[2])) ||
  	    (!ConfigFile::isNumber(args[3])) ||
        (!ConfigFile::isNumber(args[5]))) {
		std::cout << "bad parameter" << std::endl;
		return false;
	}

	int mode = 0;
	int htsize = std::stoi(args[2]);
	int nbcpu = std::stoi(args[3]);
	if (args[4] == "depth") {
		mode = 0;
	} else
	if (args[4] == "time") {
		mode = 1;
	} else {
		std::cout << "bad parameter" << std::endl;
		return false;			
	}	
	int value = std::stoi(args[5]);
	std::string fname = (args.size() == 7) ? args[6] : "";

	const std::vector<std::string> benchmarkFens = {
		"r2qkbnr/ppp2p1p/2n5/3P4/2BP1pb1/2N2p2/PPPQ2PP/R1B2RK1 b kq -  ",
		"r2qkbnr/ppp2p1p/8/nB1P4/3P1pb1/2N2p2/PPPQ2PP/R1B2RK1 b kq - ",
		"r2qkbnr/pp3p1p/2p5/nB1P4/3P1Qb1/2N2p2/PPP3PP/R1B2RK1 b kq - ",
		"r2qkb1r/pp3p1p/2p2n2/nB1P4/3P1Qb1/2N2p2/PPP3PP/R1B1R1K1 b kq - ",
		"r2q1b1r/pp1k1p1p/2P2n2/nB6/3P1Qb1/2N2p2/PPP3PP/R1B1R1K1 b - - ",
		"r2q1b1r/p2k1p1p/2p2n2/nB6/3PNQb1/5p2/PPP3PP/R1B1R1K1 b - - ",
		"r2q1b1r/p2k1p1p/2p5/nB6/3Pn1Q1/5p2/PPP3PP/R1B1R1K1 b - - ",
		"r2q1b1r/p1k2p1p/2p5/nB6/3PR1Q1/5p2/PPP3PP/R1B3K1 b - - ",
		"r2q1b1r/p1k2p1p/8/np6/3PR3/5Q2/PPP3PP/R1B3K1 b - - ",
		"r4b1r/p1kq1p1p/8/np6/3P1R2/5Q2/PPP3PP/R1B3K1 b - - ",
		"r6r/p1kqbR1p/8/np6/3P4/5Q2/PPP3PP/R1B3K1 b - - ",
		"5r1r/p1kqbR1p/8/np6/3P1B2/5Q2/PPP3PP/R5K1 b - - ",
		"5r1r/p2qbR1p/1k6/np2B3/3P4/5Q2/PPP3PP/R5K1 b - - ",
		"5rr1/p2qbR1p/1k6/np2B3/3P4/2P2Q2/PP4PP/R5K1 b - - ",
		"5rr1/p2qbR1p/1kn5/1p2B3/3P4/2P2Q2/PP4PP/4R1K1 b - - ",
		"4qRr1/p3b2p/1kn5/1p2B3/3P4/2P2Q2/PP4PP/4R1K1 b - - ",
		"5qr1/p3b2p/1kn5/1p1QB3/3P4/2P5/PP4PP/4R1K1 b - - ",
		"5q2/p3b2p/1kn5/1p1QB1r1/P2P4/2P5/1P4PP/4R1K1 b - - ",
		"5q2/p3b2p/1kn5/3QB1r1/p1PP4/8/1P4PP/4R1K1 b - - ",
		"5q2/p3b2p/1k6/3QR1r1/p1PP4/8/1P4PP/6K1 b - - ",
		"5q2/p3b2p/1k6/4Q3/p1PP4/8/1P4PP/6K1 b - - ",
		"3q4/p3b2p/1k6/2P1Q3/p2P4/8/1P4PP/6K1 b - - ",
		"3q4/p3b2p/8/1kP5/p2P4/8/1P2Q1PP/6K1 b - - ",
		"3q4/p3b2p/8/2P5/pk1P4/3Q4/1P4PP/6K1 b - - "
	};

	if (htsize < 1) {
		std::cout << "wrong hash table size" << std::endl;
		return false;
	}
	if (nbcpu < 1) {
		std::cout << "wrong number of threads" << std::endl;
		return false;
	}
	if (mode == 0) {
		if (value < 1) {
			std::cout << "wrong depth value" << std::endl;
			return false;
		}
	} else {
		if (value < 1) {
			std::cout << "wrong time value" << std::endl;
			return false;
		}
	}

	std::vector<std::string> fens;

	bool usefile = ChessFENParser::load(fname, fens);

	ChessTools tools(engine, engine.getBoardRef());

	engine.init(htsize, nbcpu);
	engine.newGame();

	engine.setTBProbeDepth(cfg.tbProbeDepth);
	engine.setTB50MoveRule((cfg.tb50MoveRule) ? true : false);
	engine.setTBProbeLimit(cfg.tbProbeLimit);
	TableBases::init(cfg.pathTableBase);

	engine.setModeRunTestSuite(false);
	if (mode == 0) {
		engine.setSearchFixedDepth(value);
		engine.setInfiniteSearch(true);
	} else {
		engine.setSearchFixedTime(value * 1000);
	}

	tools.runBenchmark((usefile) ? fens : benchmarkFens, mode);

	std::cout << "Hash table size : " << htsize << " Mb" << std::endl;
	if (mode == 0) {
		std::cout << "Search depth : " << value << std::endl;
	} else {
		std::cout << "Search time : " << value << " s" << std::endl;
	}
	std::cout << "Number of threads : " << nbcpu << std::endl;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessModule::cmdAnalyze(const std::vector<std::string> &args)
{
	int tm = 0, depth = 0, nbcpu = 1, htsize = 256, nodes = 0, nbmpv = 1, 
		mate = 0;
	std::string fen, excludestr;

	std::size_t p = 1;
	while (p < args.size()) {

		if (args[p] == "-fen") {
			++p;
			if (p < args.size()) {
				fen = args[p];
			}
		}

		if (args[p] == "-time") {
			++p;
			if (p < args.size()) {
				tm = std::stoi(args[p]);
				if (tm < 1) {
					std::cout << "wrong time value" << std::endl;
					return false;
				}
			}
		}

		if (args[p] == "-depth") {
			++p;
			if (p < args.size()) {
				depth = std::stoi(args[p]);
				if (depth < 1) {
					std::cout << "wrong depth value" << std::endl;
					return false;
				}
			}
		}

		if (args[p] == "-mate") {
			++p;
			if (p < args.size()) {
				mate = std::stoi(args[p]);
			}
		}

		if (args[p] == "-multipv") {
			++p;
			if (p < args.size()) {
				nbmpv = std::stoi(args[p]);
				if (nbmpv < 1) {
					std::cout << "wrong multipv value" << std::endl;
					return false;
				}
			}
		}

		if (args[p] == "-nodes") {
			++p;
			if (p < args.size()) {
				nodes = std::stoi(args[p]);
				if (nodes < 1) {
					std::cout << "wrong nodes value" << std::endl;
					return false;
				}
			}
		}

		if (args[p] == "-exclude") {
			++p;
			if (p < args.size()) {
				excludestr = args[p];
			}
		}

		if (args[p] == "-hash") {
			++p;
			if (p < args.size()) {
				htsize = std::stoi(args[p]);
				if (htsize < 1) {
					std::cout << "wrong hash table size" << std::endl;
					return false;
				}
			}
		}

		if (args[p] == "-threads") {
			++p;
			if (p < args.size()) {
				nbcpu = std::stoi(args[p]);
				if (nbcpu < 1) {
					std::cout << "wrong number of threads" << std::endl;
					return false;
				}
			}
		}

		++p;
	}

	ChessTools tools(engine, engine.getBoardRef());

	engine.init(htsize, nbcpu);
	engine.newGame();

	engine.setTBProbeDepth(cfg.tbProbeDepth);
	engine.setTB50MoveRule((cfg.tb50MoveRule) ? true : false);
	engine.setTBProbeLimit(cfg.tbProbeLimit);
	TableBases::init(cfg.pathTableBase);

	engine.setModeRunTestSuite(false);
	if (depth != 0) {
		engine.setSearchFixedDepth(depth);
		engine.setInfiniteSearch(true);
	}
	if (nodes != 0) {
		engine.setSearchFixedNodes(nodes);
		engine.setInfiniteSearch(true);
	}
	if (tm != 0) {
		engine.setSearchFixedTime(tm * 1000);
	}
	if (mate != 0) {
		engine.setSearchMateValue(mate);
		engine.setInfiniteSearch(true);
	}

	engine.setMultiPV(nbmpv);

	std::vector<std::string> excludemoves;
	std::stringstream sstr(excludestr);
	std::string str;
	while (std::getline(sstr, str, ',')) {
		excludemoves.push_back(str);
	}

	tools.analyze(fen, excludemoves);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
int ChessModule::parseInt(const std::string &str)
{
	if (ConfigFile::isNumber(str)) {
		return std::stoi(str);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
double ChessModule::parseDouble(const std::string &str)
{
	if (ConfigFile::isDouble(str)) {
		return std::stod(str);
	}
	return 0.0;
}

} // namespace Cheese

