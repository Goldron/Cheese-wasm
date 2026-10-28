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

#include "tools.h"

#include "board.h"
#include "fen.h"
#include "hash.h"
#include "moveparser.h"
#include "util/logfile.h"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Cheese {

const std::array<char, 3> charBound = { ' ', '-', '+' };

////////////////////////////////////////////////////////////////////////////////
// format number to string with thousands sperators
std::string ChessTools::formatNumber(std::uint64_t v)
{
	if (v < 1000) {
		return std::to_string(v);
	} else {
		std::string str;
		std::uint64_t n = v % 1000;
		if (n < 100) {
			str += '0';
		}
		if (n < 10) {
			str += '0';
		}
		str += std::to_string(n);
		return formatNumber(v / 1000) + ' ' + str;
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessTools::onEndSearch([[maybe_unused]] ChessEngineNotifier &notifier,
	ChessMove bm, ChessMove pm, int depth, int ply, std::uint64_t cntnodes, 
	std::uint64_t searchtime)
{
	// save search result
	searchResult.bestMove = bm;
	searchResult.ponderMove = pm;
	searchResult.maxDepth = depth;
	searchResult.maxPly = ply;
	searchResult.countNodes = cntnodes;
	searchResult.searchTime = searchtime;
}

////////////////////////////////////////////////////////////////////////////////
void ChessTools::onSendMultiPV([[maybe_unused]] ChessEngineNotifier &notifier,
	int num, int depth, int score, const ChessPVMoveList &mlist, int count,
	std::uint64_t cntnodes, std::uint64_t searchtime, int maxply, int bound,
	[[maybe_unused]] std::uint64_t tbhits, 
	[[maybe_unused]] std::uint64_t hashfull)
{
	if (muted) {
		return;
	}

	ChessBoard pos(engine.getBoardRef());

	std::string strpv;
	for (int l=0; l<count; l++) {
		std::string strmove = ChessMoveParser::move_to_string_san(mlist[l], 
			pos);
		pos.makeMove(mlist[l]);
		if (!pos.isPositionLegal()) {
			Logger::warning() << "Illegal move in pv : " 
				<< ChessMoveParser::move_to_string_lan(mlist[l]);
			break;
		} else {
			strpv += strmove;
			if (pos.inCheck()) {
				if (engine.countLegalMoves(pos) == 0) {
					strpv += '#';
				} else {
					strpv += '+';
				}
			}
			strpv += ' ';
		}
	}

	std::uint64_t nps = (cntnodes * 1000) / (searchtime + 1);

	std::stringstream ss;

	if (num <= 1) {
		std::stringstream ss2;
		ss2  << depth << '/' << maxply << charBound[bound];
		ss << std::setfill(' ') << std::setw(8)
		   << ss2.str() << ' ';
	} else {
		ss << "         ";
	}

	// TODO : imbue for numbers thousand separator

	ss << Timer::timeToString(searchtime) << ' '
	   << std::setfill(' ') << std::setw(16)
	   << ChessTools::formatNumber(cntnodes) << ' '
	   << std::setfill(' ') << std::setw(14)
	   << ChessTools::formatNumber(nps) << ' ';

	std::stringstream ss3;

	if (score <= min_score_mate) {
		ss3 << "-M" << ChessEngine::calcMateValue(score);
	} else
	if (score >= max_score_mate) {
		ss3 << "+M" << ChessEngine::calcMateValue(score);
	} else {
		if (score >= 0) {
			ss3 << '+';
		}
		ss3 << std::fixed << std::setprecision(2)
		    << static_cast<double>(score) * 0.01;
	}

	ss << std::setfill(' ') << std::setw(8)
	   << ss3.str();
	ss << "  " << strpv;

	std::cout << ss.str() << std::endl;

	Logger::info() << ss.str();
}

////////////////////////////////////////////////////////////////////////////////
ChessTools::ChessTools(ChessEngine &e, ChessBoard &b) : engine(e), board(b)
{
	engine.registerListener(this);
}

////////////////////////////////////////////////////////////////////////////////
ChessTools::~ChessTools()
{
	engine.unregisterListener(this);
}

////////////////////////////////////////////////////////////////////////////////
std::uint64_t ChessTools::searchAllMoves(int depth)
{
	ChessMoveSortList mlist;
	ChessMoveGen::generateMoves(board, mlist, board.inCheck());

	std::uint64_t nb = 0ULL;

	if (depth == 1) {
		for (const auto &m : mlist) {
			const ChessMoveRec rec = board.makeMove(m.move);
			if (board.isPositionLegal()) {
				++nb;
			}
			board.unMakeMove(m.move, rec);
		}
	} else {
		for (const auto &m : mlist) {
			const ChessMoveRec rec = board.makeMove(m.move);
			if (board.isPositionLegal()) {
				nb += searchAllMoves(depth - 1);
			}
			board.unMakeMove(m.move, rec);
		}
	}

	return nb;
}

//////////////////////////////////////////////////////////////////////////////////
void ChessTools::divide(int depth)
{
	ChessMoveSortList mlist;
	ChessMoveGen::generateMoves(board, mlist, board.inCheck());

	nbDivide = mlist.size();

	int i = 0;

	for (const auto &m : mlist) {

		std::string strmove = 
			ChessMoveParser::move_to_string_san(m.move, board);

		ChessMoveRec rec = board.makeMove(m.move);
		if (board.isPositionLegal()) {

			if (depth > 1) {
				listDivide[i].count = searchAllMoves(depth - 1);
			} else {
				listDivide[i].count++;
			}

			listDivide[i].name = strmove;
			++i;
		}

		board.unMakeMove(m.move, rec);
	}

	nbDivide = i;
}

////////////////////////////////////////////////////////////////////////////////
void ChessTools::perftd(int depth)
{
	ChessMoveSortList mlist;
	ChessMoveGen::generateMoves(board, mlist, board.inCheck());

	for (const auto &m : mlist) {

		// make move (makemove return 0 if we are in check after moving)
		const ChessMoveRec rec = board.makeMove(m.move);
		if (board.isPositionLegal()) {

			if (depth == 1) {

				++countnode;

				// captures
				if (m.move.isCapture()) {
					++countcapture;
				}

				// en passant
				if (m.move.isEnPassant()) {
					++countep;
				}

				// promotion
				if (m.move.isPromotion()) {
					++countpromotion;
				}

				// castling
				if (m.move.isCastle()) {
					++countcastle;
				}

				// check
				bool incheck = board.inCheck();
				if (incheck) {
					++countcheck;
					if (ChessMoveGen::countLegalMoves(board) == 0) {
						++countmate;
					}
				}

			} else {
				// next ply
				perftd(depth - 1);
			}
		}

		// unmake move
		board.unMakeMove(m.move, rec);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessTools::perft(int depth)
{
	ChessMoveSortList mlist;
	ChessMoveGen::generateMoves(board, mlist, board.inCheck());

	if (depth == 1) {
		for (const auto &m : mlist) {
			const ChessMoveRec rec = board.makeMove(m.move);
			if (board.isPositionLegal()) {
				++countnode;
			}
			board.unMakeMove(m.move, rec);
		}
	} else {
		for (const auto &m : mlist) {
			const ChessMoveRec rec = board.makeMove(m.move);
			if (board.isPositionLegal()) {
				perft(depth - 1);
			}
			board.unMakeMove(m.move, rec);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessTools::doPerft(int maxdepth)
{
	countnode = 0;
	perft(maxdepth);
}

////////////////////////////////////////////////////////////////////////////////
bool findMoveInString(const std::string &str, const std::string &move)
{
	std::istringstream stream(str);
	std::string token;
	while (std::getline(stream, token, ' ')) {
		if (token == move) {
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
// run a test suite, from a .EPD file
bool ChessTools::runTestSuite(const std::string &filename)
{
	std::vector<std::string> fens;

	std::uint64_t totalNodes = 0;
	std::uint64_t timesearch = 0;
	int score = 0;
	int	count = 0;
	int dmoy = 0;
	int dmin = 1024;
	int dmax = 0;

	std::cout << "Cheese version " << std::string(Cheese::project_ver) 
		<< std::endl << "Test suite : " << filename << std::endl;

	ChessMoveSortList mvlist;

	std::vector<std::string> listbadresult;

	if (!ChessFENParser::load(filename, fens)) {
		Logger::error() << "file not found : " << filename;
		return false;
	}

	engine.setModeRunTestSuite(true);

	for (auto const &strfen : fens) {

		Logger::info() << strfen;

		std::string strbm = ChessFENParser::getOption(strfen, "bm");
		std::string stram = ChessFENParser::getOption(strfen, "am");

		for (char c : std::string("+#!?")) {
			strbm.erase(std::remove(strbm.begin(), strbm.end(), c),
				strbm.end());
			stram.erase(std::remove(stram.begin(), stram.end(), c),
				stram.end());
		}
		
		std::cout << strfen << '\n' << std::endl;

		if ((!strbm.empty()) || (!stram.empty())) {

			engine.clearHashTable();
			if (!engine.setFENPosition(strfen)) {
				Logger::warning() << "bad FEN : " << strfen;
			} else {

				mvlist.clear();
				ChessMoveGen::generateMoves(board, mvlist, board.inCheck());
				if (!mvlist.empty()) {

					searchResult.bestMove.clear();

					// start search
					engine.startSearch(false);
					engine.waitEndSearch();

					timesearch += searchResult.searchTime;

					if (!searchResult.bestMove.isEmpty()) {

						totalNodes += searchResult.countNodes;
						ChessMove move = searchResult.bestMove;

						std::string txtsan = 
							ChessMoveParser::move_to_string_san(move, board);

						std::string mtxt =
							ChessMoveParser::move_to_string_lan(move);

						bool tst;
						if (!stram.empty()) {
							tst = (!findMoveInString(stram, mtxt) &&
							       !findMoveInString(stram, txtsan));
						} else {
							tst = (findMoveInString(strbm, mtxt) ||
							       findMoveInString(strbm, txtsan));
						}

						if (tst) {
							std::cout << "\nresult => '" << txtsan
									  << "' : ok\n" << std::endl;
							++score;
						} else {
							std::cout << "\nresult => '" << txtsan
									  << "' : bad\n" << std::endl;
							listbadresult.push_back(strfen);
						}

						int sd = searchResult.maxDepth;
						dmoy += sd;
						dmin = std::min(sd, dmin);
						dmax = std::max(sd, dmax);

						++count;

					} else {
						Logger::warning() << "no moves found : " << strfen;
					}

				} else {
					Logger::warning() << "no possible moves : " << strfen;
				}
			}

		} else {
			Logger::warning() << "no bm or am in fen : " << strfen;
		}
	}

	std::uint64_t nps = (totalNodes * 1000) / (timesearch + 1);

	dmoy = (dmoy != 0) ? (dmoy / count) : dmin;

	std::cout << "Time : " << Timer::timeToString(timesearch) << '\n'
			  << "Total nodes : " << formatNumber(totalNodes) << '\n'
			  << "Speed : " << formatNumber(nps) << " nodes/s\n"
			  << "Depth : min = " << dmin << ", max = " << dmax
			  << ", average = " << dmoy << '\n'
			  << "Score : " << score << " / " << count << '\n'
			  << "\nBad results : \n" << std::endl;

	for (auto const &strbad : listbadresult) {
		std::cout << strbad << std::endl;
	}

	engine.setModeRunTestSuite(false);

	return true;
}

////////////////////////////////////////////////////////////////////////////////
void ChessTools::runBenchmark(const std::vector<std::string> &fens, int mode)
{
	muted = true;
	engine.setModeRunTestSuite(true);

	std::uint64_t countNodes = 0;
	std::uint64_t timesearch = 0;
	int dmin = 128;
	int dmax = 1;
	int dmoy = 0;

	for (auto n=0U; n<fens.size(); n++) {

		std::cout << "Position " << (n + 1) << " / " << fens.size() 
				  << std::endl;

		if (engine.setFENPosition(fens[n])) {

			engine.clearHashTable();
			engine.startSearch(false);
			engine.waitEndSearch();

			countNodes += searchResult.countNodes;
			timesearch += searchResult.searchTime;
			int sd = searchResult.maxDepth;
			dmoy += sd;
			dmin = std::min(sd, dmin);
			dmax = std::max(sd, dmax);

		} else {
			Logger::warning() << "bad FEN : " << fens[n];
		}
	}

	dmoy = (fens.empty()) ? dmin : (dmoy / static_cast<int>(fens.size()));
	std::uint64_t nps = (countNodes * 1000) / (timesearch + 1);

	std::cout << "\nTime : "
			  << std::fixed << std::setprecision(2) 
			  << (static_cast<double>(timesearch) / 1000.0)
			  << " s (" << Timer::timeToString(timesearch) << ")" << '\n'
			  << "Nodes : " << formatNumber(countNodes) << '\n'
			  << "Average speed : " << formatNumber(nps) << " nodes/s";

	if (mode == 1) {
		std:: cout << "\nAverage depth = " << dmoy << ", min = " << dmin
				   << ", max = " << dmax;
	}
	std::cout << std::endl;

	engine.setModeRunTestSuite(false);
	muted = false;
}

////////////////////////////////////////////////////////////////////////////////
void ChessTools::analyze(const std::string &fen, 
	const std::vector<std::string> &excludemoves)
{
	if (!engine.setFENPosition(fen)) {
		Logger::warning() << "bad FEN : " << fen;
		return;
	}

	ChessMoveList mlist;
	ChessMove move;
	for (const auto &mv : excludemoves) {
		if (ChessMoveParser::move_from_string_san(mv, board, move)) {
			mlist.push(move);
		} else {
			Logger::warning() << "bad excluded move : " << mv;
		}
	}

	engine.setSearchExcludedMoves(mlist);

	muted = false;
	engine.setModeRunTestSuite(true);

	engine.clearHashTable();

	engine.startSearch(false);
	engine.waitEndSearch();
	
	engine.setModeRunTestSuite(false);
}

} // namespace Cheese
