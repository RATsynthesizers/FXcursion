/*
 * Module.hpp
 *
 *  Created on: Nov 10, 2020
 *      Author: romte
 */

#ifndef MODULE_HPP_
#define MODULE_HPP_


#include <Handy.h>
#include <stddef.h>

#include "Parameter.hpp"

class Module {
public:
	static u32 instancesNum; // used to count how many Modules were created; 0 by default
	static u32 allParamNum;  // used to count how many Parameters were used by all Modules in total; 0 by default
	u32 paramNum;            // num of parameters in module
	Parameter * p;           // pointer for module params to be stored to
	float output[STEREO];
	float * input[STEREO];

	Module(Parameter *p, u32 paramNum) {   // feed (allocated param array addr) & (number of Module's params to allocate)
		instancesNum++;                    // new instance created
		this->paramNum = paramNum;         // how many params do we have in Module (specify precisely in child class)
		if (Module::allParamNum + paramNum >= MAX_SYNTH_PARAMS) // if MAX_SYNTH_PARAMS is not enough to store
			while (1);                                    // error, increase MAX_SYNTH_PARAMS
		else {
			this->p = p + Module::allParamNum; // find where to store Module params in allocated array
			Module::allParamNum += paramNum;   // increase alloc array offset (to store params of the next Module if created)
			for(u32 i=0;i<paramNum;i++)
				this->p[i].index = i;           // init parameter indexes
		}
		for (int i = 0; i < STEREO; i++) {       // let in & output be zeroed
			this->input[i] = NULL;
			this->output[i] = 0;
		}
	}

	virtual void process()=0;
//	virtual void printInfo()=0; // send module & its params & other info to Interface MCU TODO stringify

};


#endif /* MODULE_HPP_ */
