#ifndef DISTORTION_HPP_
#define DISTORTION_HPP_

#include "libModules/Module.hpp"

#define DISTORTION_PARAMETERS 4

class Distortion: public Module {
public:
	Parameter *drive;
	Parameter *tone;
	Parameter *character;
	Parameter *mix;

	Distortion(Parameter *p) :
			Module(p, DISTORTION_PARAMETERS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		drive = tmp++;
		tone = tmp++;
		character = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		drive->val = 1;
		tone->val = 1;
		character->val = 1;
		mix->val = 1;
	}
	void process(void);
	uint8_t getPublicParameter(uint8_t i);
	void setPublicParameter(uint8_t i, uint8_t val);
};

#endif /* DISTORTION_HPP_ */
