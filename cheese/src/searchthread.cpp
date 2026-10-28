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

#include "searchthread.h"

#include "engine.h"
#include "lazysmp.h"
#include "moveorder.h"
#include "search.h"
#include "ybwc.h"

#include <numeric>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
ChessSearchThread::ChessSearchThread(int id, SMPMode mode,
	ChessSearchThreadList &th, ChessEngine &e) : smp(mode), stid(id),
	threads(th), engine(e), hashTablePawn()
{
	hashTablePawn.clear();
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThread::create()
{
	terminate = false;
	thread = std::thread(&ChessSearchThread::func, this);
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThread::stop()
{
	mutex.lock();
	terminate = true;
	mutex.unlock();
	cond.notify_one();
	thread.join();
}

////////////////////////////////////////////////////////////////////////////////
bool ChessSearchThread::isAvailable(
	[[maybe_unused]] ChessSearchThread *mainthread)
{
	return (!searching);
}

////////////////////////////////////////////////////////////////////////////////
bool ChessSearchThread::cutoff() const
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThread::resetNodeCount()
{
	countNodes = 0;
	maxPly = 0;
	tbhits = 0;
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThread::initSearch()
{
	bestMove.clear();
	ponderMove.clear();
	searchDepth = 0;
	bestScore = 0;
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThread::updateSearch(ChessMove bm, ChessMove pm,
	int depth, int score)
{
	bestMove = bm;
	ponderMove = pm;
	searchDepth = depth;
	bestScore = score;
}

////////////////////////////////////////////////////////////////////////////////
ChessSearchThreadList::~ChessSearchThreadList()
{
	destroy();
}

////////////////////////////////////////////////////////////////////////////////
ChessSearchThread *ChessSearchThreadList::createThread(SMPMode mode, int n,
	ChessEngine &engine)
{
	switch (mode) {
		case SMPMode::ybwc:
			return new ChessSearchThreadYBWC(n, *this, engine);
		case SMPMode::lazy:
			return new ChessSearchThreadLazy(n, *this, engine);
		default:
			Logger::error() << "unknown thread type : " 
						    << static_cast<int>(mode);
			return nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////
// create all search threads
void ChessSearchThreadList::create(ChessEngine &engine)
{
	destroy();

	threads.reserve(maxThread);

	for (int n=0; n<maxThread; n++) {
		auto *thread = createThread(engine.getSMPMode(), n, engine);
		if (thread != nullptr) {
			threads.push_back(thread);
			thread->create();
			thread->initMultiPV(1);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadList::destroy()
{
	for (ChessSearchThread *th : threads) {
		th->stop();

#ifdef HASH_STATS
		Logger::debug() << "Thread " << th->stid << " : ";
		th->hashTablePawn.printStat();
#endif

		delete th;
	}
	threads.clear();
}

////////////////////////////////////////////////////////////////////////////////
ChessSearchThread *ChessSearchThreadList::findAvailableThread(
	ChessSearchThread *thread)
{
	for (auto *th : threads) {
		if (th->isAvailable(thread)) {
			return th;
		}
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
bool ChessSearchThreadList::isThreadAvailable(ChessSearchThread *thread)
{
	for (auto *th : threads) {
		if ((th->getSearchId() != thread->getSearchId()) &&
			(th->isAvailable(thread))) {
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
std::uint64_t ChessSearchThreadList::getNodeCount() const
{
	return std::accumulate(threads.begin(), threads.end(), 0ULL,
		[](std::uint64_t v, auto *th) { return v + th->countNodes; });
}

////////////////////////////////////////////////////////////////////////////////
int ChessSearchThreadList::getMaxPly() const
{
	int maxply = 0;
	for (auto *th : threads) {
		const int p = th->maxPly;
		if (p > maxply) {
			maxply = p;
		}
	}
	return maxply;
}

////////////////////////////////////////////////////////////////////////////////
std::uint64_t ChessSearchThreadList::getTBHits() const
{
	return std::accumulate(threads.begin(), threads.end(), 0ULL,
		[](std::uint64_t v, auto *th) { return v + th->tbhits; });
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadList::clearHistory()
{
	for (auto *th : threads) {
		th->clearHistory();
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadList::resetNodeCount()
{
	for (auto *th : threads) {
		th->resetNodeCount();
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadList::setMaxThreads(int m, ChessEngine &engine)
{
	maxThread = std::clamp(m, 1, (engine.getSMPMode() == SMPMode::ybwc) ?
		max_threads_ybwc : max_threads);

	Logger::info() << "Using " << maxThread << " threads";

	auto nb = std::thread::hardware_concurrency();
	if (maxThread > static_cast<int>(nb)) {
		Logger::warning() << "number of hardware threads supported = " << nb;
	}

	create(engine);
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadList::initMultiPV(int size)
{
	// set multipv only for main thread
	for (auto *th : threads) {
		th->initMultiPV(size);
	}
}

////////////////////////////////////////////////////////////////////////////////
void ChessSearchThreadList::waitEndSearch()
{
	std::unique_lock<std::mutex> lk(mutex);
	cond.wait(lk, [&]{ return !getMainThread()->thinking; });
}

} // namespace Cheese
