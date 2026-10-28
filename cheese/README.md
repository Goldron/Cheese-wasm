# Cheese

Cheese is a chess engine, running as a console based application compatible with UCI and CECP protocols.

It supports multiple threads, and compiles on Windows, Linux, Mac OS, supports x86 and ARM processors.

https://cheesechess.net/

# Command line
Some tools are available on command line.
* #### cheese -testsuite HASH_SIZE NB_THREADS {depth|time} VALUE FILENAME.epd
    search all positions in filename.epd (support bm and am EPD options)
    * #### HASH_SIZE = hash table size (in Mb)
    * #### NB_THREADS = number of threads used for the search
    * #### depth VALUE = search limited by depth
    * #### time VALUE = search limited by time (seconds)

* #### cheese -perft DEPTH [-fen "FEN_STRING"]
    count all possible legal moves until the specified depth

* #### cheese -perftd DEPTH [-fen "FEN_STRING"]
    detailed version of perft (show checks, mates, captures, and en-passant, promotions and castlings)

* #### cheese -divide DEPTH [-fen "FEN_STRING"]
    count all possible legal moves until the specified depth, for all moves in the position

* #### cheese -benchmark HASH_SIZE NB_THREADS {depth|time} VALUE {FILENAME.epd}
    run a benchmark using filename.epd, or using some default positions if file is not specified
    * #### HASH_SIZE = hash table size (in Mb)
    * #### NB_THREADS = number of threads used for the search
    * #### depth VALUE = search limited by depth
    * #### time VALUE = search limited by time (seconds)

* #### cheese -analyze -fen "FEN_STRING" [-hash HASH_SIZE] [-threads NB_THREADS] [-depth DEPTH] [-time TIME] [-nodes NODES] [-multipv MULTIPV] [-exclude MOVELIST] [-mate VALUE]
	* #### HASH_SIZE ) hash table sizee (in Mb)
	* #### NB_THREADS = number of threads used for analysis
	* #### DEPTH = search limited by depth
	* #### NODES = search limited by nodes
	* #### MULTIPV = number of multipv lines to search
	* #### MOVELIST = liste of moves to exclude from search (in SAN notation)
	* #### VALUE = mate value to search

* #### cheese -probetb "FEN_STRING"
    probe tablebases for the FEN position

* #### cheese -probebook "FEN_STRING"
    probe opening book for the FEN position

# Configuration file
The configuration file cheese.ini is used to set default value for engine parameters. (some parameters are also available at run time with UCI and CECP)

* #### Hash
    Size of the main hash table (in Mb, must be a power of 2, maximum 65536)

* #### Book
    Enable opening book (0 or 1)

* #### BookFile
    Full path of the opening book, Polyglot format.

* #### UseStrength
    Enable ELO strength reduction (0 or 1)

* #### StrengthELO
    ELO rating used for strength reduction (800 to 3000)

* #### UCILimitMode
    Method used to reduce strenght (0 = reduce speed, 1 = limit nodes)

* #### UseNullMovePruning
    Use Null move pruning algorithm (0 or 1)

* #### UseLMR
    Use Late Move Reduction algorithm (0 or 1)

* #### DrawScore
    Score used as draw value (default = 0)

* #### SMPMode
    Algorithm used for multithreading. 
    ybwc = Young Brother Wait Concept, 
    lazy = Lazy SMP (Shared hashtable). 

* #### MaxThreads
    Maximum number of threads used for the search (1 to 64 on Windows, 1 to 256 on Linux, limited to 64 for YBWC)

* #### MinSplitDepth
    Minimum split depth used for YBWC (default = 4)

* #### PersonalityFile
    Full path of the personality file (look at personality/default.txt for more informations)

* #### UsePersonality
    Use a personality file to change some evaluation parameters (0 or 1)

* #### LogLevel
    A flag to set Log file level of detail (0 = disable, 1 = info, 2 = warning, 4 = error, 8 = debug, 16 = gui, 31 = all)

* #### SyzygyPath
    Full path of syzygy tablebases or &lt;empty&gt; to disable

* #### SyzygyProbeDepth
    default probe depth for Syzygy tables

* #### Syzygy50MoveRule
    Use 50 move rule for tablebases (0 or 1)

* #### SyzygyProbeLimit
    Limit proble depth for Syzygy tables

# Compilation

## Requirements

- CMake minimum version 3.15 
- A compiler with C++17 support

## Compilation
Example:
```
    mkdir build
    cd build
    cmake .. -D"USE_POPCOUNT=ON"
    cmake --build . --config Release
```
Binary will be located in the bin/ folder, (or bin\Release for Windows)

## CMake Compilation options

* #### USE_POPCOUNT
    Use hardware popcount instruction (for x86_64 and ARM64)

* #### USE_PEXT
    Use hardware PEXT instruction for magic bitboards (for x86_64)

* #### BUILD_DYNAMIC
    Build using dynamic libraries

* #### USE_TABLEBASES
    Enable Syzygy tablebases code

* #### ENABLE_TESTING
    Compile unit tests (GoogleTest must be installed)

* #### ENABLE_TUNER
	Compile the tuner application used to optimize evaluation with Texel's tuning method

* #### ENABLE_IPO
    Enable Inter Procedural Optimization

* #### ARM
    Needed when compiling for ARM

* #### ARM_THUMB
    Use thumb instructions for ARM 32 bits

* #### ARM_PIE
    Create a PIE binary for ARM build

# License
Cheese is distributed under the GNU General Public License (GPL).

# Credits

This project would not have been possible without the computer chess community, and I was specially inspired and motivated by Crafty and Stockfish engines.

Tablebases code before version 3.2 was a modified version of Syzygy by Ronald de Man.
New versions are using Fathom, by Jon Dart, basil00, and Ronald de Man.

Opening book code is based on Fabien Letousey's Polyglot book format.

The evaluation is tuned using Peter Österlund Texel's tuning method, 
Tuner code since version 3.2 is based on Andrew Grant's document "Evaluation & Tuning in Chess Engines".

Some personality parameters are inspired by Pawel Kosiol's engine Rodent.

I want to thank to the following people for their work and all the information they released about chess engine programming :

Robert M. Hyatt, Bruce Morland, Gerd Isenberg, Pradu Kannan, Grant Osborn, Jonatan Pettersson, Peter Österlund, 
Ronald de Man, Fabien Letousey, Jon Dart, Stockfish developpers, the chess programming wiki and Talkchess forum.

And a big thanks to people using and testing my engine.
