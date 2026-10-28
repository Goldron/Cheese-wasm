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

#ifndef CHEESE_EVAL_SCORE_H_
#define CHEESE_EVAL_SCORE_H_

#include "config.h"

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
// evaluation score, depending on middle game and endgame score
class EvalScore {

public:

	using value_type = std::int32_t;

	// middle game score
	value_type mid {0};

	// end game score
	value_type end {0};

	constexpr EvalScore() = default;

	constexpr EvalScore(value_type m, value_type e) : mid(m), end(e)
	{ }

	constexpr EvalScore & operator=(const value_type &v)
	{
		mid = v;
		end = v;
		return *this;
	}

	// interpolate mid and end score to get the real score
	[[nodiscard]]
	constexpr int interpolate(int p, int max) const
	{
		return (mid * p + end * (max - p)) / max;
	}

	constexpr EvalScore & operator+= (const EvalScore &s)
	{
		mid += s.mid;
		end += s.end;
		return *this;
	}

	constexpr EvalScore & operator-= (const EvalScore &s)
	{
		mid -= s.mid;
		end -= s.end;
		return *this;
	}

	constexpr EvalScore & operator+= (int v)
	{
		mid += v;
		end += v;
		return *this;
	}

	constexpr EvalScore & operator-= (int v)
	{
		mid -= v;
		end -= v;
		return *this;
	}

	constexpr EvalScore & operator*= (int v)
	{
		mid *= v;
		end *= v;
		return *this;
	}

	constexpr EvalScore & operator/= (int v)
	{
		mid /= v;
		end /= v;
		return *this;
	}
};

constexpr EvalScore operator+(const EvalScore &a, const EvalScore &b)
{
	return { a.mid + b.mid, a.end + b.end };
}

constexpr EvalScore operator-(const EvalScore &a, const EvalScore &b)
{
	return { a.mid - b.mid, a.end - b.end };
}

constexpr EvalScore operator-(const EvalScore &e)
{
	return { -e.mid, -e.mid };
}

constexpr EvalScore operator*(const EvalScore &e, int v)
{
	return { e.mid * v, e.end * v };
}

constexpr EvalScore operator/(const EvalScore &e, int v)
{
	return { e.mid / v, e.end / v };
}

} // namespace Cheese

#endif //CHEESE_EVAL_SCORE_H_

