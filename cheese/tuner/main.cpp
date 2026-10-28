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

#include "board.h"
#include "engine.h"
#include "fen.h"
#include "movegen.h"
#include "moveparser.h"
#include "pgnselect.h"
#include "tuner.h"
#include "util/logfile.h"
#include "playgames.h"

#include <iostream>
#include <string>
#include <vector>

using namespace Cheese;

////////////////////////////////////////////////////////////////////////////////
bool filterPositions(const std::string &filename, const std::string &fileresult)
{
	ChessEngine::precalcTables();

	ChessBoard board;
	std::vector<std::string> fens;
	if (!ChessFENParser::load(filename, fens)) {
		Logger::error() << "file not found : " << filename;
		return false;
	}

	std::ofstream ofs;
	ofs.open(fileresult, std::ofstream::out);

	std::size_t count = 0;
	for (const auto &f : fens) {

		if (!ChessFENParser::set(f, board)) {
			Logger::warning() << "FEN parsing error : " << f;
			continue;
		}

		if (ChessEngine::isPositionQuiet(board)) {
			ofs << f << '\n';
			++count;
		}
	}
	ofs.close();

	std::cout << "saved " << count << " / " << fens.size() 
			  << " positions" << std::endl;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
void printHelp()
{
	std::cout << "Cheese tuner " << Cheese::project_ver << "\n\n";

	std::cout << "cheese_tuner -tune -epd FILE.epd [-threads NBTHREADS] \n";
	std::cout << "        [-batchsize NBBATCH] [-stop NBITERATIONS]\n";
	std::cout << "        [-display COUNTDISPLAY] [-learningrate RATE]\n";
	std::cout << "        [-reducelearningrate REDUCE]\n\n";

	std::cout << "cheese_tuner -playgames [-games NBGAMES] [-pgnout FILEOUT.pgn]\n";
	std::cout << "        [-epdout FILEOUT.epd] [-minpgn] [-quiet] [-samples COUNT]\n";
	std::cout << "        [-maxscore MAXSCORE] [-mingameply MINGAMEPLY]\n";
	std::cout << "        [-minply MINPLY] [-maxply MAXPLY] [-openings BOOK.pgn]\n";
	std::cout << "        [-bookmoves COUNT] [-fullrandom] [-tc TIMECONTROL]\n";
	std::cout << "        [-depth DEPTH] [-depthrandom DEPTH] [-randommoves COUNT]\n";
	std::cout << "        [-randomdistance DISTANCE] [-randommultipv COUNT]\n";
	std::cout << "        [-threads COUNT] [-enginethreads COUNT] [-hash SIZE]\n";
	std::cout << "        [-tb PATH]\n\n";

	std::cout << "cheese_tuner -pgntoepd FILEIN.pgn -out FILEOUT.epd \n";
	std::cout << "        [-minply MINPLY] [-maxply MAXPLY] [-maxscore MAXSCORE]\n";
	std::cout << "        [-mingameply MINGAMEPLY] [-samples SAMPLES] [-quiet]\n\n";

	std::cout << "cheese_tuner -filter FILEIN.epd -out FILEOUT.epd\n\n";

	std::cout << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
int cmdFilter(const std::vector<std::string> &args)
{
	std::string cmd, filein, fileout;

	std::size_t p = 1;
	while (p < args.size()) {

		if (args[p] == "-filter") {
			cmd = args[p];
			++p;
			if (p < args.size()) {
				filein = args[p];
			}
		}

		if (args[p] == "-out") {
			++p;
			if (p < args.size()) {
				fileout = args[p];
			}
		}

		++p;
	}

	return filterPositions(filein, fileout) ? 0 : 1;
}

////////////////////////////////////////////////////////////////////////////////
int cmdPGNToEPD(const std::vector<std::string> &args)
{
	std::string cmd, filein, fileout;
	int minply = 0, maxply = 0;
	std::size_t samples = 0, mingameply = 0;
	int maxscore = 0;
	bool quiet = false;

	std::size_t p = 1;
	while (p < args.size()) {

		if ((args[p] == "-help") ||
			(args[p] == "--help") ||
			(args[p] == "-h")) {
			printHelp();
			return 0;
		}

		if (args[p] == "-pgntoepd") {
			cmd = args[p];
			++p;
			if (p < args.size()) {
				filein = args[p];
			}
		}

		if (args[p] == "-minply") {
			++p;
			if (p < args.size()) {
				minply = std::stoi(args[p]);
			}
		}

		if (args[p] == "-maxply") {
			++p;
			if (p < args.size()) {
				maxply = std::stoi(args[p]);
			}
		}

		if (args[p] == "-maxscore") {
			++p;
			if (p < args.size()) {
				maxscore = std::stoi(args[p]);
			}
		}

		if (args[p] == "-mingameply") {
			++p;
			if (p < args.size()) {
				mingameply = std::stoi(args[p]);
			}
		}

		if (args[p] == "-samples") {
			++p;
			if (p < args.size()) {
				samples = std::stoi(args[p]);
			}
		}

		if (args[p] == "-quiet") {
			quiet = true;
		}

		++p;
	}
	
	ChessEngine::precalcTables();

	PGNSelect pgn;

	return pgn.selectFromFile(filein, fileout, minply, maxply, samples, 
			mingameply, maxscore, quiet) ? 0 : 1;
}

////////////////////////////////////////////////////////////////////////////////
int cmdPlayGames(const std::vector<std::string> &args)
{
	std::string cmd, fileout, fileepdout, fileopenings = "", timecontrol, 
		tbpath = "";
	std::size_t bookmoves = 0, nbthreads = 1;
	int count = 1, mingameply = 1, minply = 1, maxply = 1000, 
		maxscore = eval_infinity;
	std::size_t samples = 0;
	int hashsize=256, maxdepth=0;
	int maxdepthrandom=12, nbrandom=0, multipv=4;
	int randomdist = 4;
	bool fullrandom = false, minpgn = false, quiet = false;

	std::size_t p = 1;
	while (p < args.size()) {

		if (args[p] == "-playgames") {
			cmd = args[p];
		}

		if (args[p] == "-games") {
			++p;
			if (p < args.size()) {
				count = std::stoi(args[p]);
			}
		}

		if (args[p] == "-pgnout") {
			++p;
			if (p < args.size()) {
				fileout = args[p];
			}
		}

		if (args[p] == "-epdout") {
			++p;
			if (p < args.size()) {
				fileepdout = args[p];
			}
		}
		
		if (args[p] == "-minpgn") {
			minpgn = true;
		}

		if (args[p] == "-quiet") {
			quiet = true;
		}

		if (args[p] == "-samples") {
			++p;
			if (p < args.size()) {
				samples = std::stoi(args[p]);
			}
		}

		if (args[p] == "-maxscore") {
			++p;
			if (p < args.size()) {
				maxscore = std::stoi(args[p]);
			}
		}

		if (args[p] == "-mingameply") {
			++p;
			if (p < args.size()) {
				mingameply = std::stoi(args[p]);
			}
		}

		if (args[p] == "-minply") {
			++p;
			if (p < args.size()) {
				minply = std::stoi(args[p]);
			}
		}

		if (args[p] == "-maxply") {
			++p;
			if (p < args.size()) {
				maxply = std::stoi(args[p]);
			}
		}

		if (args[p] == "-openings") {
			++p;
			if (p < args.size()) {
				fileopenings = args[p];
			}
		}

		if (args[p] == "-bookmoves") {
			++p;
			if (p < args.size()) {
				bookmoves = std::stoi(args[p]);
			}
		}

		if (args[p] == "-fullrandom") {
			fullrandom = true;
		}

		if (args[p] == "-tc") {
			++p;
			if (p < args.size()) {
				timecontrol = args[p];
			}
		}

		if (args[p] == "-depth") {
			++p;
			if (p < args.size()) {
				maxdepth = std::stoi(args[p]);
			}
		}

		if (args[p] == "-depthrandom") {
			++p;
			if (p < args.size()) {
				maxdepthrandom = std::stoi(args[p]);
			}
		}

		if (args[p] == "-randommoves") {
			++p;
			if (p < args.size()) {
				nbrandom = std::stoi(args[p]);
			}
		}

		if (args[p] == "-randomdistance") {
			++p;
			if (p < args.size()) {
				randomdist = std::stoi(args[p]);
			}
		}

		if (args[p] == "-randommultipv") {
			++p;
			if (p < args.size()) {
				multipv = std::stoi(args[p]);
			}
		}

		if (args[p] == "-threads") {
			++p;
			if (p < args.size()) {
				nbthreads = std::stoi(args[p]);
			}
		}

		if (args[p] == "-enginethreads") {
		}

		if (args[p] == "-hash") {
			++p;
			if (p < args.size()) {
				hashsize = std::stoi(args[p]);
			}
		}

		if (args[p] == "-tb") {
			++p;
			if (p < args.size()) {
				tbpath = args[p];
			}
		}

		++p;
	}

	ChessEngine::precalcTables();

	ChessGamePlayer gp;
	gp.init(nbthreads, fileout, fileepdout, fileopenings, bookmoves, 
		timecontrol, maxdepth, fullrandom, minpgn, quiet, hashsize, tbpath,
		mingameply, minply, maxply, maxscore, samples, maxdepthrandom, 
		nbrandom, randomdist, multipv);
	gp.playGames(count);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
int cmdTuner(const std::vector<std::string> &args)
{
	std::string cmd, fileepd;
	std::size_t nbthreads = 1, nbbatch = 16384, display = 100, stop = 6000;
	double rate = 5.0, reduce = 1.25;
	
	std::size_t p = 1;
	while (p < args.size()) {

		if (args[p] == "-playgames") {
			cmd = args[p];
		}

		if (args[p] == "-epd") {
			++p;
			if (p < args.size()) {
				fileepd = args[p];
			}
		}

		if (args[p] == "-threads") {
			++p;
			if (p < args.size()) {
				nbthreads = std::stoi(args[p]);
			}
		}

		if (args[p] == "-batchsize") {
			++p;
			if (p < args.size()) {
				nbbatch = std::stoi(args[p]);
			}
		}

		if (args[p] == "-display") {
			++p;
			if (p < args.size()) {
				display = std::stoi(args[p]);
			}
		}

		if (args[p] == "-stop") {
			++p;
			if (p < args.size()) {
				stop = std::stoi(args[p]);
			}
		}

		if (args[p] == "-learningrate") {
			++p;
			if (p < args.size()) {
				rate = std::stod(args[p]);
			}
		}

		if (args[p] == "-reducelearningrate") {
			++p;
			if (p < args.size()) {
				reduce = std::stod(args[p]);
			}
		}

		++p;
	}

	ChessTuner tuner(fileepd, nbthreads, nbbatch, display, stop, rate, reduce);
	auto r = (tuner.run() ? 0 : 1);

	return r;
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, const char *argv[])
{
	std::vector<std::string> args(argv, std::next(argv, argc));

	Logger::open("logtuner.txt");
	Logger::enable_level(Logger::LogLevel::info, true);
	Logger::enable_level(Logger::LogLevel::warning, true);
	Logger::enable_level(Logger::LogLevel::error, true);
	Logger::enable_level(Logger::LogLevel::debug, Logger::use_log_debug);

	int r = 0;

	if (args.size() > 1) {

		if ((args[1] == "-help") ||
			(args[1] == "--help") ||
			(args[1] == "-h")) {
			printHelp();
			return 0;
		} else
		if (args[1] == "-filter") {
			r = cmdFilter(args);
		} else 
		if (args[1] == "-pgntoepd") {
			r = cmdPGNToEPD(args);
		} else 
		if (args[1] == "-playgames") {
			r = cmdPlayGames(args);
		} else
		if (args[1] == "-tune") {
			r = cmdTuner(args);
		} else {
			return -1;
		}
		
	} else {
		return -1;
	}	
	
	Logger::info() << "End.";
	Logger::close();
	
	return r;
}

