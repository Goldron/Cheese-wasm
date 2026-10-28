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

#ifndef CHEESE_CONFIG_H_
#define CHEESE_CONFIG_H_

// Config & compilation options

#include "makeconfig.h"

#include <cinttypes>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
// prevent windows.h to declare min & max macros
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// detect 64 bits
#if defined(_WIN64) || defined(_LP64)
#define SYSTEM64BIT
#endif

#endif //CHEESE_CONFIG_H_
