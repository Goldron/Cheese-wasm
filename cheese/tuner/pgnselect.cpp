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

#include "pgnselect.h"

#include "moveparser.h"

#include <iostream>

////////////////////////////////////////////////////////////////////////////////
PGNSelect::PGNSelect() : ChessPGN::PGNLoader()
{
	std::random_device rd;	
	std::seed_seq sd { rd(), rd(), rd(), rd() };
	mtrnd = std::mt19937_64(sd);
}

////////////////////////////////////////////////////////////////////////////////
void PGNSelect::onStartGame([[maybe_unused]] ChessPGN::PGNParser &parser,
	std::size_t id)
{ 
	gid = id;
	++countgames;
	ChessBoard &board = engine.getBoardRef();

	positions.clear();

	if (startfen.empty()) {
		board.setStartPosition();
	} else {
		bool r = ChessFENParser::set(startfen, board);
		if (!r) {
			Logger::info() << "bad start position : " << startfen;
		}
	}

	if (gameresult.empty() || (gameresult == "*")) {
		skipgame = true;
	}

	board.setPly(0);
	if (!skipgame) {
		if (opt_minply == 0) {
			positions.emplace_back(ChessFENParser::get(board));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void PGNSelect::onEndGame([[maybe_unused]] ChessPGN::PGNParser &parser,
	[[maybe_unused]] std::size_t id)
{
	startfen = "";
	
	if (!skipgame) {

		if (opt_samples == 0) {
			for (const auto &fen : positions) {
				ofs << fen 
					<< " c9 " << '"' << gameresult << '"' << ";\n";
				++countpos;
			}
		} else {
			std::uniform_int_distribution<std::size_t> dist(0, 
				positions.size() - 1);

			sampled.clear();
			
			const std::size_t dmin = 4;
			std::size_t nb = std::min(opt_samples, positions.size() / dmin);

			for (std::size_t n=0U; n<nb; ++n) {
				
				// try to limit distance
				const std::size_t maxretry = 8;
				std::size_t index = 0;
				bool ok = false;
				std::size_t ret = 0;
				while ((!ok) && (ret < maxretry)) {
					index = dist(mtrnd);
					ok = true;
					for (const auto &s : sampled) {
						auto d = (s > index) ? (s - index) : (index - s);
						if (d < dmin) {
							ok = false;
							++ret;
							break;
						}
					}
				}

				if (ok) {
					sampled.push_back(index);
				}
			}

			for (const auto &s : sampled) {
				ofs << positions[s] 
					<< " c9 " << '"' << gameresult << '"' << ";\n";
				++countpos;
			}
		}

		++count;
	}
	skipgame = false;
}

////////////////////////////////////////////////////////////////////////////////
void PGNSelect::onParseDeclaration([[maybe_unused]] ChessPGN::PGNParser &parser,
	const std::string_view &name,
	const std::string_view &value)
{ 
	if (name == "Result") {
		gameresult.assign(value);
	}
	if (name == "PlyCount") {
		plycount = std::stoi(std::string(value));
		if (plycount < opt_mingameply) {
			skipgame = true;
		}
	}
	if (name == "FEN") {
		startfen = std::string(value);
	}
}

////////////////////////////////////////////////////////////////////////////////
void PGNSelect::onParseGameResult([[maybe_unused]] ChessPGN::PGNParser &parser,
	[[maybe_unused]] const std::string_view &result)
{
}

////////////////////////////////////////////////////////////////////////////////
void PGNSelect::logErrorMove(ChessPGN::PGNParser &parser, 
	const std::string_view &move)
{
	ChessBoard &board = engine.getBoardRef();
	auto mc = (parser.getMoveCount() / 2) + 1;
	auto pc = parser.getMoveCount() + 1;
	Logger::info() << "illegal position : " 
		<< mc
		<< ((pc & 1) ? ". " : "..")
		<< move;
	Logger::info() << "Game " << count + 1;
	Logger::info() << "Line " << parser.getTokenLine()
		<< ", Column " << parser.getTokenColumn();
	Logger::info() << board;
	Logger::info() << ChessFENParser::get(board);
}

////////////////////////////////////////////////////////////////////////////////
void PGNSelect::onParseMove([[maybe_unused]] ChessPGN::PGNParser &parser,
	const std::string_view &move)
{
	ChessBoard &board = engine.getBoardRef();

	if (!skipgame) {

		ChessMove mv;
		bool r = ChessMoveParser::move_from_string_san(move, board, mv);
		if (r) {
			if (board.validMove(mv)) {

				board.makeMove(mv);
				if (!board.isPositionLegal()) {
					parser.cancelParsing();
					positions.clear();
					logErrorMove(parser, move);
				}

				const int minpieces = 5;
				int nbp = BitBoards::bitCount(board.bitboardAll());
				if (nbp > minpieces) {

					if ((opt_maxscore == 0) ||
						(std::abs(engine.evaluate(-eval_infinity, 
						eval_infinity)) <= opt_maxscore)) {

						if ((!opt_quiet) ||
							(ChessEngine::isPositionQuiet(board))) {

							auto ply = static_cast<int>(parser.getMoveCount());
							board.setPly(ply);

							if ((ply >= opt_minply) &&
								((opt_maxply == 0) || (ply <= opt_maxply))) {
								positions.emplace_back(
									ChessFENParser::get(board));
							}
						}
					}
				}

			} else {
				parser.cancelParsing();
				positions.clear();
				logErrorMove(parser, move);
			}
		} else {
			parser.cancelParsing();
			positions.clear();
			logErrorMove(parser, move);
		}
	}
}
	
////////////////////////////////////////////////////////////////////////////////	
void PGNSelect::onError([[maybe_unused]] ChessPGN::PGNParser &parser,
	const std::string &str, std::size_t line, std::size_t col, char c)
{ 
	Logger::error() << str 
		<< ", line " << line
		<< ", column " << col 
		<< ", char = " << c;
}

////////////////////////////////////////////////////////////////////////////////
void PGNSelect::onLog([[maybe_unused]] ChessPGN::PGNParser &parser,
	const std::string &str, std::size_t line, std::size_t col,
	ChessPGN::ParseMode mode)
{
	Logger::info() << str 
		<< ", line " << line
		<< ", column " << col 
		<< ", parsemode = " 
		<< ChessPGN::str_parse_mode[static_cast<std::size_t>(mode)];
}

////////////////////////////////////////////////////////////////////////////////
bool PGNSelect::selectFromFile(const std::string &filename,
	const std::string &fileout, int minply, int maxply, 
	std::size_t samples, std::size_t mingameply, int maxscore, 
	bool quiet)
{
	opt_minply = minply;
	opt_maxply = maxply;
	opt_samples = samples;
	opt_mingameply = mingameply;
	opt_maxscore = maxscore;
	opt_quiet = quiet;

	countgames = 0;
	count = 0;
	countpos = 0;

	engine.init(1);	

	ofs.open(fileout, std::ofstream::out);

	Logger::info() << "indexing";

	if (!loadIndex(filename, pgnindex)) {
		return false;
	}

	Logger::info() << "loading";

	sampled.reserve(opt_samples);

	if (!loadFromIndex(filename, pgnindex, 0, pgnindex.size())) {
		return false;
	}

	Logger::info() << "end";
	std::cout << count << " games parsed" << std::endl;
	if (count != countgames) {
			std::cout << (countgames - count) 
				<< " games with error" << std::endl;
	}
	std::cout << countpos << " positions parsed" << std::endl;
	
	return true;
}

