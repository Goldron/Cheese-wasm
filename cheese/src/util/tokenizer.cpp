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

#include "tokenizer.h"

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
Tokenizer::Tokenizer(std::string_view sv) : str(sv)
{
	pos = str.find_first_not_of(' ', pos);
	more = (pos != std::string_view::npos); 
}

////////////////////////////////////////////////////////////////////////////////
std::string_view Tokenizer::nextToken()
{
	auto p1 = pos;
	auto p2 = str.find_first_of(' ', p1);
	pos = str.find_first_not_of(' ', p2);
	more = (pos != std::string_view::npos); 
	return str.substr(p1, (p2 != std::string_view::npos) ? (p2 - p1) : p2);
}

} // namespace Cheese

