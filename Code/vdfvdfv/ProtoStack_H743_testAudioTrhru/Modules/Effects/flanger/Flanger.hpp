#ifndef FLANGER_HPP_
#define FLANGER_HPP_

#include "libModules/Module.hpp"

#define FLANGER_PARAMETERS 4

class Flanger: public Module {
public:
	Parameter *speed;
	Parameter *depth;
	Parameter *resonance;
	Parameter *mix;

	Flanger(Parameter *p) :
			Module(p, FLANGER_PARAMETERS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		speed = tmp++;
		depth = tmp++;
		resonance = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		speed->val = 1;
		depth->val = 1;
		resonance->val = 1;
		mix->val = 1;
	}
	void process(void);
	uint8_t getPublicParameter(uint8_t i);
	void setPublicParameter(uint8_t i, uint8_t val);
};

#endif /* FLANGER_HPP_ */
