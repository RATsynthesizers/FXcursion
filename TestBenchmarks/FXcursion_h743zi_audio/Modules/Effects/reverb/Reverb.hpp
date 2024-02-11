#ifndef REVERB_HPP_
#define REVERB_HPP_

#include "libModules/Module.hpp"

#define REVERB_PARAMETERS 4

class Reverb: public Module {
public:
	Parameter *decay;
	Parameter *depth;
	Parameter *diffusion;
	Parameter *mix;

	Reverb(Parameter *p) :
			Module(p, REVERB_PARAMETERS) {
		name = "Reverb";
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		decay = tmp++;
		depth = tmp++;
		diffusion = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		decay->val = 1;
		depth->val = 1;
		diffusion->val = 1;
		mix->val = 1;
	}
	void process(void);
};

#endif /* REVERB_HPP_ */
