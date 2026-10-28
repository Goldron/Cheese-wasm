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

#ifndef CHEESE_LAZY_SMP_H_
#define CHEESE_LAZY_SMP_H_

#include "config.h"

#include "search.h"
#include "searchthread.h"
#include "moveorder.h"
#include "move.h"

namespace Cheese {

// "Lazy SMP" idea implementation (Shared hash table) :
// launch N search at the same time, sharing only through the
// hash table, and keep the best result

class ChessEngine;

////////////////////////////////////////////////////////////////////////////////
class ChessSearchThreadLazy final : public ChessSearchThread {

	ChessSearchData searchData;

public:

	ChessSearchThreadLazy(int id, ChessSearchThreadList &th, ChessEngine &e);

	~ChessSearchThreadLazy() override = default;

	// thread main function, called by thread
	void func() override;

    // find the thread with the most accurate result
	[[nodiscard]]
    ChessSearchThread *findBestThread();

    // code for main thread
	void runMain();

    // code for child threads
	void runChild();

	// test if we have not reached the maximum splitpoint for this thread
	[[nodiscard]]
	bool canSplit() const override;

	// if a cutoff happens in splitpoint or parent splitpoints
	[[nodiscard]]
	bool cutoff() const override;

	// test if the thread is available to help another thread
	[[nodiscard]]
	bool isAvailable(ChessSearchThread *mainthread) override;
};

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThreadLazy::canSplit() const
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThreadLazy::cutoff() const
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThreadLazy::isAvailable(
	[[maybe_unused]] ChessSearchThread *mainthread)
{
	return true;
}

} // namespace Cheese

#endif //CHEESE_LAZY_SMP_H_
