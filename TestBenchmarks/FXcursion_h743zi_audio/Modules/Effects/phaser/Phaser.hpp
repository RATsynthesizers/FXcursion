#ifndef PHASER_HPP_
#define PHASER_HPP_

#include "libModules/Module.hpp"

#define PHASER_PARAMETERS 4

class Phaser: public Module {
public:
	Parameter *speed;
	Parameter *depth;
	Parameter *feedback;
	Parameter *mix;

	Phaser(Parameter *p) :
			Module(p, PHASER_PARAMETERS) {
		name = "Phaser";
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		speed = tmp++;
		depth = tmp++;
		feedback = tmp++;
		mix = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		speed->val = 1;
		depth->val = 1;
		feedback->val = 1;
		mix->val = 1;
	}
	void process(void);
};

#endif /* PHASER_HPP_ */
