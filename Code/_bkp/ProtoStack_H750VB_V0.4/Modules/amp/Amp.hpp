/*
 * amp.hpp
 *
 *  Created on: Aug 25, 2021
 *      Author: romte
 */

#ifndef AMP_AMP_HPP_
#define AMP_AMP_HPP_

#include "../libModules/Module.hpp"

#define AMP_PARAMETERS 1

class Amp: public Module {
public:
	Parameter *volume;

	Amp();
	Amp(Parameter *p) :
				Module(p, AMP_PARAMETERS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		volume = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		volume->val = 1;
	}
	void process(void);
//	float &vol = volume->val;
};

#endif /* AMP_AMP_HPP_ */
