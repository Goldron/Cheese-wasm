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

#ifndef CHEESE_PLAYGAMES_H_
#define CHEESE_PLAYGAMES_H_

#include "board.h"
#include "engine.h"
#include "fen.h"
#include "move.h"

#include "pgn/pgn.h"

#include <atomic>
#include <string>
#include <vector>

using namespace Cheese;

////////////////////////////////////////////////////////////////////////////////
class PGNBook : public ChessPGN::PGNLoader {

	private:

		ChessEngine engine;

		std::vector<ChessPGN::PGNIndex> pgnindex;

		std::vector<ChessMove> moves;

		std::string pgnfile;

		std::string eco;

		std::size_t count {0};

	public:

		PGNBook();

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

		void logErrorMove(ChessPGN::PGNParser &parser, 
			const std::string_view &move);
	
		[[nodiscard]]
		bool loadPGNIndex(const std::string &filename);

		[[nodiscard]]
		std::string getECO() const
		{
			return eco;
		}

		[[nodiscard]]
		std::size_t countGames() const
		{
			return pgnindex.size();
		}

		[[nodiscard]]
		std::size_t countMoves() const
		{
			return moves.size();
		}

		[[nodiscard]]
		ChessMove getMove(std::size_t n) const
		{
			return moves[n];
		}

		bool loadPGNGame(std::size_t id);
};

////////////////////////////////////////////////////////////////////////////////
struct ChessGamePosition {
	std::string fen;
	int ply {0};
	bool sampled {false};

	ChessGamePosition(const std::string &f, int p, bool s) : 
		fen(f), ply(p), sampled(s)
	{}
};

class ChessGamePlayer;

////////////////////////////////////////////////////////////////////////////////
class ChessGameThread : public ChessEngineListener
{
public:

	ChessGamePlayer &gamePlayer;

	// pointer to the chess engine
	ChessEngine engine;

	ChessMoveSortList moves;

	std::mt19937_64 mtrnd;

	ChessMove bestMove {0};
	int searchDepth {0};
	int bestScore {0};

	std::uint64_t searchNodes {0};
	std::uint64_t searchTime {0};

	int game_id {0};
	int game_num {0};

	int game_round {0};
	int game_number {0};

	int tc_moves {0};
	int tc_time {0};
	int tc_inc {0};
	int tc_depth {0};

	int tc_depth_random {0};

	int timew {0};
	int timeb {0};

	int gamePly {0};
	int countDraw {0};
	int countScore {0};

	std::size_t gameRandomMoves {0};
	int gameRandomPos {0};


	ChessGameThread(ChessGamePlayer &gp);

	~ChessGameThread() override;

	void init();

	void playGame();

	ChessMove getFullRandomMove();

	ChessMove getRandomMove();

	[[nodiscard]]
	bool testGameTermination(std::string &strtermination, 
		std::string &strend, std::string &strresult);

	void samplePositions(std::vector<ChessGamePosition> &positions);

	void onEndSearch([[maybe_unused]] ChessEngineNotifier &notifier,
		ChessMove bm, [[maybe_unused]] ChessMove pm, int depth, 
		[[maybe_unused]] int ply, 
		[[maybe_unused]] std::uint64_t cntnodes,
		[[maybe_unused]] std::uint64_t searchtime) override
	{
		bestMove = bm;
		searchDepth = depth;
		searchNodes = cntnodes;
		searchTime = searchtime;
	}

	void onSendMultiPV([[maybe_unused]] ChessEngineNotifier &notifier, 
		[[maybe_unused]] int num, [[maybe_unused]] int depth,
		int score, [[maybe_unused]] const ChessPVMoveList &mlist, 
		[[maybe_unused]] int count, 
		[[maybe_unused]] std::uint64_t cntnodes, 
		[[maybe_unused]] std::uint64_t searchtime, 	
		[[maybe_unused]] int maxply, 
		[[maybe_unused]] int bound, [[maybe_unused]] std::uint64_t tbhits,
		[[maybe_unused]] std::uint64_t hashfull) 
			override
	{
		bestScore = score;
	}

	void onSearchRootMove([[maybe_unused]]ChessEngineNotifier &notifier,
		[[maybe_unused]] int depth, [[maybe_unused]] ChessMove move, 
		[[maybe_unused]] int num) override
	{}

	void onSendInfoMessage([[maybe_unused]] ChessEngineNotifier &notifier,
		[[maybe_unused]] const std::string &msg) override
	{}

};

////////////////////////////////////////////////////////////////////////////////
class ChessGamePlayer 
{
public:

	std::vector<std::unique_ptr<ChessGameThread>> games;

	std::vector<std::thread> threads;

	std::mutex mutex_write;

	std::mt19937_64 mtrnd;

	std::string filepgn;
	std::string fileepd;
	std::string fileopenings;
	std::string tb_path;

	PGNBook pgnBook;

	std::ofstream ofs;
	std::ofstream ofsepd;

	std::atomic_int game_id {0};
	std::atomic_int game_count {0};

	std::atomic_int count_draw {0};
	std::atomic_int count_win {0};
	std::atomic_int count_loss {0};
	std::atomic_int count_error {0};

	int game_round {0};
	int game_number {0};

	std::uint64_t countPositions {0};

	int searchDepth {0};
	std::uint64_t searchNodes {0};
	std::uint64_t searchTime {0};

	int hash_size {1};

	std::size_t randommoves {4};
	int randomdist {8};
	int multipv {4};
	int randomscoremargin {100};

	int min_game_ply {0};
	int min_ply {0};
	int max_ply {0};
	int max_score {eval_infinity};
	std::size_t samples {0};

	int draw_ply {40};
	int draw_count {8};
	int draw_score {10};

	int resign_count {4};
	int resign_score {600};

	int tc_moves {0};
	int tc_time {0};
	int tc_inc {0};

	int tc_depth {0};
	int tc_depth_random {12};
	
	std::size_t nb_threads {1};

	std::size_t bookmoves;

	bool full_random {false};
	bool min_pgn { false };
	bool quiet { false };

	ChessGamePlayer();

	void writePGN(const std::string &str);

	void writeEPD(const std::vector<ChessGamePosition> &positions, 
		const std::string &strresult);

	void output(const std::string &str);

	[[nodiscard]]
	std::size_t getRandomOpeningId();

	void init(std::size_t nbthreads, const std::string &file, 
		const std::string &fileeod, const std::string &filebook, 
		std::size_t nbbook, const std::string &tc, 
		int maxdepth, bool fullrandom, bool minpgn, bool quietpos, 
		int hashsize, const std::string &tbpath, int mingameply,
		int minply, int maxply, int maxscore, std::size_t nbsamples,
		int depthrand, int maxrand, int distrand, int multipvrand);

	void playGames(int count);

	void loadOpeningMoves(std::size_t num, ChessMoveSortList &moves);

};

#endif // CHEESE_PLAYGAMES_H_

