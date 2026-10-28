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

#ifndef CHEESE_TUNE_H_
#define CHEESE_TUNE_H_

#include "config.h"

#include "evalscore.h"
#include "move.h"

#include <array>
#include <string>

namespace Cheese {

using EvalTerm = std::array<std::int32_t, nb_sides>;

////////////////////////////////////////////////////////////////////////////////
struct Tunable {

	// name of parameter
	std::string name;

	// reference to evaluation score
	EvalScore &value;

	// reference to term
	EvalTerm &term;

	Tunable(const std::string &vname, EvalScore &v, EvalTerm &t) : 
		name(vname), value(v), term(t)
	{ }
};

} // namespace Cheese

#endif //CHEESE_TUNE_H_

