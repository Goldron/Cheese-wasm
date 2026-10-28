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

#ifndef CHEESE_PGNSELECT_H_
#define CHEESE_PGNSELECT_H_

#include "board.h"
#include "engine.h"
#include "fen.h"
#include "move.h"

#include "pgn/pgn.h"

#include <string>
#include <vector>

using namespace Cheese;

////////////////////////////////////////////////////////////////////////////////
class PGNSelect : public ChessPGN::PGNLoader {

	private:

		ChessEngine engine;

		std::vector<std::string> positions;

		std::vector<ChessPGN::PGNIndex> pgnindex;

		std::vector<std::size_t> sampled;

		std::string gameresult;

		std::string startfen;

		std::ofstream ofs;

		std::mt19937_64 mtrnd;

		std::size_t gid {0};

		std::size_t count {0};

		std::size_t countgames {0};

		std::size_t countpos {0};

		std::size_t plycount {0};

		std::size_t opt_samples {0}; 

		std::size_t opt_mingameply {0};

		int opt_minply {0};

		int opt_maxply {0}; 

		int opt_maxscore {0};

		bool skipgame {false};

		bool opt_quiet {false};

	public:

		PGNSelect();

		void onStartGame([[maybe_unused]] ChessPGN::PGNParser &parser,
			[[maybe_unused]] std::size_t id) override;

		void onEndGame([[maybe_unused]] ChessPGN::PGNParser &parser,
			[[maybe_unused]] std::size_t id) override;
			
		void onParseDeclaration([[maybe_unused]] ChessPGN::PGNParser &parser,
			[[maybe_unused]] const std::string_view &name,
			[[maybe_unused]] const std::string_view &value) override;
		
		void onParseMove([[maybe_unused]] ChessPGN::PGNParser &parser,
			[[maybe_unused]] const std::string_view &move) override;
				
		void onError([[maybe_unused]] ChessPGN::PGNParser &parser,
			[[maybe_unused]] const std::string &str, 
			[[maybe_unused]] std::size_t line, 
			[[maybe_unused]] std::size_t col,
			[[maybe_unused]] char c) override;
		
		void onLog([[maybe_unused]] ChessPGN::PGNParser &parser,
			[[maybe_unused]] const std::string &str, 
			[[maybe_unused]] std::size_t line, 
			[[maybe_unused]] std::size_t col,
			[[maybe_unused]] ChessPGN::ParseMode mode) override;

		void onParseGameResult(
			[[maybe_unused]] ChessPGN::PGNParser &parser,
			[[maybe_unused]] const std::string_view &result) override;
		
		bool selectFromFile(const std::string &filename,
			const std::string &fileout, int minply, int maxply, 
			std::size_t samples, std::size_t mingameply, int maxscore, 
			bool quiet);

		void logErrorMove(ChessPGN::PGNParser &parser, 
			const std::string_view &move);
};

#endif //CHEESE_PGNSELECT_H_

