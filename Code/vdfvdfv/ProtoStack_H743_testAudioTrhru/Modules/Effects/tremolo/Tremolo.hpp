#ifndef TREMOLO_HPP_
#define TREMOLO_HPP_

#include "libModules/Module.hpp"

#define TREMOLO_PARAMETERS 4

class Tremolo: public Module {
public:
	Parameter *speed;
	Parameter *depth;
	Parameter *shape;
	Parameter *width;

	Tremolo(Parameter *p) :
			Module(p, TREMOLO_PARAMETERS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		speed = tmp++;
		depth = tmp++;
		shape = tmp++;
		width = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		speed->val = 1;
		depth->val = 1;
		shape->val = 1;
		width->val = 1;
	}
	void process(void);
	uint8_t getPublicParameter(uint8_t i);
	void setPublicParameter(uint8_t i, uint8_t val);
};

#endif /* TREMOLO_HPP_ */
