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

#ifndef CHEESE_SEARCH_THREAD_H_
#define CHEESE_SEARCH_THREAD_H_

#include "config.h"

#include "history.h"
#include "hash.h"
#include "move.h"
#include "movegen.h"
#include "pv.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace Cheese {

class ChessEngine;
class ChessSearchThreadList;
class ChessSearchThread;
class ChessMoveOrder;
class ChessRepetitions;
class ChessPV;

// smp search algorithms
enum class SMPMode {

	// YBWC (Young Brother Wait Concept)
	ybwc,

	// "Lazy SMP" (Shared hash table)
	lazy

	// TODO : (ABDADA) Alpha Beta Distribué Avec Droits d'Aînesse
	//abdada
};

// maximum number of threads supported
// (YBWC : 64 because using a 64 bits number for splitpoint threads mask)
#ifdef __EMSCRIPTEN__
inline constexpr int max_threads_ybwc = 32;
#else
inline constexpr int max_threads_ybwc = 64;
#endif

#ifdef __EMSCRIPTEN__
inline constexpr int max_threads = 32;
#else
#ifdef WIN32
// TODO : support more than 64 threads for Windows
inline constexpr int max_threads = 64;
#else
inline constexpr int max_threads = 256;
#endif
#endif

////////////////////////////////////////////////////////////////////////////////
// base class for searching threads
class ChessSearchThread {

public:

	std::thread thread;
	
	std::condition_variable cond;

	std::mutex mutex;

	std::atomic_bool terminate {false};

	// smp algorithm
	SMPMode smp;

	// search unique thread id
	// (0 = main thread, 1 = 1st additional thread, ...)
	// can't use Thread::id because we don't know if other threads
	// are created before)
	int stid;

	// reference to thread list
	ChessSearchThreadList &threads;

	// pointer on the chess engine
	ChessEngine &engine;

	// current node count for this thread
	std::atomic_uint64_t countNodes {0};

	// maximum ply reached by this thread
	std::atomic_int maxPly {0};

	std::atomic_uint64_t tbhits {0};

	// the thread is searching
	std::atomic_bool searching {false};

	// main thread only, we are thinking
	std::atomic_bool thinking {false};

	ChessMove bestMove;

	ChessMove ponderMove;

	int searchDepth {0};

	int bestScore {0};

	// excluded moves at root
	ChessMoveList excludedMoves;

	ChessMultiPV mpv;

	// history heuristic
	ChessHistory history;

	// pawn hash table
	ChessPawnHashTable hashTablePawn;

	ChessSearchThread(int id, SMPMode mode, ChessSearchThreadList &th,
		ChessEngine &e);

	virtual ~ChessSearchThread() = default;

	void resetNodeCount();

	void initSearch();

	void updateSearch(ChessMove bm, ChessMove pm, int depth, int score);

	void startSearching();

	void waitEndSearching();

	void clearHistory();

	[[nodiscard]]
	bool isExcludedMove(ChessMove move) const;

	void initMultiPV(int size);

	[[nodiscard]]
	int getSearchId() const;

	// main thread search id must be = 0
	[[nodiscard]]
	bool isMainThread() const;

	// create system thread
	void create();

	// stop system thread
	void stop();

	void notify();

	// test if we have not reached the maximum splitpoint for this thread
	[[nodiscard]]
	virtual bool canSplit() const;

	// if a cutoff happens in splitpoint or parent splitpoints
	[[nodiscard]]
	virtual bool cutoff() const;

	// test if the thread is available to help another thread
	[[nodiscard]]
	virtual bool isAvailable(ChessSearchThread *mainthread);

	// thread main function
	virtual void func()
	{ }
};

////////////////////////////////////////////////////////////////////////////////
// list all searching threads
class ChessSearchThreadList {

	// all search threads the first thread is main thread 
	std::vector<ChessSearchThread *> threads;

	std::condition_variable cond;

	std::mutex mutex;

	// all threads must stop searching
	std::atomic_bool stop {false};

	// maximum number of threads used by the search
	int maxThread {1};

public:

	ChessSearchThreadList() = default;

	~ChessSearchThreadList();

	[[nodiscard]]
	auto begin();

	[[nodiscard]]
	auto end();

	[[nodiscard]]
	bool mustStop() const;

	void stopSearch();

	void initSearch();

	void lock();

	void unlock();

	void notify();

	// wait for the end of current search
	void waitEndSearch();
	
	// create and launch all threads
	void create(ChessEngine &engine);

	[[nodiscard]]
	auto size() const;

	// stop and destroy all threads
	void destroy();

	void clearHistory();

	void resetNodeCount();

	// total node count for all threads
	[[nodiscard]]
	std::uint64_t getNodeCount() const;

	// get maximum ply reached by all threads
	[[nodiscard]]
	int getMaxPly() const;

	[[nodiscard]]
	std::uint64_t getTBHits() const;

	[[nodiscard]]
	int getMaxThreads() const;

	// change number of threads
	void setMaxThreads(int m, ChessEngine &engine);

	void initMultiPV(int size);

	auto *getMainThread();

	[[nodiscard]]
	bool isThinking();

	// YBWC : find an available thread
	// note : must set thread child mask or the result will be = thread
	ChessSearchThread *findAvailableThread(ChessSearchThread *thread);

	// YBWC : test if a search thread is available and ready
	// to help current thread
	[[nodiscard]]
	bool isThreadAvailable(ChessSearchThread *thread);
	
private:

	ChessSearchThread *createThread(SMPMode mode, int n, ChessEngine &engine);

};

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThread::startSearching()
{
	std::unique_lock<std::mutex> lk(mutex);
	searching = true;
	cond.notify_one();
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThread::waitEndSearching()
{
	std::unique_lock<std::mutex> lk(mutex);
	cond.wait(lk, [&]{ return !searching; });
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThread::clearHistory()
{
	history.clear();
	history.clearKillers();
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessSearchThread::getSearchId() const
{
	return stid;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThread::isMainThread() const
{
	return (stid == 0);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThread::notify()
{
	std::lock_guard<std::mutex> lk(mutex);
	cond.notify_one();
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThread::canSplit() const
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThread::isExcludedMove(ChessMove move) const
{
	return ((!excludedMoves.empty()) &&
			(std::find(excludedMoves.begin(), excludedMoves.end(), move) !=
			excludedMoves.end()));
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThread::initMultiPV(int size)
{
	mpv.init(size);
	mpv.clear();
}

////////////////////////////////////////////////////////////////////////////////
inline auto ChessSearchThreadList::begin()
{
	return threads.begin();
}

////////////////////////////////////////////////////////////////////////////////
inline auto ChessSearchThreadList::end()
{
	return threads.end();
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThreadList::mustStop() const
{
	return stop;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThreadList::stopSearch()
{
	stop = true;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThreadList::initSearch()
{
	stop = false;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThreadList::lock()
{
	mutex.lock();
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThreadList::unlock()
{
	mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessSearchThreadList::notify()
{
	cond.notify_one();
}

////////////////////////////////////////////////////////////////////////////////
inline auto ChessSearchThreadList::size() const
{
	return threads.size();
}

////////////////////////////////////////////////////////////////////////////////
inline auto *ChessSearchThreadList::getMainThread()
{
	return threads.front();
}

////////////////////////////////////////////////////////////////////////////////
inline bool ChessSearchThreadList::isThinking()
{
	return threads.empty() ? false : (getMainThread()->thinking == true);
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessSearchThreadList::getMaxThreads() const
{
	return maxThread;
}

} // namespace Cheese

#endif //CHEESE_SEARCH_THREAD_H_
