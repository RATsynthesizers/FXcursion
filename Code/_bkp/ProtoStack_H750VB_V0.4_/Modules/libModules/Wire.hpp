/*
 * wire.hpp
 *
 *  Created on: 8 ????. 2020 ?.
 *      Author: Predtech
 */

#ifndef INC_WIRE_HPP_
#define INC_WIRE_HPP_

#include "Module.hpp"

class Wire {  // TODO think about wire management & reconnection
public:
	static u32 instancesNum;  // used to count how many Wires were created
	Module *srcModule;
	Module *destModule;

	// default k-tor
	Wire() {
		Wire::instancesNum++;
		srcModule = NULL;
		destModule = NULL;
	}

	// single parameter modulation
	Wire(Module *srcModule_, Module *destModule_) {
		Wire::instancesNum++;
		srcModule = srcModule_;
		destModule = destModule_;
		Plug();
	}

	void Unplug(void) {
		destModule->input[LEFT] = NULL;
		destModule->input[RIGHT] = NULL;
	}

	void Plug(void) {
		destModule->input[LEFT] = &srcModule->output[LEFT];
		destModule->input[RIGHT] = &srcModule->output[RIGHT];

	}

	void Replug(Module *srcModule_, Module *destModule_) {
		Unplug();
		srcModule = srcModule_;
		destModule = destModule_;
		Plug();
	}

};

#endif /* INC_WIRE_HPP_ */
