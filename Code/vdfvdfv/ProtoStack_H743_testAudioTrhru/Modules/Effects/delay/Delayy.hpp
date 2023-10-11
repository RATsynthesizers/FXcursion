#ifndef DELAY_HPP_
#define DELAY_HPP_

#include "libModules/Module.hpp"

#define DELAY_PARAMETERS 4

class Delayy: public Module {
public:
	Parameter *delay;
	Parameter *feedback;
	Parameter *saturation;
	Parameter *mix;

	Delayy(Parameter *p) :
			Module(p, DELAY_PARAMETERS) {
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
	uint8_t getPublicParameter(uint8_t i);
	void setPublicParameter(uint8_t i, uint8_t val);
};

#endif /* DELAY_HPP_ */
