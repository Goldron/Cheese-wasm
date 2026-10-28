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

#ifndef CHEESE_BOOK_H_
#define CHEESE_BOOK_H_

#include "config.h"

#include "board.h"
#include "hash.h"
#include "move.h"

#include <fstream>
#include <random>
#include <string>

namespace Cheese {

// This code is based on Polyglot by Fabien Letousey

////////////////////////////////////////////////////////////////////////////////
// Polyglot book entry
struct BookFileEntry {
	std::uint64_t key;
	std::uint16_t move;
	std::uint16_t count;
	std::uint16_t n;
	std::uint16_t sum;
};

////////////////////////////////////////////////////////////////////////////////
// Chess opening book based on Polyglot book format
// the book is not loaded in memory to support big files
class ChessOpeningBook {

	private:

		// current book file stream
		std::ifstream ifs;

		// random number genertor
		std::mt19937_64 mtrnd;

		// size of the book in bytes
		std::size_t size {0};

	public:

		ChessOpeningBook() = default;

		~ChessOpeningBook();

		// open the book file
		bool open(const std::string &name);

		// close the book file
		void close();

		// find a move in the book for the position
		ChessMove findMove(const ChessBoard &board);

		// show all possible moves for a position
		bool showMoves(const ChessBoard &board);

	private:

		// build polygot hashkey from position
		std::uint64_t makePolyglotHashKey(const ChessBoard &board) const;

		// convert polyglot move to ChessMove
		ChessMove moveFromPolyglot(const ChessBoard &board, 
			std::uint16_t m) const;

		// find a position in the book
		bool findPosition(HashKey key, std::size_t &pos);

		// read an entry in the book
		bool read(std::size_t pos, BookFileEntry &entry);

};

} // namespace Cheese

#endif //CHEESE_BOOK_H_
