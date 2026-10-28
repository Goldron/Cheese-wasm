(function() {

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

// Preamble code for emscripten version

var listeners = [];

// add listener to get engine output
Module['addMessageListener'] = function(listener)
{
	if (typeof listener === 'function') {
		listeners.push(listener);
	}
};

// remove listener
Module['removeMessageListener'] = function(listener)
{
	if (typeof listener === 'function') {
		const i = listeners.indexOf(listener);
		if (i >= 0) {	
			listeners.splice(i, 1);
		}
	}
}

// process UCI command
Module['processCommand'] = function(msg) 
{
	Module['ccall']('processCommand', 'number', ['string'], [ msg ]);
};

// stdout output
Module['print'] = function(msg)
{
	if (listeners.length !== 0) {
		listeners.forEach((listener) => {
			listener(msg);
		});
	} else {
		console.log(msg);
	}
}

// stderr output
Module['printErr'] = function(msg)
{
	if (listeners.length !== 0) {
		listeners.forEach((listener) => {
			listener(msg);
		});
	} else {
		console.error(msg);
	}
}

})();


