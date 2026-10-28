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

#ifndef CHEESE_BITFIELDS_H_
#define CHEESE_BITFIELDS_H_

#include <cinttypes>
#include <limits>
#include <type_traits>

namespace Cheese {

////////////////////////////////////////////////////////////////////////////////
// portable bit fields
template<typename T, unsigned int Tshift, unsigned int Tsize>
class BitField {

	static_assert(std::is_integral_v<T>);
	static_assert(std::is_unsigned_v<T>);
	static_assert(Tsize != 0);
	static_assert(Tshift <= std::numeric_limits<T>::digits);
	static_assert((Tshift + Tsize) <= std::numeric_limits<T>::digits);

public:

	// encode value
	template<typename Tval>
	[[nodiscard]]
	static constexpr T encode(Tval v)
	{			
		return (static_cast<T>(v) & mask()) << Tshift;
	}

	// set the value
	template<typename Tval>
	static constexpr void set(T &b, Tval v)
	{
		clear(b);
		b |= encode(v);
	}

	// get the value
	template<typename Tval>
	[[nodiscard]]
	static constexpr Tval get(const T &b)
	{
		return static_cast<Tval>((b >> Tshift) & mask());
	}
	
	// test if value is not zero
	[[nodiscard]]
	static constexpr bool test(const T &b)
	{
		return (b & (mask() << Tshift)) != 0;
	}

	// clear bits
	static constexpr void clear(T &b)
	{
		b &= ~(mask() << Tshift);
	}

	[[nodiscard]]
	static constexpr T mask()
	{
		return static_cast<T>((1 << Tsize) - 1);
	}
};

} // namespece Cheese

#endif //CHEESE_BITFIELDS_H_

