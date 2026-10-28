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

#include "protocolcecp.h"

#include "util/logfile.h"
#include "board.h"
#include "engine.h"
#include "module.h"
#include "moveparser.h"
#include "tablebases.h"

#include <iomanip>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
ChessProtocolCECP::ChessProtocolCECP(ChessEngine &e, ChessModule &m) :
	ChessProtocol(e, m)
{
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolCECP::init()
{
	mainModule.remainingMoves = 30;
	mainModule.maxGameMoves = 30;
	countHalfMove = 0;
	firstThink = false;
	forcemode = false;

	mainModule.initEngine();
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolCECP::initFeatures()
{
	mainModule.sendCommand("feature myname=\"" +
					   ChessModule::name_program + ' ' +
					   std::string(Cheese::project_ver));

	mainModule.sendCommand("feature setboard=1");
	mainModule.sendCommand("feature analyze=1");
	mainModule.sendCommand("feature ping=1");
	mainModule.sendCommand("feature draw=0");
	mainModule.sendCommand("feature sigint=0");
	mainModule.sendCommand("feature sigterm=0");
	mainModule.sendCommand("feature colors=0");
	mainModule.sendCommand("feature smp=1");

	// supported chess variants
	mainModule.sendCommand("feature variants=\"normal,fischerandom\"");

	// gui options
	mainModule.sendCommand("feature option=\"Clear Hash -button\"");

	mainModule.sendCommand("feature done=1");
}

////////////////////////////////////////////////////////////////////////////////
bool ChessProtocolCECP::parse(const std::string &str)
{
	Tokenizer tk(str);

	if (!tk.hasMoreTokens()) {
		return true;
	}

	auto token = tk.nextToken();

	// move
	if (((token.size() == 4) || (token.size() == 5)) &&
		ChessMoveParser::is_char_file(token[0]) &&
		ChessMoveParser::is_char_rank(token[1]) &&
		ChessMoveParser::is_char_file(token[2]) &&
		ChessMoveParser::is_char_rank(token[3])) {

		parseMove(token);

	} else
	if (token == "time") {

		if (tk.hasMoreTokens()) {

			auto value = tk.nextToken();

            // we are 1-player, because we have not played the opponent move
			engine.setPlayerTime(~engine.side(), 
				ChessModule::parseInt(value) * 10);
		}

	} else
	if (token == "otim") {

		if (tk.hasMoreTokens()) {

			auto value = tk.nextToken();

			engine.setPlayerTime(engine.side(), 
				ChessModule::parseInt(value) * 10);
		}

	} else
	if (token == "go") {

        // first think
        if (!firstThink) {

            firstThink = true;

            // if number of moves is fixed : count moves already played in
			// the opening book
            if (mainModule.remainingMoves != 0) {
                mainModule.remainingMoves -= (countHalfMove / 2);
                engine.setRemainingMoves(mainModule.remainingMoves);
            }
        }

        if (forcemode) {
            forcemode = false;
        }

		mainModule.startSearch(false);

	} else
	if (token == "quit") {
		mainModule.exit();
	} else
	if (token == "undo") {
		engine.takeBack();
	} else
	if (token == "force") {
		forcemode = true;
	} else
	if (token == "result") {
	    if (engine.isThinking()) {
			engine.stopThinking();
		}
	} else
	if (token == "analyze") {
		engine.setInfiniteSearch(true);
		mainModule.startSearch(false);
	}
	else
	if (token == "exit") {
	    // stop thinking (if we are pondering, the player played the wrong move)
		if (engine.isThinking()) {
			engine.stopThinking();
		}
	} else
	if (token == "setboard") {
		if (tk.hasMoreTokens()) {

			auto strfen = tk.getCurrentString();

			// warning : if sent in game, look at current game move count
			// (used by chessgui for book moves...)
            if (!engine.setFENPosition(strfen)) {
				Logger::error() << "bad FEN string";
			}
		}
	} else
	if (token == "?") {

		// stop thinking (if we are pondering, the player played the wrong move)
		if (engine.isThinking()) {
			engine.stopThinking();
		}

	} else
	if (token == "new") {

		mainModule.ponderingActive = false;
		mainModule.movePonder.clear();

		if (engine.isThinking()) {
			engine.stopThinking();
		}
		auto &cfg = mainModule.getConfig();	
		engine.newGame();
		engine.clearHashTable(cfg.threads);
        mainModule.remainingMoves = 30;
        mainModule.maxGameMoves = 30;
        countHalfMove = 0;
        firstThink = false;
        engine.setRemainingMoves(mainModule.remainingMoves);

		forcemode = false;

	} else
	if (token == "easy") {

		// stop current search
        if (engine.isThinking()) {
            engine.stopThinking();
        }

        // turn off pondering
		mainModule.ponderingActive = false;
		mainModule.movePonder.clear();

	} else
	if (token == "hard") {
		// turn on pondering
		mainModule.ponderingActive = true;
	} else
	if (token == "post") {
		// turn on pondering output
		showthinking = true;
	} else
	if (token == "nopost") {
		// turn off pondering output
		showthinking = false;
	} else
	if (token == "protover") {

		if (tk.hasMoreTokens()) {

			auto value = tk.nextToken();

			protocolversion = ChessModule::parseInt(value);
			// send features commands
			initFeatures();
		}

	} else
	if (token == "ping") {

		if (tk.hasMoreTokens()) {

			auto value = tk.nextToken();

			mainModule.sendCommand("pong " + std::string(value));
		}
	} else
	if (token == "st") {

		if (tk.hasMoreTokens()) {

			auto value = tk.nextToken();

			engine.setSearchFixedTime(ChessModule::parseInt(value) * 1000);
			engine.setSearchFixedDepth(0);
		}

	} else
	if (token == "sd") {

		if (tk.hasMoreTokens()) {

			auto value = tk.nextToken();

			engine.setSearchFixedDepth(ChessModule::parseInt(value));
			engine.setSearchFixedTime(0);
		}

	} else
	if (token == "level") {

		if (tk.hasMoreTokens()) {

			auto v1 = tk.nextToken();

			int p1 = ChessModule::parseInt(v1);
			int p2 = 0;
			int p20 = 0;
			int p21 = 0;
			double p3 = 0.0;

			if (tk.hasMoreTokens()) {

				auto v2 = tk.nextToken();
				auto p = v2.find_first_of(':');
				if (p != std::string::npos) {
					p20 = ChessModule::parseInt(v2.substr(0, p));
					p21 = ChessModule::parseInt(v2.substr(p + 1));
				} else {
					p20 = ChessModule::parseInt(v2);
				}

				if (tk.hasMoreTokens()) {
					auto v3 = tk.nextToken();
					p3 = ChessModule::parseDouble(v3);
				}
			}

			mainModule.maxGameMoves = p1;
			mainModule.remainingMoves = p1;
            engine.setRemainingMoves(p1);

			p2 = p20 * 60 + p21;

            engine.setPlayerTime(engine.side(), p2 * 1000);
            engine.setTimeIncrement(white, static_cast<int>(p3 * 1000));
            engine.setTimeIncrement(black, static_cast<int>(p3 * 1000));
			engine.setSearchFixedTime(0);
			engine.setSearchFixedDepth(0);	
		}

	} else
	if (token == "option") {
		parseOption(tk);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolCECP::parseOption(Tokenizer &tk)
{
	if (!tk.hasMoreTokens()) {
		return;
	}

	auto token = tk.nextToken();

	if (token == "Clear") {

		if (!tk.hasMoreTokens()) {
			return;
		}

		auto opt = tk.nextToken();

		if (opt == "Hash") {
			auto &cfg = mainModule.getConfig();	
			engine.clearHashTable(cfg.threads);
		}

	} else {
		Logger::debug() << "unknown option : " << token;
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolCECP::parseMove(const std::string_view &command)
{
	++countHalfMove;

	if ((mainModule.ponderingActive) &&
		(mainModule.pondernextmove) &&
		(!mainModule.movePonder.isEmpty())) {

		File fs = ChessMoveParser::file_from_char(command[0]);
		Rank rs = ChessMoveParser::rank_from_char(command[1]);
		File fd = ChessMoveParser::file_from_char(command[2]);
		Rank rd = ChessMoveParser::rank_from_char(command[3]);

		// same move : play
		if ((fs == squareFile(mainModule.movePonder.src())) &&
			(rs == squareRank(mainModule.movePonder.src())) &&
			(fd == squareFile(mainModule.movePonder.dst())) &&
			(rd == squareRank(mainModule.movePonder.dst()))) {

			if (engine.isThinking()) {
				mainModule.pondernextmove = false;
				engine.ponderHit();
			} else {
				if (!engine.playMovesFromString(command)) {
					Logger::error() << "bad move";
					return;
				}
				mainModule.pondernextmove = true;
				mainModule.startSearch(false);
			}
			return;

		} else {

			pondernewmove = command;

			if (engine.isThinking()) {
				engine.stopThinking();
			} else {
				if (!engine.playMovesFromString(command)) {
					Logger::error() << "bad move";
					return;
				}
				mainModule.pondernextmove = false;
				mainModule.startSearch(false);
			}
			return;
		}

	} else {
		// play move
		if (!engine.playMovesFromString(command)) {
			Logger::error() << "bad move";
			return;
		}
	}

	// return if no moves possibles (mate)
	if (engine.countLegalMoves(engine.getBoardRef()) == 0) {
		return;
	}

	// computer must play
	if (!forcemode) {
		engine.setRemainingMoves(mainModule.remainingMoves);
		mainModule.startSearch(false);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolCECP::onEndSearch(ChessMove bm,
	ChessMove pm, [[maybe_unused]] int depth, [[maybe_unused]] int ply,
	[[maybe_unused]] std::uint64_t cntnodes, 
	[[maybe_unused]] std::uint64_t searchtime)
{
	if (mainModule.pondernextmove) {
		// take back if we were pondering
		engine.takeBack();
	} else {
		// play the move (if not in analyze mode)
		if ((!engine.isSearchInfinite()) &&
			(!bm.isEmpty())) {
			engine.playMove(bm);
		}
	}

	// pondering canceled (stopped search or disable pondering)
	if (mainModule.pondernextmove) {

		mainModule.pondernextmove = false;

		if (mainModule.ponderingActive) {

			// play the move
			if (!engine.playMovesFromString(pondernewmove)) {
				Logger::error() << "bad move";
				return;
			}

			// restart search if opponent played the wrong move
			mainModule.startSearch(false);
		}

	} else {

		if (!engine.isRunningTestSuite()) {

			mainModule.movePonder = pm;

			// update number of moves
			if (mainModule.maxGameMoves != 0) {
				--mainModule.remainingMoves;
				if (mainModule.remainingMoves == 0) {
					mainModule.remainingMoves += mainModule.maxGameMoves;
				}
			}

			mainModule.sendCommand("move " + engine.getTextMove(bm));

			if (mainModule.ponderingActive) {

				mainModule.pondernextmove = false;

				if (!mainModule.movePonder.isEmpty()) {

					std::string strmove = 
						engine.getTextMove(mainModule.movePonder);
					mainModule.sendCommand("Hint: " + strmove);

					if (engine.playMovesFromString(strmove)) {
						mainModule.pondernextmove = true;
						mainModule.startSearch(true);
					} else {
						Logger::error() << "bad move";
					}
				}
			}
        }
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolCECP::onSendMultiPV([[maybe_unused]] int num, int depth,
	int score, const ChessPVMoveList &mlist, int count, std::uint64_t cntnodes,
	std::uint64_t searchtime, [[maybe_unused]] int maxply,
	[[maybe_unused]] int bound, [[maybe_unused]] std::uint64_t tbhits,
	[[maybe_unused]] std::uint64_t hashfull)
{
	if (showthinking) {
		mainModule.sendCommand(std::to_string(depth) + ' ' +
		   std::to_string(score) + ' ' +
		   std::to_string(static_cast<int>(searchtime / 10)) +
		   ' ' + std::to_string(cntnodes) + ' ' +
		   engine.getTextPV(mlist, count));
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessProtocolCECP::onSendInfoMessage(const std::string &msg)
{
	mainModule.sendCommand(msg);
}

} // namespace Cheese
