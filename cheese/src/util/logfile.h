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

#ifndef CHEESE_LOG_H_
#define CHEESE_LOG_H_

#include "config.h"

#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Cheese::Logger {

// enable logging
//#ifndef __EMSCRIPTEN__
inline constexpr bool use_log = true;
//#else
//inline constexpr bool use_log = false;
//#endif

// enable debug logging level
#ifdef DEBUG
inline constexpr bool use_log_debug = true;
#else
inline constexpr bool use_log_debug = false;
#endif

class LogStream;
class LogStreamNull;

using LogStreamType = std::conditional_t<use_log,
					  LogStream,
					  LogStreamNull>;

using LogStreamTypeDebug = std::conditional_t<use_log_debug,
						   LogStream,
						   LogStreamNull>;

////////////////////////////////////////////////////////////////////////////////
enum class LogLevel {
	info,
	warning,
	error,
    debug,
	// start of custom levels
	custom
};

////////////////////////////////////////////////////////////////////////////////
struct LogLevelInfo {

	// level name
	std::string name;

	// enable for output
	bool enable {false};

	LogLevelInfo() = default;

	LogLevelInfo(const std::string &n, bool e) : name(n), enable(e)
	{ }
};

class LogStream;

////////////////////////////////////////////////////////////////////////////////
class LogFile {

    std::ofstream ofs;

	std::mutex mutex;

    std::vector<LogLevelInfo> levels;

    bool uselog {false};	

public:

    LogFile();

	[[nodiscard]]
	auto &getStream();

    void write(LogLevel level, const std::string &str);

    void enable(bool e);

	[[nodiscard]]
	bool isEnable() const;

	[[nodiscard]]
    bool isLevelEnable(LogLevel level) const;

	void enableLevel(LogLevel level, bool b);

    // open log file
    bool open(const std::string &name);

    // close logfile
    void close();

	[[nodiscard]]
    LogLevel addCustomLevel(const std::string &name);

};

////////////////////////////////////////////////////////////////////////////////
// Temporary stream for LogFile
// to be able to catch the end of the stream when using multiple <<
class LogStream {

    std::ostringstream sstr;

    LogFile &logger;

    LogLevel level;

public:

    explicit LogStream(LogFile &log, LogLevel lev = LogLevel::info) : 
		logger(log), level(lev) 
	{}

    ~LogStream() 
	{
		// write to log before deleting stream
		logger.write(level, sstr.str());
	}

	template<typename T>
	auto & operator<<(const T &value)
	{
		sstr << value;
		return (*this);
	}
};

////////////////////////////////////////////////////////////////////////////////
// Null LoginStream to allow compiler to remove all debug logging code
class LogStreamNull {

public:

    explicit LogStreamNull([[maybe_unused]] LogFile &log, 
		[[maybe_unused]] LogLevel lev = LogLevel::info) 
	{}

	template<typename T>
	auto & operator<<([[maybe_unused]] const T &value)
	{
		return (*this);
	}
};

////////////////////////////////////////////////////////////////////////////////
inline auto &LogFile::getStream()
{
	return ofs;
}

////////////////////////////////////////////////////////////////////////////////
inline void LogFile::enable(bool e)
{
	uselog = e;
}

////////////////////////////////////////////////////////////////////////////////
inline bool LogFile::isEnable() const
{
	return uselog;
}

////////////////////////////////////////////////////////////////////////////////
inline bool LogFile::isLevelEnable(LogLevel level) const
{
	return uselog &&
		   levels[static_cast<std::size_t>(level)].enable;
}

////////////////////////////////////////////////////////////////////////////////
inline void LogFile::enableLevel(LogLevel level, bool b)
{
	levels[static_cast<std::size_t>(level)].enable = b;
}

extern LogFile g_log;

////////////////////////////////////////////////////////////////////////////////
// Helper functions :

inline auto logger(LogLevel id = LogLevel::info)
{
	return LogStreamType(g_log, id);
}

inline auto info()
{
	return logger();
}

inline auto warning()
{
	return logger(LogLevel::warning);
}

inline auto error()
{
	return logger(LogLevel::error);
}

inline auto debug()
{
	return LogStreamTypeDebug(g_log, LogLevel::debug);
}

[[nodiscard]]
inline auto open(const std::string &name)
{
	g_log.open(name);
}

inline void close()
{
	g_log.close();
}

[[nodiscard]]
inline auto add_custom_level(const std::string &name)
{
	return g_log.addCustomLevel(name);
}

inline void enable(bool b)
{
	g_log.enable(b);
}

inline void enable_level(LogLevel id, bool b)
{
	g_log.enableLevel(id, b);
}

} // namespace Cheese::Logger

#endif // CHEESE_LOG_H_
