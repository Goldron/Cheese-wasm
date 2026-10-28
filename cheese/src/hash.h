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

#ifndef CHEESE_HASH_H_
#define CHEESE_HASH_H_

#include "config.h"

#include "util/bitfield.h"

#include "move.h"
#include "zobrist.h"

#include <array>
#include <atomic>

// define to use stats on hash table
//#define HASH_STATS

namespace Cheese {

inline constexpr bool use_cache_alignment = true;

// use the lockless hash idea
inline constexpr bool use_lockless_hash = true;

// use hashfull stat
inline constexpr bool use_hashfull = true;

inline constexpr int cache_aligned_value = 64;

// number of hash cluster
inline constexpr int nb_hash_cluster = 4;

// hash table default size (in Mb)
inline constexpr std::size_t hashtable_size_default = 32;

// hash table minimum size (in Mb)
inline constexpr std::size_t hashtable_size_min = 1;

// hash table maximum size (in Mb)
// in 32 bits, max allocated size < 4 Gb
#ifdef __EMSCRIPTEN__
inline constexpr std::size_t hashtable_size_max = 512;
#else
#ifdef SYSTEM64BIT
inline constexpr std::size_t hashtable_size_max = 65536;
#else
inline constexpr std::size_t hashtable_size_max = 2048;
#endif
#endif

// default size for pawn hash table (in Kb)
inline constexpr std::size_t hashtable_pawn_size = 2048;

// maximum value for full hash table
inline constexpr std::size_t hashfull_max = 1000;

////////////////////////////////////////////////////////////////////////////////
// hash node type
enum class HashNodeType {

	// exact value
	exact,

	// lower bound
	alpha,

	// upper bound
	beta,

	// hash entry not found
	not_found
};

class ChessHashTable;

////////////////////////////////////////////////////////////////////////////////
// size = 64 bits key + 64 bits data = 128 bits = 16 bytes
struct ChessHashNode {

	using data_type = std::uint64_t;

private:

	using bits_depth = BitField<data_type, 0, 8>;
	using bits_age   = BitField<data_type, 8, 8>;
	using bits_score = BitField<data_type, 16, 16>;
	using bits_move  = BitField<data_type, 32, 24>;
	using bits_type  = BitField<data_type, 56, 2>;

	// hash key (64 bits)
	HashKey key;

	// data (64 bits)

	// high 32 bits part :
	// 00000000 xxxxxxxx xxxxxxxx xxxxxxxx ...  = move   (24 bits)
	// 000000xx 00000000 00000000 00000000 ...  = type   (2 bits)
	// ??????00 00000000 00000000 00000000 ...  = unused (6 bits)

	// low 32 bits part :
	// ... 00000000 00000000 00000000 xxxxxxxx  = depth (8 bits) in 1/2 PLY
	// ... 00000000 00000000 xxxxxxxx 00000000  = age   (8 bits)
	// ... xxxxxxxx xxxxxxxx 00000000 00000000  = score (16 bits)

	data_type data;

public:

	friend class ChessHashTable;

	ChessHashNode() = default;

	ChessHashNode(HashKey k, ChessMove mv, HashNodeType type, int depth,
		int score, unsigned int age);

	void clear();

	void setData(ChessMove mv, HashNodeType type, int depth, int score, 
		unsigned int age);

	// set hash key
	void setKey(HashKey k);

	// get type of node
	[[nodiscard]]
	HashNodeType getType() const;

	// get search depth
	[[nodiscard]]
	int getDepth() const;

	// get age
	[[nodiscard]]
	unsigned int getAge() const;

	// set age
	void setAge(unsigned int age);

	// get score
	[[nodiscard]]
	int getScore() const;

	// get hash key
	[[nodiscard]]
	HashKey getKey() const;

	// get move
	[[nodiscard]]
	ChessMove getMove() const;

	// read entry key (support lockless)
	[[nodiscard]]
	HashKey readKey() const;

	// read entry (support lockless)
	void read(ChessHashNode &node) const;

	// write entry (support lockless)
	void write(const ChessHashNode &node);

	static void increaseAge(unsigned int &age)
	{
		age = (age + 1) & bits_age::mask();
	}
};

////////////////////////////////////////////////////////////////////////////////
class ChessHashTable
{
	// hash table (aligned)
	ChessHashNode *hashtable {nullptr};

	// hash table size
	std::size_t size {0};

	// hash table modulo
	std::size_t sizeModulo {0};

	// age
	unsigned int age {0};

	// stats
#ifdef HASH_STATS
	std::atomic_uint64_t statExact {};
	std::atomic_uint64_t statAlpha {};
	std::atomic_uint64_t statBeta {};
	std::atomic_uint64_t statCount {};
	std::atomic_uint64_t statHit {};
	std::atomic_uint64_t statDepth {};
	std::atomic_uint64_t statStore {};
	std::array<std::atomic_uint64_t, nb_hash_cluster> statStoreBucket {};
#endif

public:

	ChessHashTable() = default;

	~ChessHashTable();

	// allocate and clear hash table
	// nt = number of threads to clear
	bool init(std::size_t sizehash, int nt = 1);

	// free hash table
	void release();

	// clear hash table
	// nt = number of threads
	void clear(int nt = 1);

	// must be called at initialization of the search if using
	// the age parameter in the hash
	void initSearch();

	// hash full approximation
	[[nodiscard]]
	std::uint64_t getHashFull() const;

	// get hash node
	[[nodiscard]]
	ChessHashNode *getHashNode(HashKey key) const;

	// probe hash table
	[[nodiscard]]
	HashNodeType findNode(HashKey key, int depth, int ply, int alpha, int beta,
		ChessMove &cm, int &v);

	// add new hash entry in the hash table
	void addNode(HashKey key, ChessMove move, int score, int depth, int ply, 
		HashNodeType type);

	// get best move from hash table
	[[nodiscard]]
	ChessMove getBestMove(HashKey key) const;

private:

	// adjust score for mate after reading from the hashtable
	[[nodiscard]]
	static int adjustScoreFrom(int score, int ply);

	// adjust score for mate before writing to the hashtable
	[[nodiscard]]
	static int adjustScoreTo(int score, int ply);

};

////////////////////////////////////////////////////////////////////////////////
// Pawn hashtable entry :
//
// key      =  8 bytes
// passed   = 16 bytes
// evalmid  =  4 bytes
// evalend  =  4 bytes
//
//  Total   = 32 bytes
//
struct ChessPawnHashNode {

	// hashkey (64 bits)
	HashKey key;

	// passed pawns
	std::array<std::uint64_t, nb_sides> passed;

	// pawn evaluation (without passed pawns eval)
	std::int32_t evalmid;

	std::int32_t evalend;

	ChessPawnHashNode() = default;

	void setKey(HashKey k);

	[[nodiscard]]
	HashKey getKey() const;

	void get(std::array<std::uint64_t, nb_sides> &pp, std::int32_t &vm, 
		std::int32_t &ve) const;

	void set(HashKey k, const std::array<std::uint64_t, nb_sides> &pp, 
		std::int32_t vm, std::int32_t ve);

};

// 2048 Kb / 32 bytes => 65536 elements
inline constexpr auto hashtable_pawn_nb_entry = static_cast<std::size_t>(
	(hashtable_pawn_size * 1024) / sizeof(ChessPawnHashNode));

////////////////////////////////////////////////////////////////////////////////
class ChessPawnHashTable {

	// hash table
	std::array<ChessPawnHashNode, hashtable_pawn_nb_entry> hashtable;

#ifdef HASH_STATS
	std::atomic_uint64_t statFound{};
	std::atomic_uint64_t statMiss{};
#endif

public:

	ChessPawnHashTable() = default;

	~ChessPawnHashTable() = default;

#ifdef HASH_STATS
	void printStat();
#endif

	// clear hash table
	void clear();

	// get hash node
	[[nodiscard]]
	ChessPawnHashNode *getHashNode(HashKey key);
};

////////////////////////////////////////////////////////////////////////////////
inline void ChessHashNode::clear()
{
	key = 0ULL;
	data = 0ULL;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHashNode::setData(ChessMove mv, HashNodeType type, int depth,
	int score, unsigned int age)
{
	const auto sc = static_cast<std::uint16_t>(score);
	data = bits_move::encode(mv.data()) |
		   bits_type::encode(type) |
		   bits_depth::encode(depth) |
		   bits_score::encode(sc) |
		   bits_age::encode(age);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHashNode::setKey(HashKey k)
{
	key = k;
}

////////////////////////////////////////////////////////////////////////////////
inline HashNodeType ChessHashNode::getType() const
{
	return bits_type::get<HashNodeType>(data);
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessHashNode::getDepth() const
{
	return bits_depth::get<int>(data);
}

////////////////////////////////////////////////////////////////////////////////
inline unsigned int ChessHashNode::getAge() const
{
	return bits_age::get<unsigned int>(data);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHashNode::setAge(unsigned int age)
{
	bits_age::set(data, age);
}

////////////////////////////////////////////////////////////////////////////////
inline int ChessHashNode::getScore() const
{
	auto sc = bits_score::get<std::int16_t>(data);
	return static_cast<int>(sc);
}

////////////////////////////////////////////////////////////////////////////////
inline HashKey ChessHashNode::getKey() const
{
	return key;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessMove ChessHashNode::getMove() const
{
	return ChessMove(bits_move::get<std::uint32_t>(data));
}

////////////////////////////////////////////////////////////////////////////////
inline HashKey ChessHashNode::readKey() const
{
	if constexpr (use_lockless_hash) {
		return (key ^ data);
	} else {
		return key;
	}
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHashNode::read(ChessHashNode &node) const
{
	if constexpr (use_lockless_hash) {
		node.key = key ^ data;
	} else {
		node.key = key;
	}
	node.data = data;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHashNode::write(const ChessHashNode &node)
{
	if constexpr (use_lockless_hash) {
		key = node.key ^ node.data;
	} else {
		key = node.key;
	}
	data = node.data;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessHashTable::initSearch()
{
	ChessHashNode::increaseAge(age);
}

////////////////////////////////////////////////////////////////////////////////
inline ChessHashNode *ChessHashTable::getHashNode(HashKey key) const
{
	return hashtable + static_cast<std::uintptr_t>(key & sizeModulo);
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessPawnHashNode::setKey(HashKey k)
{
	key = k;
}

////////////////////////////////////////////////////////////////////////////////
inline HashKey ChessPawnHashNode::getKey() const
{
	return key;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessPawnHashNode::get(std::array<std::uint64_t, nb_sides> &pp, 
	std::int32_t &vm, std::int32_t &ve) const
{
	pp[black] = passed[black];
	pp[white] = passed[white];
	vm = evalmid;
	ve = evalend;
}

////////////////////////////////////////////////////////////////////////////////
inline void ChessPawnHashNode::set(HashKey k, 
	const std::array<std::uint64_t, nb_sides> &pp, std::int32_t vm, 
		std::int32_t ve)
{
	key = k;
	passed[black] = pp[black];
	passed[white] = pp[white];
	evalmid = vm;
	evalend = ve;
}

////////////////////////////////////////////////////////////////////////////////
inline ChessPawnHashNode *ChessPawnHashTable::getHashNode(HashKey key)
{
	const auto i = static_cast<std::uintptr_t>(key & 
		(hashtable_pawn_nb_entry - 1));
	auto *hp = &hashtable[i];

#ifdef HASH_STATS
	if (hp->key == key) {
		++statFound;
	} else {
		++statMiss;
	}
#endif	

	return hp;
}

} // namespace Cheese

#endif //CHEESE_HASH_H_
