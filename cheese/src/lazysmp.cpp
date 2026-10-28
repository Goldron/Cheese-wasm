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

#include "lazysmp.h"

#include "engine.h"
#include "moveparser.h"
#include "search.h"

#include "logfile.h"

#include <map>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
ChessSearchThreadLazy::ChessSearchThreadLazy(int id, ChessSearchThreadList &th,
	ChessEngine &e) : ChessSearchThread(id, SMPMode::lazy, th, e)
{
}

////////////////////////////////////////////////////////////////////////////////
ChessSearchThread *ChessSearchThreadLazy::findBestThread()
{
    std::map<std::uint32_t, std::uint64_t> scores;

    // minimum score
	int minScore = 0;
	for (auto *th : threads) {
        minScore = std::min(minScore, th->bestScore);
    }	

	// keep best result, based on depth, score and occurence of the best move
	// note : this must be the first thread in the list or first scores  
	// access for bestThread will be uninitialized
	auto *bestThread = static_cast<ChessSearchThread *>(this);
	for (auto *th : threads) {

        Logger::debug() << "thread " << th->stid << " score = "
            << th->bestScore << " depth = " << th->searchDepth << " move " 
			<< ChessMoveParser::move_to_string_lan(bestMove);

		scores[th->bestMove.data()] += 
			static_cast<std::uint64_t>((th->bestScore - minScore + 1) * 
                th->searchDepth);

		if (scores.at(th->bestMove.data()) > 
			scores.at(bestThread->bestMove.data())) {
			bestThread = th;
		}
	}

    Logger::debug() << "best thread = " << bestThread->stid;

    return bestThread;
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadLazy::func()
{
	if (isMainThread()) {
		runMain();
    } else {
		runChild();
    }
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadLazy::runMain()
{
    while (!terminate) {

		std::unique_lock<std::mutex> lk(mutex);
		thinking = false;

		threads.notify();
		cond.wait(lk, [&]{ return thinking || terminate; });
		lk.unlock();

		if (!terminate) {

			searching = true;
			
			// wake up child threads
			for (auto *th : threads) {
				if (!th->isMainThread()) {
					th->startSearching();
				}
			}

            resetNodeCount();
			clearHistory();

			searchData.repetition = engine.getRepetitions();

			ChessSearch searchMain(searchData, engine, threads, 
				engine.getEval(), engine.getHashTable(), this, 
				engine.getBoardRef(), engine.mustCheckNodes());

			// start iterative deepening search
			searchMain.startSearch();

			// wait child threads to finish the search
			for (auto *th : threads) {
				if (!th->isMainThread()) {
					th->waitEndSearching();
				}
			}

            auto *bestThread = findBestThread();

			// send result
			engine.onEndSearch(bestThread->bestMove, 
                bestThread->ponderMove, bestThread->searchDepth, 
				threads.getMaxPly(), threads.getNodeCount(), 
				engine.getSearchTime());

			searching = false;
		}
	}   
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadLazy::runChild()
{
    while (!terminate) {

		std::unique_lock<std::mutex> lk(mutex);
		searching = false;
		cond.notify_one();
		cond.wait(lk, [&]{ return searching || terminate; });
		lk.unlock();

		if (searching) {

			resetNodeCount();
			clearHistory();

			searchData.repetition = engine.getRepetitions();

			ChessSearch searchMain(searchData, engine, threads, 
				engine.getEval(), engine.getHashTable(), this, 
				engine.getBoardRef(), false);

			// start iterative deepening search
			searchMain.startSearch();
		}
	}
}

} // namespace Cheese

