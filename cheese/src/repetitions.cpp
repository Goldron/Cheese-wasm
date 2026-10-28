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

#include "repetitions.h"

#include "util/logfile.h"

#include <algorithm>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
bool ChessRepetitions::check(HashKey h, int fifty) const
{
	if (fifty < 4) {
		return false;
	}

	int num = nbRep - 4;
	int p = std::max(0, nbRep - 1 - fifty);

	while (num >= p) {
		if (repetitions[num] == h) {
			return true;
		}
		num -= 2;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////
int ChessRepetitions::getCount(HashKey h, int fifty) const
{
	if (fifty < 4) {
		return 0;
	}

	int count = 0;
	int num = nbRep - 4;
	int p = std::max(0, nbRep - 1 - fifty);

	while (num >= p) {
		if (repetitions[num] == h) {
			++count;
		}
		num -= 2;
	}

	return count;
}

////////////////////////////////////////////////////////////////////////////////
void ChessRepetitions::print()
{
	Logger::debug() << "repetitions : " << nbRep;
	for (int n=0; n<nbRep; n++) {
		Logger::debug() << Zobrist::hashKeyToString(repetitions[n]) << ' ';
	}
}

} // namespace Cheese
