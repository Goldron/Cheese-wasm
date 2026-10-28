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

#include "configfile.h"

#include "util/logfile.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
void ConfigFile::clear()
{
	config.clear();
}

////////////////////////////////////////////////////////////////////////////////
auto ConfigFile::trimString(const std::string &str) const
{
	auto p1 = str.find_first_not_of(' ');
	if (p1 == std::string::npos) {
		return std::string();
	}
	auto p2 = str.find_last_not_of(' ');
	return str.substr(p1, (p2 - p1 + 1));
}

////////////////////////////////////////////////////////////////////////////////
auto ConfigFile::find(const std::string &key)
{
	return std::find_if(config.begin(), config.end(),
		[&](const auto &e) { return e.first == key; });
}

////////////////////////////////////////////////////////////////////////////////
bool ConfigFile::load(const std::string &filename)
{
	std::ifstream ifs(filename, std::ifstream::in);
	if (!ifs.is_open()) {
		return false;
	}

	std::string str;
	while (std::getline(ifs, str)) {

		// remove special characters
		str.erase(std::remove_if(str.begin(), str.end(), 
			[](unsigned char c) { return !std::isprint(c); }), str.end());

		if (str.size() > 2) {
			auto p1 = str.find_first_not_of(' ');
			if ((p1 != std::string::npos) &&
				(str[p1] != ';') && (str[p1] != '#')) {
				p1 = str.find_first_of('=');
				if (p1 != std::string::npos) {
					auto key = trimString(str.substr(0, p1));
					auto value = trimString(str.substr(p1 + 1,
						str.size() - p1 - 1));
					set(key, value);
				}
			}
		}
	}

	ifs.close();

	debug();

	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ConfigFile::save(const std::string &filename)
{
	std::ofstream ofs(filename, std::ofstream::out);
	if (!ofs.is_open()) {
		return false;
	}

	for (const auto & [name, value] : config) {
		ofs << name << '=' << value << '\n';
	}

	ofs.close();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
std::string ConfigFile::get(const std::string &key, const std::string &def)
{
	auto it = find(key);
	return (it != config.end()) ? it->second : def;
}

////////////////////////////////////////////////////////////////////////////////
int ConfigFile::getInt(const std::string &key, int def)
{
	auto it = find(key);
	if (it != config.end()) {
		if (ConfigFile::isNumber(it->second)) {
			return std::stoi(it->second);
		} else {
			Logger::error() << "wrong integer value in parameter " 
				<< key << " : " << it->second;
		}
	}
	return def;
}

////////////////////////////////////////////////////////////////////////////////
double ConfigFile::getDouble(const std::string &key, double def)
{
	auto it = find(key);
	if (it != config.end()) {
		if (ConfigFile::isDouble(it->second)) {
			return std::stod(it->second);
		} else {
			Logger::error() << "wrong double value in parameter " 
				<< key << " : " << it->second;
		}
	}
	return  def;
}

////////////////////////////////////////////////////////////////////////////////
void ConfigFile::set(const std::string &key, const std::string &value)
{
	auto it = find(key);
	if (it != config.end()) {
		it->second = value;
	} else {
		config.emplace_back(trimString(key), trimString(value));
	}
}

////////////////////////////////////////////////////////////////////////////////
bool ConfigFile::isNumber(const std::string &str)
{
	if (str.empty()) {
		return false;
	}

	if (str.size() == 1) {
		return (std::isdigit(str[0]) != 0);
	}

	for (auto i=0U; i<str.size(); ++i) {
		const auto c = str[i];
		if ((i == 0) && ((c == '-') || (c == '+'))) {
			continue;
		}
		if (std::isdigit(c) == 0) {
			return false;
		}
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////
bool ConfigFile::isDouble(const std::string &str)
{
	if (str.empty()) {
		return false;
	}

	if (str.size() == 1) {
		return (std::isdigit(str[0]) != 0);
	}

	int n = 0;
	for (auto i=0U; i<str.size(); ++i) {
		const auto c = str[i];
		if ((i == 0) && ((c == '-') || (c == '+'))) {
			continue;
		}
		if (c == '.') {
			++n;
			continue;
		}
		if (std::isdigit(c) == 0) {
			return false;
		}
	}
	return (n < 2);
}

////////////////////////////////////////////////////////////////////////////////
void ConfigFile::debug()
{
	for (const auto & [name, value] : config) {
		Logger::debug() << "'" << name << "'"
				        << " = "
				        << "'" << value << "'";
	}
}


} // namespace Cheese
