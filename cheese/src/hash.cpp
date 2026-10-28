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

#include "hash.h"

// * note : need eval.h only for mate_value
#include "eval.h"
#include "util/logfile.h"

#include <iomanip>
#include <iostream>
#include <memory>

#include <thread>

#include <cstdlib>
#include <cstring>

#ifdef WIN32
#include <malloc.h>
#endif

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
void *allocateMemory(std::size_t size, std::size_t alignment)
{	
	if constexpr (use_cache_alignment) {
#ifdef __APPLE__
		return aligned_alloc(alignment, size);
#elif defined(WIN32)
		return _mm_malloc(size, alignment);
#else
		return std::aligned_alloc(alignment, size);
#endif
	} else {
		return std::malloc(size);
	}
}

////////////////////////////////////////////////////////////////////////////////
void freeMemory(void *ptr)
{
	if constexpr (use_cache_alignment) {
#ifdef __APPLE__
		free(ptr);
#elif defined(WIN32)
		_mm_free(ptr);
#else
		std::free(ptr);
#endif
	} else {
		std::free(ptr);
	}
}

////////////////////////////////////////////////////////////////////////////////
ChessHashNode::ChessHashNode(HashKey k, ChessMove mv, HashNodeType type,
	int depth, int score, unsigned int age) : key(k)
{
	const auto sc = static_cast<std::uint16_t>(score);
	data = bits_move::encode(mv.data()) |
		   bits_type::encode(type) |
		   bits_depth::encode(depth) |
		   bits_score::encode(sc) |
		   bits_age::encode(age);
}

////////////////////////////////////////////////////////////////////////////////
ChessHashTable::~ChessHashTable()
{
	release();
}

////////////////////////////////////////////////////////////////////////////////
// create hash keys, and init hash table in Mb
bool ChessHashTable::init(std::size_t sizehash, int nt)
{
	std::size_t sz = std::clamp(sizehash, hashtable_size_min,
		hashtable_size_max);

	// init hash table
	if (hashtable != nullptr) {
		freeMemory(hashtable);
	}
	hashtable = nullptr;

	age = 0;

	// allocate to the next power of two size
	std::size_t n = 1;
	std::size_t asize = 1;

	while ((n < sz) && (n <= hashtable_size_max)) {
		n = (n << 1);
		if (n >= sz) {
			asize = n;
		}
	}

	static constexpr std::size_t sznode = sizeof(ChessHashNode);
	static constexpr auto one_mb = static_cast<std::size_t>(1024 * 1024);

	size = (asize * one_mb) / sznode;
	sizeModulo = size - nb_hash_cluster;

	Logger::info() << "main hash table size : " << sz << " Mb";
	Logger::debug() << "allocated size = " << asize << " Mb";
	Logger::debug() << "hash node size = " << sznode << " bytes";
	Logger::debug() << "size = " << size;
	Logger::debug() << "modulo = " << sizeModulo;

	hashtable = static_cast<ChessHashNode *>(allocateMemory(size * sznode,
		cache_aligned_value));

	if (hashtable == nullptr) {
		Logger::error() << "hash table : out of memory";
		return false;
	}

	clear(nt);

#ifdef HASH_STATS
	statExact = 0;
	statAlpha = 0;
	statBeta = 0;
	statCount = 0;
	statHit = 0;
	statDepth = 0;
	statStore = 0;
	for (n=0; n<nb_hash_cluster; n++) {
		statStoreBucket[n] = 0;
	}
#endif

	return true;
}

////////////////////////////////////////////////////////////////////////////////
// free hash table and keys
void ChessHashTable::release()
{
	if (hashtable != nullptr) {

#ifdef HASH_STATS
		Logger::debug() << "hash exact      = " << statExact;
		Logger::debug() << "hash alpha      = " << statAlpha;
		Logger::debug() << "hash beta       = " << statBeta;
		Logger::debug() << "hash count      = " << statCount;
		Logger::debug() << "hash hit        = " << statHit;
		Logger::debug() << "hash depth      = " << statDepth;
		Logger::debug() << "hash store      = " << statStore;

		for (int n=0; n<nb_hash_cluster; n++) {
			Logger::debug() << "hash store [" << n << "] = "
   					        << statStoreBucket[n];
		}

		Logger::debug() << "hash efficiency = "
				  		<< static_cast<double>(statHit) * 100.0 / 
						   static_cast<double>(statCount)
				        << " %";
#endif

		freeMemory(hashtable);
		hashtable = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////
// clear hash table
void ChessHashTable::clear(int nt)
{
	age = 0;
	if (hashtable != nullptr) {
#ifdef __EMSCRIPTEN__
		std::memset(static_cast<void *>(hashtable), 0, 
				size * sizeof(ChessHashNode));
#else
		if (nt <= 1) {
			std::memset(static_cast<void *>(hashtable), 0, 
				size * sizeof(ChessHashNode));
		} else {
			std::vector<std::thread> threads;
			threads.reserve(nt);
			for (int n=0; n<nt; n++) {
				threads.emplace_back([this, n, nt]() {
					const auto sz = size / nt;
					const auto nb = (n != (nt - 1)) ? sz : (size - n * sz);					
					std::memset(static_cast<void *>(hashtable + n * sz), 0, 
						nb * sizeof(ChessHashNode));  
				});
			}
			for (auto &t : threads) {
				t.join();
			}
		}
#endif
	}
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessHashTable::adjustScoreFrom(int score, int ply)
{
	if (score > max_score_mate) {
		return (score - ply);
	}

	if (score < min_score_mate) {
		return (score + ply);
	}

	return score;
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessHashTable::adjustScoreTo(int score, int ply)
{
	if (score > max_score_mate) {
		return (score + ply);
	}

	if (score < min_score_mate) {
		return (score - ply);
	}

	return score;
}

////////////////////////////////////////////////////////////////////////////////
// probe hash table
HashNodeType ChessHashTable::findNode(HashKey key, int depth, int ply,
	int alpha, int beta, ChessMove &cm, int &v)
{
	ChessHashNode *hash = getHashNode(key);

	ChessHashNode node;
	int n;
	for (n=0; n<nb_hash_cluster; n++) {
		hash[n].read(node);
		if (node.getKey() == key) {
			break;
		}
	}

#ifdef HASH_STATS
	++statCount;
#endif

	if (n < nb_hash_cluster) {

#ifdef HASH_STATS
		++statHit;
#endif
		cm = node.getMove();

		int hd = node.getDepth();

		if (hd >= depth) {

#ifdef HASH_STATS
			++statDepth;
#endif
			unsigned int ha = node.getAge();

			// adjust mate values because they depend on ply
			int hs = adjustScoreFrom(node.getScore(), ply);

			v = hs;

			HashNodeType htype = node.getType();

			switch (htype) {

				// lower bound
				case	HashNodeType::beta:
						if (hs >= beta) {
#ifdef HASH_STATS
							++statBeta;
#endif
							// refresh entry age
							if (age != ha) {
								node.setAge(age);
								hash[n].write(node);
							}

							return HashNodeType::beta;
						}
						break;

				// upper bound
				case	HashNodeType::alpha:
						if (hs <= alpha) {
#ifdef HASH_STATS
							++statAlpha;
#endif
							// refresh entry age
							if (age != ha) {
								node.setAge(age);
								hash[n].write(node);
							}

							return HashNodeType::alpha;
						}
						break;

				// exact value
				case	HashNodeType::exact:

#ifdef HASH_STATS
						++statExact;
#endif
						// refresh entry age
						if (age != ha) {
							node.setAge(age);
							hash[n].write(node);
						}

						return HashNodeType::exact;

				default:
					break;
			}
		} 
	}

	return HashNodeType::not_found;
}

////////////////////////////////////////////////////////////////////////////////
// add new hash entry in the hash table
void ChessHashTable::addNode(HashKey key, ChessMove move, int score,
	int depth, int ply, HashNodeType type)
{
	ChessHashNode *hash = getHashNode(key);

#ifdef HASH_STATS
	int pb = 0;
#endif

	ChessHashNode *hashnew = nullptr;

	// find same hashkey or empty
	for (int n=0; n<nb_hash_cluster; n++) {
		HashKey k = hash[n].readKey();
		if ((k == 0) || (k == key)) {
			hashnew = &hash[n];
#ifdef HASH_STATS
			pb = n;
#endif
			break;
		}
	}

	// if not found, try to replace previous age lowest depth,
	// or lowest depth available
	if (hashnew == nullptr) {

		static constexpr int max_score_age = 2048;
		static constexpr int max_score_depth = 1024;

		int mindepth = max_score_age + max_score_depth;

		for (int n=0; n<nb_hash_cluster; n++) {
			int hd = hash[n].getDepth();
			unsigned int ha = hash[n].getAge();
			int d = hd + ((ha == age) ? max_score_age : 0);
			if (d < mindepth) {
				hashnew = &hash[n];
				mindepth = d;
#ifdef HASH_STATS
				pb = n;
#endif
			}
		}
	}

	// adjust mate values because they depend on ply
	ChessHashNode node(key, move, type, depth, adjustScoreTo(score, ply), age);

	// replace
	hashnew->write(node);

#ifdef HASH_STATS
	++statStore;
	++statStoreBucket[pb];
#endif
}

////////////////////////////////////////////////////////////////////////////////
ChessMove ChessHashTable::getBestMove(HashKey key) const
{
	ChessHashNode *hash = getHashNode(key);
	ChessHashNode node;
	for (int n=0; n<nb_hash_cluster; n++) {
		hash[n].read(node);
		if (node.getKey() == key) {
			return node.getMove();
		}
	}
	return ChessMove(0);
}

////////////////////////////////////////////////////////////////////////////////
std::uint64_t ChessHashTable::getHashFull() const
{
	if (hashfull_max > size) {
		return 0;
	}

	// approximation on 1000 nodes
	std::uint64_t count = 0;
	for (std::size_t k=0; k<hashfull_max; k++) {
		ChessHashNode *hash = getHashNode(k);
		ChessHashNode node;
		for (int n=0; n<nb_hash_cluster; n++) {
			hash[n].read(node);
			if ((node.getDepth() != 0) && (node.getAge() == age)) {
				++count;
			}
		}
	}
	return count / nb_hash_cluster;
}

////////////////////////////////////////////////////////////////////////////////
// clear hash table
void ChessPawnHashTable::clear()
{
	ChessPawnHashNode node;
	node.key = 0;
	node.passed[0] = 0;
	node.passed[1] = 0;
	node.evalmid = 0;
	node.evalend = 0;
	hashtable.fill(node);

#ifdef HASH_STATS
	statFound = 0;
	statMiss = 0;
#endif
}

#ifdef HASH_STATS
////////////////////////////////////////////////////////////////////////////////
void ChessPawnHashTable::printStat()
{
	Logger::debug() << "hash pawn found     = " << statFound;
	Logger::debug() << "hash pawn miss      = " << statMiss;
	Logger::debug() << "hash paw efficiency = " <<
		static_cast<double>(statFound) * 100.0 /
		static_cast<double>(statFound + statMiss) << " %";
}
#endif

} // namespace Cheese
