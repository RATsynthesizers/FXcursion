#ifndef OVERDRIVE_HPP_
#define OVERDRIVE_HPP_

#include "libModules/Module.hpp"
#include "../../DSP_MY/dsp.hpp"

#define OVERDRIVE_PARAMETERS 4

class Overdrive: public Module {
public:
	Parameter *drive;
	Parameter *gain;
	Parameter *bias;
	Parameter *mix;

	Overdrive(Parameter *p) :
			Module(p, OVERDRIVE_PARAMETERS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		drive = tmp++;
		gain = tmp++;
		bias = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		drive->val 	= 0.1;  // 0..1 mapped to range 1..11
		gain->val 	= 1.;
		bias->val 	= 0.7;
		mix->val 	= 1.;
	}
	void process(void);
	uint8_t getPublicParameter(uint8_t i);
	void setPublicParameter(uint8_t i, uint8_t val);
};

#endif /* OVERDRIVE_HPP_ */
