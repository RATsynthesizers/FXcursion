#ifndef VIBRATO_HPP_
#define VIBRATO_HPP_

#include "libModules/Module.hpp"

#define VIBRATO_PARAMETERS 4

class Vibrato: public Module {
public:
	Parameter *speed;
	Parameter *depth;
	Parameter *delay;
	Parameter *mix;

	Vibrato(Parameter *p) :
			Module(p, VIBRATO_PARAMETERS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		speed = tmp++;
		depth = tmp++;
		delay = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		speed->val = 1;
		depth->val = 1;
		delay->val = 1;
		mix->val = 1;
	}
	void process(void);
	uint8_t getPublicParameter(uint8_t i);
	void setPublicParameter(uint8_t i, uint8_t val);
};

#endif /* VIBRATO_HPP_ */