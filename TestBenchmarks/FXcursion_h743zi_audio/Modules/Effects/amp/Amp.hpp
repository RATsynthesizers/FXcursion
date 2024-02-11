#ifndef AMP_HPP_
#define AMP_HPP_

#include "libModules/Module.hpp"

#define AMP_PARAMETERS 0

class Amp: public Module {
public:
	Amp() :
			Module(NULL, AMP_PARAMETERS) {
		name = "Amp";
	}
	void process(void);
};

#endif /* AMP_HPP_ */
