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

#include "engine.h"
#include "module/module.h"
#include "timer.h"

#include <vector>
#include <string>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

Cheese::CmdQueue *gCmdQueue = nullptr;

EMSCRIPTEN_KEEPALIVE 
extern "C" void processCommand(const char *msg)
{
	if ((gCmdQueue != nullptr) && (msg != nullptr)) {
		gCmdQueue->push(std::string(msg));
	}
}

#endif //__EMSCRIPTEN__

////////////////////////////////////////////////////////////////////////////////
#ifndef __EMSCRIPTEN__
int main(int argc, const char *argv[])
{
	std::vector<std::string> args(argv, std::next(argv, argc));
	
	Cheese::ChessModule mainModule;

	bool r = mainModule.init(args);
	if (r) {
		mainModule.run();
	}

	mainModule.free();

	return (r) ? 0 : 1;
}
#else
int main()
{
	Cheese::ChessModule mainModule;

	mainModule.initModuleEmscripten();
	gCmdQueue = mainModule.getCmdQueue();

	mainModule.run();

	mainModule.free();

	return 0;
}
#endif


