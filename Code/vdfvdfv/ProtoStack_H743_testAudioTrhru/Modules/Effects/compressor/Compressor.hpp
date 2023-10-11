#ifndef COMPRESSOR_HPP_
#define COMPRESSOR_HPP_

#include "libModules/Module.hpp"

#define COMPRESSOR_PARAMETERS 4

class Compressor: public Module {
public:
	Parameter *threshold;
	Parameter *ratio;
	Parameter *attack;
	Parameter *mkpgain;

	Compressor(Parameter *p) :
			Module(p, COMPRESSOR_PARAMETERS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;
		threshold = tmp++;
		ratio = tmp++;
		attack = tmp++;
		mkpgain = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//
		threshold->val = 1;
		ratio->val = 1;
		attack->val = 1;
		mkpgain->val = 1;
	}
	void process(void);
	uint8_t getPublicParameter(uint8_t i);
	void setPublicParameter(uint8_t i, uint8_t val);
};

#endif /* COMPRESSOR_HPP_ */
