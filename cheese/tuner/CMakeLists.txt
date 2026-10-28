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

add_executable(cheese_tuner
	main.cpp	
	pgnselect.cpp
	playgames.cpp
	tuner.cpp

	pgnselect.h
	playgames.h
	tuner.h
)

target_include_directories(cheese_tuner PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_include_directories(cheese_tuner PRIVATE ${PROJECT_SOURCE_DIR}/src/util)
target_include_directories(cheese_tuner PRIVATE ${PROJECT_SOURCE_DIR}/src/module)
target_include_directories(cheese_tuner PRIVATE ${PROJECT_SOURCE_DIR}/src/fathom)
target_include_directories(cheese_tuner PRIVATE ${PROJECT_SOURCE_DIR}/src/polyglot)
target_include_directories(cheese_tuner PRIVATE ${PROJECT_SOURCE_DIR}/src/pgn)

target_link_libraries(cheese_tuner PRIVATE
					  Threads::Threads
					  cheese_lib
					  project_options)

set_target_properties(cheese_tuner PROPERTIES RUNTIME_OUTPUT_DIRECTORY 
	"${PROJECT_SOURCE_DIR}/bin")
