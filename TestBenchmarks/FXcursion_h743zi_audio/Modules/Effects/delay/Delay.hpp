#ifndef DELAY_HPP_
#define DELAY_HPP_

#include "libModules/Module.hpp"

#define DELAY_PARAMETERS 4

class Delay: public Module {
public:
	Parameter *delay;
	Parameter *feedback;
	Parameter *saturation;
	Parameter *mix;

	Delay(Parameter *p) :
			Module(p, DELAY_PARAMETERS) {
		name = "Delay";
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		delay = tmp++;
		feedback = tmp++;
		saturation = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		delay->val = 1;
		feedback->val = 1;
		saturation->val = 1;
		mix->val = 1;
	}
	void process(void);
};

#endif /* DELAY_HPP_ */
