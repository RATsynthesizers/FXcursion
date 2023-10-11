/*
 * Mixer.hpp
 *
 *  Created on: Sep 17, 2022
 *      Author: Predtech
 */

#ifndef MIXER_MIXER_HPP_
#define MIXER_MIXER_HPP_

#include "../../Modules/libModules/Module.hpp"

class Mixer: public Module {
public:
	Parameter *volume[MAX_MODULE_INPUTS];

	Mixer();
	Mixer(Parameter *p) :
				Module(p, MAX_MODULE_INPUTS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		for(u32 i = 0; i < MAX_MODULE_INPUTS; i++)
		{
			volume[i] = tmp++;
			volume[i]->val = 1;
		}
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//

	}
	void process(void);
//	float &vol = volume->val;
};

#endif /* MIXER_MIXER_HPP_ */
