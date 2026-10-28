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

add_executable(cheese_app
	main.cpp
	resource.h
	cheese.rc
)

target_include_directories(cheese_app PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(cheese_app PRIVATE ${PROJECT_SOURCE_DIR}/src/util)
target_include_directories(cheese_app PRIVATE ${PROJECT_SOURCE_DIR}/src/module)
target_include_directories(cheese_app PRIVATE ${PROJECT_SOURCE_DIR}/src/fathom)
target_include_directories(cheese_app PRIVATE ${PROJECT_SOURCE_DIR}/src/polyglot)
target_include_directories(cheese_app PRIVATE ${PROJECT_SOURCE_DIR}/src/pgn)

target_link_libraries(cheese_app PRIVATE
					  Threads::Threads
					  cheese_lib
					  project_options)

# set executable name
set(RELEASE_NAME "${PROJECT_NAME}-")
set(RELEASE_NAME "${RELEASE_NAME}${PROJECT_VERSION_MAJOR}")
set(RELEASE_NAME "${RELEASE_NAME}${PROJECT_VERSION_MINOR}")
set(RELEASE_NAME "${RELEASE_NAME}${PROJECT_VERSION_PATCH}")

if (NOT EMSCRIPTEN)

if (WIN32)
	set(RELEASE_NAME "${RELEASE_NAME}-win")
endif()

if (UNIX AND NOT APPLE AND NOT ARM AND NOT RASPBERRYPI)
	set(RELEASE_NAME "${RELEASE_NAME}-linux")
endif()

if (APPLE)
	set(RELEASE_NAME "${RELEASE_NAME}-mac")
endif()

if (ARM AND NOT RASPBERRYPI)
	set(RELEASE_NAME "${RELEASE_NAME}-arm")
endif()

if (RASPBERRYPI)
	set(RELEASE_NAME "${RELEASE_NAME}-rpi")
endif()

if (USE_PEXT)
	set(RELEASE_NAME "${RELEASE_NAME}-pext")
elseif (USE_POPCOUNT)
	set(RELEASE_NAME "${RELEASE_NAME}-pop")
endif()

set(RELEASE_NAME "${RELEASE_NAME}$<$<CONFIG:Debug>:-debug>")

endif()

set_target_properties(cheese_app PROPERTIES OUTPUT_NAME "${RELEASE_NAME}")

set_target_properties(cheese_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
	"${PROJECT_SOURCE_DIR}/bin")

if (EMSCRIPTEN)
set_target_properties(cheese_app PROPERTIES LINK_FLAGS "-sASSERTIONS=0 -sSTACK_SIZE=65536 -sPTHREAD_POOL_SIZE=1 -pthread -sPROXY_TO_PTHREAD=1 -sINITIAL_MEMORY=64MB -sALLOW_MEMORY_GROWTH=0 -sMODULARIZE=1 -sDISABLE_EXCEPTION_CATCHING=1 -sEXPORT_ES6=1 -sALLOW_BLOCKING_ON_MAIN_THREAD=0 -sSTRICT -sWASM=1 -sENVIRONMENT=web,worker -sEXPORT_NAME=Cheese --closure 1 -sEXIT_RUNTIME=1 -sFILESYSTEM=0 -sINCOMING_MODULE_JS_API=\"[print, printErr]\" -s\"EXPORTED_RUNTIME_METHODS=['ccall']\" --pre-js ${PROJECT_SOURCE_DIR}/js/preamble.js")
endif()

