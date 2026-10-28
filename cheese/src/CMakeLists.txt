# Cheese, Chess engine.
#
# Copyright (C) 2006-2025 Patrice Duhamel. All rights reserved.
#
# Cheese is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Cheese is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

set(PROJECT_SOURCES
	bitboard.cpp
	bitbase.cpp
	board.cpp
	endgame.cpp
	engine.cpp
	eval.cpp
	fen.cpp
	hash.cpp
	history.cpp
	lazysmp.cpp
	listener.cpp
	movelist.cpp
	movegen.cpp
	moveorder.cpp
	moveparser.cpp
	pv.cpp
	repetitions.cpp
	search.cpp
	searchthread.cpp
	tablebases.cpp
	tools.cpp
	ybwc.cpp
	zobrist.cpp

	board.h
	bitbase.h
	bitboard.h	
	config.h
	endgame.h
	engine.h
	eval.h
	evalscore.h
	fen.h
	hash.h
	history.h
	lazysmp.h
	listener.h
	makeconfig.h
	material.h
	move.h
	movelist.h
	movegen.h
	moveorder.h
	moveparser.h
	pv.h
	repetitions.h
	search.h
	searchthread.h
	tablebases.h
	tools.h
	tune.h
	ybwc.h
	zobrist.h

	util/bitfield.h
	util/configfile.cpp
	util/logfile.cpp
	util/timer.cpp
	util/tokenizer.cpp
	util/configfile.h
	util/logfile.h
	util/timer.h
	util/tokenizer.h
	util/xorshiftstar.h

	module/protocoluci.cpp
	module/protocolcecp.cpp
	module/module.cpp
	module/protocol.h
	module/protocoluci.h
	module/protocolcecp.h
	module/module.h

	polyglot/book.cpp
	polyglot/book.h

	pgn/pgn.cpp
	pgn/pgn.h
)

if (USE_TABLEBASES)
	set(PROJECT_SOURCES 
		${PROJECT_SOURCES}
		fathom/tbconfig.h
		fathom/stdendian.h
		fathom/tbprobe.h
		fathom/tbprobe.c
	)
endif()

add_library(cheese_lib ${PROJECT_SOURCES})

# force fathom to be compiled as C++
if (USE_TABLEBASES)
	set_source_files_properties("fathom/tbprobe.c" PROPERTIES LANGUAGE CXX)
endif()

target_include_directories(cheese_lib PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(cheese_lib PRIVATE ${PROJECT_SOURCE_DIR}/src/util)
target_include_directories(cheese_lib PRIVATE ${PROJECT_SOURCE_DIR}/src/module)
target_include_directories(cheese_lib PRIVATE ${PROJECT_SOURCE_DIR}/src/fathom)
target_include_directories(cheese_lib PRIVATE ${PROJECT_SOURCE_DIR}/src/polyglot)
target_include_directories(cheese_lib PRIVATE ${PROJECT_SOURCE_DIR}/src/pgn)

target_link_libraries(cheese_lib PRIVATE
					  Threads::Threads
					  project_options)

