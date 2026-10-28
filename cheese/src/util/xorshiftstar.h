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

#ifndef CHEESE_XORSHIFTSTAR_H_
#define CHEESE_XORSHIFTSTAR_H_

#include <cstdint>

namespace Cheese::Random {

////////////////////////////////////////////////////////////////////////////////
// XorShift* random generator 64 bits implementation
class XorShiftStar64 {

	using data_t = std::uint64_t;

    inline static constexpr data_t magic = 0x2545F4914F6CDD1D;

    data_t state;

public:

	// seed must not be zero
	constexpr explicit XorShiftStar64(data_t seed = 1) : state(seed)
	{ }

	[[nodiscard]]
    constexpr data_t operator()() {
        auto x = state;
        x = x ^ (x >> 12);
        x = x ^ (x << 25);
        x = x ^ (x >> 27);
        state = x;        
		return x * magic;
    }
};

}  // namespace Cheese::Random

#endif //CHEESE_XORSHIFTSTAR_H_

