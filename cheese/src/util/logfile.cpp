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

#include "logfile.h"

#include <chrono>
#include <ctime>
#include <iomanip>

namespace Cheese::Logger {

// global logfile
LogFile g_log;

///////////////////////////////////////////////////////////////////////////////
LogFile::LogFile()
{
	levels.emplace_back("INFO", true);
	levels.emplace_back("WARNING", true);
	levels.emplace_back("ERROR", true);
	levels.emplace_back("DEBUG", true);
}

///////////////////////////////////////////////////////////////////////////////
bool LogFile::open(const std::string &name)
{
	uselog = false;
	if constexpr (use_log) {
		ofs.open(name, std::ofstream::out);
	    uselog = ofs.is_open();
	}	
	return uselog;
}

///////////////////////////////////////////////////////////////////////////////
void LogFile::close()
{
	if constexpr (use_log) {
		if (ofs.is_open()) {
			ofs.close();
		}
	}
	uselog = false;
}

///////////////////////////////////////////////////////////////////////////////
LogLevel LogFile::addCustomLevel(const std::string &name)
{
	levels.emplace_back(name, true);
	return static_cast<LogLevel>(levels.size() - 1);
}

///////////////////////////////////////////////////////////////////////////////
void LogFile::write(LogLevel level, const std::string &str)
{
	if constexpr (use_log) {
		if (isLevelEnable(level)) {
			std::lock_guard<std::mutex> lk(mutex);

			auto now = std::chrono::system_clock::now();
			auto te = now.time_since_epoch();
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>
				(te - std::chrono::duration_cast<std::chrono::seconds>(te));

			auto tc = std::chrono::system_clock::to_time_t(now);

			std::tm ltm{};
			#ifdef WIN32
			localtime_s(&ltm, &tc);
			#else
			localtime_r(&tc, &ltm);
			#endif

			ofs << std::put_time(&ltm, "%Y/%m/%d %H:%M:%S")
				<< '.' << std::setfill('0') << std::setw(3) << ms.count()
				<< ' '
				<< levels[static_cast<int>(level)].name
				<< ' '
				<< str
				<< std::endl;
		}
	}
}

} // namespace Cheese::Logger

