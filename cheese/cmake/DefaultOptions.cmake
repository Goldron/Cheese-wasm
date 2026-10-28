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

add_library(project_options INTERFACE)

target_compile_features(project_options INTERFACE cxx_std_17)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Build type not specified, using Release as default")
	set(CMAKE_BUILD_TYPE Release)
endif()

# export compile commands for clang tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (ENABLE_IPO)
	include(CheckIPOSupported)
 	check_ipo_supported(RESULT result OUTPUT output)
 	if (result)
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
	else()
		message(STATUS "Interprocedural optimizations not supported")
	endif()
endif()

if (WIN32)
	target_compile_definitions(project_options INTERFACE WIN32)
endif()

# limit optimizations to reduce size of emscripten binary
set(OPTLEVEL "-O3")
if (EMSCRIPTEN)
	set(OPTLEVEL "-O2")
endif()

if (MSVC)

	# compiler options for Microsoft Visual Studio

	# doesn't work ?
	#set_property(TARGET project_options PROPERTY MSVC_RUNTIME_LIBRARY
	#	"MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<BOOL:BUILD_DYNAMIC>:DLL>")

	if (BUILD_DYNAMIC)
		target_compile_options(project_options INTERFACE
			$<$<CONFIG:Release>:/MD>
			$<$<CONFIG:Debug>:/MDd>
		)
	else()
		target_compile_options(project_options INTERFACE
			$<$<CONFIG:Release>:/MT>
			$<$<CONFIG:Debug>:/MTd>
		)
	endif()

	target_compile_options(project_options INTERFACE
		/favor:blend /nologo
		/W4 /WX-
		$<$<CONFIG:Release>:
			/Ox /GS- /Gw>

		$<$<BOOL:${ENABLE_IPO}>: /GL>
	)

	target_link_options(project_options INTERFACE
		$<$<CONFIG:Release>:
			/OPT:REF>

		$<$<BOOL:${ENABLE_IPO}>: /INCREMENTAL:NO /LTCG>
	)

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")

	# compiler options for Clang

	target_compile_options(project_options INTERFACE
		-pipe
		-Wall -Wextra -Wshadow -Wunused -Wnon-virtual-dtor -pedantic -pthread
		$<$<CONFIG:Release>:
			${OPTLEVEL} -fomit-frame-pointer>
		$<$<BOOL:${USE_POPCOUNT}>:
			$<$<NOT:$<BOOL:${ARM}>>:
				-mpopcnt>>
		$<$<BOOL:${USE_PEXT}>:
			-mbmi2>
		$<$<BOOL:${ARM_THUMB}>:
			-mthumb>
		$<$<BOOL:${ARM_PIE}>:
			-fPIE>
	)

	target_link_options(project_options INTERFACE
		$<$<NOT:$<BOOL:${BUILD_DYNAMIC}>>:
			-static -static-libgcc -static-libstdc++
			-Wl,--whole-archive -lpthread -Wl,--no-whole-archive>
		$<$<BOOL:${ARM_PIE}>:
			-fPIE -pie>
		$<$<CONFIG:Release>: -s>
	)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

	# compiler options for GNU gcc or MinGW gcc

	target_compile_options(project_options INTERFACE
		-pipe
		-Wall -Wextra -pedantic -Wshadow -Wunused -pthread 
		$<$<CONFIG:Release>:
			-O3 -fomit-frame-pointer>
		$<$<BOOL:${USE_POPCOUNT}>:
			$<$<NOT:$<BOOL:${ARM}>>:
				-mpopcnt>>
		$<$<BOOL:${USE_PEXT}>:
			-mbmi2>
		$<$<BOOL:${ARM_THUMB}>:
			-mthumb>
		$<$<BOOL:${ARM_PIE}>:
			-fPIE>
	)

	target_link_options(project_options INTERFACE
		$<$<NOT:$<BOOL:${BUILD_DYNAMIC}>>:
			-static -static-libgcc -static-libstdc++
			-Wl,--whole-archive -lpthread -Wl,--no-whole-archive>
		$<$<BOOL:${BUILD_DYNAMIC}>:
			-no-pie>
		$<$<BOOL:${ARM_PIE}>:
			-fPIE -pie>
		$<$<CONFIG:Release>: -s>
	)

else()
	message(STATUS "Compiler not detected, using default options")
endif()

# sanitizers
set(SANITIZERS "")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR 
   CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if (ENABLE_SANITIZER_ADDRESS)
		list(APPEND SANITIZERS "address")
	endif()
	if (ENABLE_SANITIZER_LEAK)
		list(APPEND SANITIZERS "leak")		
	endif()
	if (ENABLE_SANITIZER_UNDEFINED)
		list(APPEND SANITIZERS "undefined")		
	endif()
	if (ENABLE_SANITIZER_MEMORY)
		if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			list(APPEND SANITIZERS "memory")		
		endif()
	endif()
	if (ENABLE_SANITIZER_THREAD)
		list(APPEND SANITIZERS "thread")
	endif()
endif()

if (MSVC)
	if (ENABLE_SANITIZER_ADDRESS OR
	   ENABLE_SANITIZER_LEAK OR
	   ENABLE_SANITIZER_UNDEFINED OR
	   ENABLE_SANITIZER_MEMORY OR
	   ENABLE_SANITIZER_THREAD)
		message(STATUS "Sanitizers are not supported with MSVC")
	endif()
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR 
   CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	list(JOIN SANITIZERS "," LIST_SANITIZERS)
	if (LIST_SANITIZERS)
		if (NOT "${LIST_SANITIZERS}" STREQUAL "")
			target_compile_options(project_options INTERFACE
				-fsanitize=${LIST_SANITIZERS}
			)
			target_link_options(project_options INTERFACE
				-fsanitize=${LIST_SANITIZERS}
			)
		endif()
	endif()
endif()

