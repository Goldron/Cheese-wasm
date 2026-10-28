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

#ifndef CHEESE_TOKENIZER_H_
#define CHEESE_TOKENIZER_H_

#include "config.h"

#include <string>
#include <string_view>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
// string view tokenizer 
class Tokenizer {
	
	std::string_view str;
	
	std::size_t pos {0};

	bool more {false};
	
public:
	
	Tokenizer(std::string_view sv);
	
	[[nodiscard]]
	bool hasMoreTokens() const
	{
		return more;
	}
	
	[[nodiscard]]
	std::string_view nextToken();
	
	[[nodiscard]]
	std::string_view getCurrentString() const
	{
		return str.substr(pos);
	}
};

} // namespace Cheese

#endif // CHEESE_TOKENIZER_H_

