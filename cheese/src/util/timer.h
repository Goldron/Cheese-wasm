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

#ifndef CHEESE_TIMER_H_
#define CHEESE_TIMER_H_

#include "config.h"

#include <chrono>
#include <string>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
// portable timer class
class Timer {

	using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

	TimePoint tstart;

public:

	Timer();

	~Timer() = default;

	Timer(const Timer &) = default;

	Timer &operator=(const Timer &) = default;

	// convert elapsed time in milliseconds to string "HH:MM:SS.MS"
	[[nodiscard]]
	static std::string timeToString(std::uint64_t t);

	// get current time tick
	[[nodiscard]]
	static TimePoint getTick();

	// sleep for N milliseconds
	static void sleep(unsigned int ms);

	// init start time
	void start();

	// get elapsed time since start in milliseconds
	[[nodiscard]]
	std::uint64_t getElapsedTime() const;

	// get current clock time in milliseconds
	[[nodiscard]]
	static std::uint64_t getCurrentTime();
};

} // namespace Cheese


#endif //CHEESE_TIMER_H_
