#ifndef CHORUS_HPP_
#define CHORUS_HPP_

#include "libModules/Module.hpp"

#define CHORUS_PARAMETERS 4

class Chorus: public Module {
public:


	Parameter *speed;
	Parameter *depth;
	Parameter *hpfilter;
	Parameter *mix;

	Chorus(Parameter *p) :
			Module(p, CHORUS_PARAMETERS) {
		name = "Chorus";
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		speed = tmp++;
		depth = tmp++;
		hpfilter = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		speed->val = 1;
		depth->val = 1;
		hpfilter->val = 1;
		mix->val = 1;
	}
	void process(void);
};

#endif /* CHORUS_HPP_ */
