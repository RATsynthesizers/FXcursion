#ifndef OVERDRIVE_HPP_
#define OVERDRIVE_HPP_

#include "libModules/Module.hpp"

#define OVERDRIVE_PARAMETERS 4

class Overdrive: public Module {
public:
	Parameter *drive;
	Parameter *gain;
	Parameter *bias;
	Parameter *mix;

	Overdrive(Parameter *p) :
			Module(p, OVERDRIVE_PARAMETERS) {
		name = "Overdrive";
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		drive = tmp++;
		gain = tmp++;
		bias = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		drive->val = 0.5;
		gain->val = 1;
		bias->val = 1;
		mix->val = 1;
	}
	void process(void);
};

#endif /* OVERDRIVE_HPP_ */
