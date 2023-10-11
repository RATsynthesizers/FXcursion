/*
 * wire.hpp
 *
 *  Created on: 8 ????. 2020 ?.
 *      Author: Predtech
 */

#ifndef INC_WIRE_HPP_
#define INC_WIRE_HPP_

#include "../../Modules/libModules/Module.hpp"

class Wire {  // TODO think about wire management & reconnection
public:
	static u32 instancesNum;  // used to count how many Wires were created
	Module *srcModule;
	Module *destModule;
	int8_t inputID;

	// default k-tor
	Wire() {
		Wire::instancesNum++;
		srcModule = NULL;
		destModule = NULL;
		inputID = 0;
	}

	// single parameter modulation
	Wire(Module *srcModule_, Module *destModule_) {
		Wire::instancesNum++;
		srcModule = srcModule_;
		destModule = destModule_;
		Plug();
	}

	void Unplug(void) {
		destModule->isInputPlugged[inputID] = false;
		destModule->input[LEFT][inputID] = NULL;
		destModule->input[RIGHT][inputID] = NULL;
	}

	void Plug(void) {
		for(inputID = 0; inputID < MAX_MODULE_INPUTS; inputID++)
			if(destModule->isInputPlugged[inputID] == false)
			{
				destModule->isInputPlugged[inputID] = true;
				destModule->input[LEFT][inputID] = &srcModule->output[LEFT];
				destModule->input[RIGHT][inputID] = &srcModule->output[RIGHT];
				break;
			}
	}

	void Replug(Module *srcModule_, Module *destModule_) {
		Unplug();
		srcModule = srcModule_;
		destModule = destModule_;
		Plug();
	}

};

#endif /* INC_WIRE_HPP_ */
