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

#define _CLIP_LIMIT(x, d, u)	if((x)>(u)){(x)=(u);}else if((x)<(d)) {(x)=(d);}

class Module {
public:
	static u32 allParamNum; // used to count how many Parameters were used by all Modules in total; 0 by default
	u32 paramNum;           // num of parameters in module
	Parameter *p;           // pointer for module params to be stored to

	Module *prevModule;
	Module *nextModule;

	float output[STEREO];
	float outputVolume;

	char* name;

	Module(Parameter *p, u32 paramNum) { // feed (allocated param array addr) & (number of Module's params to allocate)
		this->paramNum = paramNum; // how many params do we have in Module (specify precisely in child class)
		this->prevModule = NULL;
		this->nextModule = NULL;
		if (Module::allParamNum + paramNum > MAX_SYNTH_PARAMS) // if MAX_SYNTH_PARAMS is not enough to store
			while (1)
				;                            // error, increase MAX_SYNTH_PARAMS
		else {
			this->p = p + Module::allParamNum; // find where to store Module params in allocated array
			Module::allParamNum += paramNum; // increase alloc array offset (to store params of the next Module if created)
			for (u32 i = 0; i < paramNum; i++)
				this->p[i].index = i;           // init parameter indexes
		}
		this->outputVolume = 1;
		for (int i = 0; i < STEREO; i++)       // let output be zeroed
			this->output[i] = 0;
	}

	void plugNext(Module *nextModule) {
		this->nextModule = nextModule;
		nextModule->plugPrev(this);
	}

	void plugPrev(Module *prevModule) {
		this->prevModule = prevModule;
	}

	void unplugNext() {
		nextModule->unplugPrev();
		this->nextModule = NULL;
	}

	void unplugPrev() {
		this->prevModule = NULL;
	}

	uint8_t getPublicParameter(uint8_t i) {
		return (this->p + i)->val * 255;
	}

	void setPublicParameter(uint8_t i, uint8_t val) {
		(this->p + i)->val = val / 255.;
	}

	virtual void process()=0;
};

#endif /* MODULE_HPP_ */
