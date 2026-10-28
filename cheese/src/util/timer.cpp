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

#include "timer.h"

#include <iomanip>
#include <sstream>
#include <thread>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
Timer::Timer()
{
	tstart = getTick();
}

////////////////////////////////////////////////////////////////////////////////
Timer::TimePoint Timer::getTick()
{
	return std::chrono::steady_clock::now();
}

////////////////////////////////////////////////////////////////////////////////
void Timer::sleep(unsigned int ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

////////////////////////////////////////////////////////////////////////////////
std::string Timer::timeToString(std::uint64_t t)
{
	static constexpr std::uint64_t one_sec = 1000;
	static constexpr std::uint64_t one_min = 60 * one_sec;
	static constexpr std::uint64_t one_hour = 60 * one_min;

	auto tt = t;
	auto th = tt / one_hour;
	tt -= th * one_hour;
    auto tm = tt / one_min;
    tt -= tm * one_min;
    auto ts = tt / one_sec;
    tt -= ts * one_sec;
    auto tms = tt;

	std::stringstream ss;
	ss << std::setfill('0') << std::setw(2) << th << ':'
	   << std::setfill('0') << std::setw(2) << tm << ':'
	   << std::setfill('0') << std::setw(2) << ts << '.'
	   << std::setfill('0') << std::setw(3) << tms;
	return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
void Timer::start()
{
	tstart = getTick();
}

////////////////////////////////////////////////////////////////////////////////
std::uint64_t Timer::getElapsedTime() const
{
	auto tend = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>
			(tend - tstart).count();
}

////////////////////////////////////////////////////////////////////////////////
std::uint64_t Timer::getCurrentTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace Cheese
