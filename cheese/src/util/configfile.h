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

#ifndef CHEESE_CONFIGFILE_H_
#define CHEESE_CONFIGFILE_H_

#include <string>
#include <utility>
#include <vector>

namespace Cheese {

using ConfigFileEntry = std::pair<std::string, std::string>;

////////////////////////////////////////////////////////////////////////////////
// configuration file
// format :
//			# comment
//			; another comment
//			key = value
class ConfigFile {

	// note: why use a map when a vector do the job
	std::vector<ConfigFileEntry> config;

	[[nodiscard]] 
	auto trimString(const std::string &str) const;

public:

	ConfigFile() = default;

	void clear();

	[[nodiscard]] 
	auto find(const std::string &key);

	bool load(const std::string &filename);

	bool save(const std::string &filename);

	[[nodiscard]] 
	std::string get(const std::string &key, const std::string &def = 
		std::string());

	[[nodiscard]] 
	int getInt(const std::string &key, int def=0);

	[[nodiscard]] 
	double getDouble(const std::string &key, double def=0.0);

	void set(const std::string &key, const std::string &value);

	[[nodiscard]] 
	auto size() const;
	
	[[nodiscard]] 
	auto begin() const;

	[[nodiscard]] 
	auto end() const;

	void debug();

	// test if the string contains a number
	static bool isNumber(const std::string &str);

	// test if the string contains a double
	static bool isDouble(const std::string &str);
};

////////////////////////////////////////////////////////////////////////////////
inline auto ConfigFile::size() const
{
	return config.size();
}

////////////////////////////////////////////////////////////////////////////////
inline auto ConfigFile::begin() const
{
	return config.cbegin();
}

////////////////////////////////////////////////////////////////////////////////
inline auto ConfigFile::end() const
{
	return config.cend();
}

} // namespace Cheese

#endif // CHEESE_CONFIGFILE_H_
