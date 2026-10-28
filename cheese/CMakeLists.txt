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

cmake_minimum_required(VERSION 3.15)

project(cheese
	    VERSION 3.3.2
		DESCRIPTION "Chess engine"
		LANGUAGES CXX)

option(USE_POPCOUNT   "Use hardware popcount instruction"      on)
option(USE_PEXT       "Use hardware PEXT instruction"          off)
option(USE_TABLEBASES "Use Syzygy tablebases"                  on)
option(BUILD_DYNAMIC  "Build with dynamic libraries"           on)
option(ENABLE_IPO     "Enable Interprocedural Optimizations"   on)
option(ARM 		      "Options needed when building for ARM"   off)
option(ARM_PIE        "Use PIE for ARM"                        off)
option(ARM_THUMB      "Use thumb instructions for ARM 32 bits" off)

option(ENABLE_TUNER   "Build Tuner code" 	                   off)

option(ENABLE_TESTING "Enable unit tests"                      off)

option(ENABLE_SANITIZER_ADDRESS   "Enable address sanitizer"   off)
option(ENABLE_SANITIZER_LEAK      "Enable leak sanitizer"      off)
option(ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" off)
option(ENABLE_SANITIZER_MEMORY    "Enable memory sanitizer"    off)
option(ENABLE_SANITIZER_THREAD    "Enable thread sanitizer"    off)

# set compilers options for all targets
include(cmake/DefaultOptions.cmake)

configure_file (
	"${PROJECT_SOURCE_DIR}/makeconfig.h.in"
	"${PROJECT_SOURCE_DIR}/src/makeconfig.h"
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_subdirectory(src)

add_subdirectory(app)

if (ENABLE_TUNER)
	add_subdirectory(tuner)
endif()

if (ENABLE_TESTING)
	enable_testing()
	add_subdirectory(tests)
endif()

