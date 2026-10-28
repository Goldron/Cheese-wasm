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

#include "protocoluci.h"

#include "util/logfile.h"
#include "board.h"
#include "engine.h"
#include "module.h"
#include "tablebases.h"

#include <iomanip>
#include <string>

namespace Cheese {

const std::array<std::string, 3> strBound = { "", "upperbound ",
	"lowerbound " };

const std::array<std::string, 2> strMate = { "cp", "mate" };

////////////////////////////////////////////////////////////////////////////////
ChessProtocolUCI::ChessProtocolUCI(ChessEngine &e, ChessModule &m) :
	ChessProtocol(e, m)
{
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolUCI::init()
{
	auto &cfg = mainModule.getConfig();

	mainModule.sendCommand("id name " + ChessModule::name_program + ' ' +
		std::string(Cheese::project_ver));
	mainModule.sendCommand("id author " + ChessModule::name_author);
	mainModule.sendCommand("option name Hash type spin default "
		+ std::to_string(cfg.hashSize) +
		" min " + std::to_string(hashtable_size_min) +
		" max " + std::to_string(hashtable_size_max));
	mainModule.sendCommand("option name Ponder type check default false");
#ifndef __EMSCRIPTEN__
	mainModule.sendCommand("option name OwnBook type check default " +
		std::string(cfg.useBook ? "true" : "false"));
	mainModule.sendCommand("option name Book type string default " +
		cfg.bookFile);
#endif
	mainModule.sendCommand("option name Clear Hash type button");
#ifndef __EMSCRIPTEN__
	mainModule.sendCommand("option name UsePersonality type check default " +
		std::string(cfg.usePersonality ? "true" : "false"));
	mainModule.sendCommand("option name PersonalityFile type string default " +
		cfg.personalityFile);
#endif
	mainModule.sendCommand("option name UCI_Chess960 type check default false");
	mainModule.sendCommand("option name MultiPV type spin default 1 min 1 max " 
		+ std::to_string(max_multipv));
	mainModule.sendCommand("option name NullMovePruning type check default " +
		std::string(cfg.nullMoves ? "true" : "false"));
	mainModule.sendCommand("option name LateMoveReduction type check default " 
		+ std::string(cfg.lmr ? "true" : "false"));
	mainModule.sendCommand("option name Threads type spin default " +
		std::to_string(cfg.threads) + " min 1 max " +
		std::to_string(max_threads));
	mainModule.sendCommand("option name SMPMode type combo default " +
		cfg.smpMode + " var ybwc var lazy");
	mainModule.sendCommand("option name UCI_LimitStrength type check default " 
		+ std::string(cfg.useStrength ? "true" : "false"));
	mainModule.sendCommand("option name UCI_Elo type spin default " +
		std::to_string(cfg.elo) + " min " +
		std::to_string(min_strength_elo) + " max " +
		std::to_string(max_strength_elo));
	std::string strlimitmode = (cfg.uciLimitMode == ELOLimitMode::speed) ? 
		"speed" : "nodes";
	mainModule.sendCommand("option name LimitStrengthMode type combo default "
		+ strlimitmode + " var speed var nodes");
	mainModule.sendCommand("option name EngineStrength type spin default "
		+ std::to_string(max_engine_level) + " min 1 max " + 
		std::to_string(max_engine_level));	
	mainModule.sendCommand("option name DrawScore type spin default " +
		std::to_string(cfg.drawScore) + " min -100 max 100");
#ifdef USE_TABLEBASES
	mainModule.sendCommand("option name SyzygyPath type string default " +
		cfg.pathTableBase);
	mainModule.sendCommand("option name SyzygyProbeDepth type spin default " +
		std::to_string(cfg.tbProbeDepth) + " min 1 max 100");
	mainModule.sendCommand("option name Syzygy50MoveRule type check default " +
		std::string((cfg.tb50MoveRule == 1) ? "true" : "false"));
	mainModule.sendCommand("option name SyzygyProbeLimit type spin default " +
		std::to_string(cfg.tbProbeLimit) + " min 0 max 7");
#endif

	mainModule.initEngine();

	mainModule.sendCommand("uciok");
}

////////////////////////////////////////////////////////////////////////////////
bool ChessProtocolUCI::parse(const std::string &str)
{
	Tokenizer tk(str);

	if (!tk.hasMoreTokens()) {
		return true;
	}

	auto token = tk.nextToken();

	if (token == "go") {
		parseGo(tk);
		mainModule.startSearch(mainModule.pondernextmove);
	} else
	if (token == "position") {
		parsePosition(tk);
	} else
	if (token == "isready") {
		mainModule.sendCommand("readyok");
	} else
	if (token == "ponderhit") {

		// the user has played the expected move
		if (engine.isThinking()) {
			// => continue the search or play
			engine.ponderHit();
		}

	} else
	if (token == "setoption") {
	    parseOption(tk);
	} else
	if (token == "quit") {
		mainModule.exit();
	} else
	if (token == "ucinewgame") {

		// turn off pondering
		mainModule.movePonder.clear();

		auto &cfg = mainModule.getConfig();	

		if (engine.isThinking()) {
			engine.stopThinking();

			engine.newGame();
			engine.clearHashTable(cfg.threads);

            mainModule.remainingMoves = 30;
            mainModule.maxGameMoves = 30;
            engine.setRemainingMoves(mainModule.remainingMoves);

		} else {
			engine.newGame();
			engine.clearHashTable(cfg.threads);
		}

	} else
	if (token == "stop") {
		// stop thinking
		// if we are pondering, the opponent played an unexpected move
		if (engine.isThinking()) {
			engine.stopThinking();
		}
	} else {
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolUCI::parseOption(Tokenizer &tk)
{
	if (!tk.hasMoreTokens()) {
		return;
	}

	auto token = tk.nextToken();

    if (token == "name") {

		if (!tk.hasMoreTokens()) {
			return;
		}
		auto name = tk.nextToken();

		if (!tk.hasMoreTokens()) {
			return;
		}
		auto opt = tk.nextToken();
		if (opt != "value") {
			return;
		}

		if (!tk.hasMoreTokens()) {
			return;
		}
		auto value = tk.nextToken();

		auto &cfg = mainModule.getConfig();	

		if (name == "Hash") {
			engine.setHashTableSize(ChessModule::parseInt(value), cfg.threads);
		} else
		if (name == "Ponder") {
			bool b = (value == "true");
			mainModule.ponderingActive = b;
		} else
		if (name == "NullMovePruning") {
			cfg.nullMoves = (value == "false") ? false : true;  
			engine.enableNullMoves(cfg.nullMoves);
		} else
		if (name == "LateMoveReduction") {
			cfg.lmr = (value == "false") ? false : true;
			engine.enableLMR(cfg.lmr);
		} else
		if (name == "OwnBook") {
#ifndef __EMSCRIPTEN__
			if (value == "true") {
				cfg.useBook = true;
				if (!engine.loadOpeningBook(cfg.bookFile)) {
					mainModule.sendCommand("info string book file not found");
					cfg.useBook = false;
				}
			} else {
				cfg.useBook = false;
				engine.unloadOpeningBook();
			}
#endif
		} else
		if (name == "Book") {
#ifndef __EMSCRIPTEN__
			cfg.bookFile.assign(value);
			if (cfg.useBook) {
				if (!engine.loadOpeningBook(cfg.bookFile)) {
					mainModule.sendCommand("info string book file not found");
					cfg.useBook = false;
				}
			}
#endif
		} else
		if (name == "Clear") {
			if (value == "Hash") {
				engine.clearHashTable(cfg.threads);
			}
		} else
		if (name == "MultiPV") {
			engine.setMultiPV(ChessModule::parseInt(value));
		} else
		if (name == "UCI_Elo") {		
			int ve = ChessModule::parseInt(value);
			cfg.elo = ve;
			if (cfg.useStrength) {
				if (cfg.engineLevel == max_engine_level) {
					engine.setStrengthELO(ve);
				} else {
					engine.setStrengthLevel(cfg.engineLevel);
				}
			}
		} else
		if (name == "UCI_LimitStrength") {
			if (value == "true") {
				if (cfg.engineLevel == max_engine_level) {		
					engine.setStrengthELO(cfg.elo);
				} else {
					engine.setStrengthLevel(cfg.engineLevel);
				}
				cfg.useStrength = true;
			} else {
				engine.setStrengthELO(max_strength_elo);
				cfg.useStrength = false;
			}
		} else
		if (name == "LimitStrengthMode") {
			cfg.uciLimitMode = (value == "speed") ? ELOLimitMode::speed :
 				ELOLimitMode::nodes;
			engine.setELOLimitMode(cfg.uciLimitMode);
			if (cfg.useStrength) {
				if (cfg.engineLevel == max_engine_level) {
					engine.setStrengthELO(cfg.elo);
				} else {
					engine.setStrengthLevel(cfg.engineLevel);
				}
			}
		} else
		if (name == "EngineStrength") {
			int ve = ChessModule::parseInt(value);
			cfg.engineLevel = ve;
			if (cfg.useStrength) {
				if (ve == max_engine_level) {		
					engine.setStrengthELO(cfg.elo);
				} else {
					engine.setStrengthLevel(ve);
				}
			}			
		} else
		if (name == "SyzygyPath") {
			TableBases::init(std::string(value));
		} else
		if (name == "SyzygyProbeDepth") {
			engine.setTBProbeDepth(ChessModule::parseInt(value));
		} else
		if (name == "Syzygy50MoveRule") {
			engine.setTB50MoveRule((value == "true") ? true : false);
		} else
		if (name == "SyzygyProbeLimit") {
			engine.setTBProbeLimit(ChessModule::parseInt(value));
		} else
		if (name == "DrawScore") {
			engine.setDrawValue(ChessModule::parseInt(value));
		} else
		if (name == "UCI_Chess960") {
			engine.getBoardRef().setChessRules((value == "true") ?
				ChessRules::frc : ChessRules::classic);
		} else
		if (name == "SMPMode") {
			if (value != cfg.smpMode) {
				if (value == "ybwc") {
					engine.setSMPMode(SMPMode::ybwc);
				} else
				if (value == "lazy") {
					engine.setSMPMode(SMPMode::lazy);
				} else {
					return;
				}
				cfg.smpMode = value;
				engine.setMaxThreads(cfg.threads);
				Logger::info() << "SMPMode = " << value;
			}
		} else
		if (name == "Threads") {
			int ve = ChessModule::parseInt(value);
			if (cfg.threads != ve) {
				engine.setMaxThreads(ve);
				cfg.threads = ve;
				Logger::info() << "Using " << ve << " threads";
			}
		} else
		if (name == "UsePersonality") {
#ifndef __EMSCRIPTEN__
			if (value == "true") {
				cfg.usePersonality = true;
				engine.setPersonality(cfg.personalityFile);
			} else {
				cfg.usePersonality = false;
				engine.setPersonality("");
			}
#endif
		} else
		if (name == "personalityFile") {
#ifndef __EMSCRIPTEN__
			cfg.personalityFile.assign(value);
			if (cfg.usePersonality) {
				engine.setPersonality(cfg.personalityFile);
			} else {
				engine.setPersonality("");
			}
#endif
		}
    } else {
		Logger::debug() << "unknown parse option " << token;
    }
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolUCI::parsePosition(Tokenizer &tk)
{
	if (!tk.hasMoreTokens()) {
		return;
	}

	auto token = tk.nextToken();

	if (token == "startpos") {

		// start position
		engine.setStartPosition();

		if (tk.hasMoreTokens()) {
			auto opt = tk.nextToken();
			if (opt == "moves") {
				if (tk.hasMoreTokens()) {
					if (!engine.playMovesFromString(tk.getCurrentString())) {
						Logger::error() << "bad move";
					}
				}
			}
		}

	} else
	if (token == "fen") {

		if (!tk.hasMoreTokens()) {
			return;
		}

		auto str = tk.getCurrentString();

		auto p = str.find("moves");
		if (p != std::string_view::npos) {

			auto strfen = str.substr(0, p);
			auto p2 = str.find_first_of(' ', p);
			auto p3 = str.find_first_not_of(' ', p2);
			auto strmoves = str.substr(p3);	

			if (!engine.setFENPosition(strfen)) {
				Logger::error() << "bad FEN string";
				return;
			}

			if (!engine.playMovesFromString(strmoves)) {
				Logger::error() << "bad move";
			}

		} else {
			if (!engine.setFENPosition(str)) {
				Logger::error() << "bad FEN string";
				return;
			}
		}

	} else {
		Logger::debug() << "unknown position parameter " << token;
    }
}

////////////////////////////////////////////////////////////////////////////////
// parse 'go' command
void ChessProtocolUCI::parseGo(Tokenizer &tk)
{
	mainModule.pondernextmove = false;
    engine.setRemainingMoves(0);
	engine.setPlayerTime(black, 0);
	engine.setPlayerTime(white, 0);
	engine.setTimeIncrement(white, 0);
	engine.setTimeIncrement(black, 0);
	engine.setSearchFixedTime(0);
	engine.setSearchFixedNodes(0);
	engine.setSearchFixedDepth(0);
	engine.setInfiniteSearch(false);
	engine.setSearchMateValue(0);
	engine.clearSearchExcludedMoves();

	while (tk.hasMoreTokens()) {

		auto token = tk.nextToken();

		if (token == "ponder") {
			mainModule.pondernextmove = true;
		} else
        if (token == "wtime") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setPlayerTime(white, ChessModule::parseInt(value));
			}
        } else
        if (token == "btime") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setPlayerTime(black, ChessModule::parseInt(value));
			}
        } else
        if (token == "winc") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setTimeIncrement(white,
					ChessModule::parseInt(value));
			}
        } else
        if (token == "binc") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setTimeIncrement(black, 
					ChessModule::parseInt(value));
			}
        } else
		if (token == "movetime") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setSearchFixedTime(ChessModule::parseInt(value));
			}
        } else
        if (token == "movestogo") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setRemainingMoves(ChessModule::parseInt(value));
			}
        } else
		if (token == "infinite") {
			engine.setInfiniteSearch(true);
        } else
		if (token == "depth") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setSearchFixedDepth(ChessModule::parseInt(value));
			}
        } else
        if (token == "nodes") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setSearchFixedNodes(ChessModule::parseInt(value));
			}
		} else
		if (token == "searchmoves") {
			if (tk.hasMoreTokens()) {
				engine.setSearchSearchedMovesFromString(tk.getCurrentString());
			}
      } else
      if (token == "mate") {
			if (tk.hasMoreTokens()) {
				auto value = tk.nextToken();
				engine.setSearchMateValue(ChessModule::parseInt(value));
			}
        } else {
			Logger::debug() << "unknown go parameter " << token;
		}
    }
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolUCI::onEndSearch(ChessMove bm,
	ChessMove pm, [[maybe_unused]] int depth, [[maybe_unused]] int ply,
	[[maybe_unused]] std::uint64_t cntnodes, 
	[[maybe_unused]] std::uint64_t searchtime)
{
	// don't need to play moves with UCI ?
	if (mainModule.pondernextmove) {
		engine.takeBack();
	} else {
		if (!engine.isSearchInfinite()) {
			engine.playMove(bm);
		}
	}

	// pondering canceled (stopped search or disable pondering)
	if (mainModule.pondernextmove) {

		if (mainModule.ponderingActive) {
			std::string str = "bestmove " + engine.getTextMove(bm);
			if (!pm.isEmpty()) {
				str += " ponder " + engine.getTextMove(pm);
			}
			mainModule.sendCommand(str);
		} else {
			mainModule.pondernextmove = false;
		}

	} else {

		mainModule.movePonder = pm;

		std::string str = "bestmove " + engine.getTextMove(bm);
		if ((mainModule.ponderingActive) && (!pm.isEmpty())) {
			str += " ponder " + engine.getTextMove(pm);
		}
		mainModule.sendCommand(str);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolUCI::onSendMultiPV(int num, int depth, int score,
	const ChessPVMoveList &mlist, int count, std::uint64_t cntnodes, 
	std::uint64_t searchtime, int maxply, int bound,
	[[maybe_unused]] std::uint64_t tbhits, std::uint64_t hashfull)
{
	std::uint64_t nps = (cntnodes * 1000) / (searchtime + 1);

	int sc = score;
	int im = 0;

	if (score <= min_score_mate) {
		im = 1;
		sc = -ChessEngine::calcMateValue(score);
	} else
	if (score >= max_score_mate) {
		im = 1;
		sc = ChessEngine::calcMateValue(score);
	}

	std::string str = "info";

	if (num != 0) {
		str += " multipv " + std::to_string(num);
	}

	str += " depth " + std::to_string(depth) +
		   " seldepth " + std::to_string(maxply) +
		   " score " + strMate[im] + ' ' + std::to_string(sc) +
		   ' ' + strBound[bound] +
		   "time " + std::to_string(searchtime) +
		   " nodes " + std::to_string(cntnodes) +
		   " nps " + std::to_string(nps) +
		   " tbhits " + std::to_string(tbhits) +
		   " hashfull " + std::to_string(hashfull) +
		   " pv " + engine.getTextPV(mlist, count);

	mainModule.sendCommand(str);
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolUCI::onSearchRootMove(int depth, ChessMove move, int num)
{
	mainModule.sendCommand("info depth " + std::to_string(depth) + 
		" currmove " + engine.getTextMove(move) + " currmovenumber " + 			
		std::to_string(num));
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolUCI::onSendInfoMessage(const std::string &msg)
{
	mainModule.sendCommand("info string " + msg);
}

} // namespace Cheese
