/*
 * Looper.hpp
 *
 *  Created on: Sep 18, 2022
 *      Author: Predtech
 */

#ifndef LOOPER_LOOPER_HPP_
#define LOOPER_LOOPER_HPP_

#include "../../Modules/libModules/Module.hpp"

#define LOOPER_PARAMETERS 1

class Looper: public Module {
public:
	Parameter *trimLength;

	Looper();
	Looper(Parameter *p) :
				Module(p, LOOPER_PARAMETERS) {
		//----------------- parameter address init  ------------------//
		Parameter *tmp = this->p;

		this->recordStatus = 0;
		trimLength = tmp++;
		trimLength->val = 0;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters
		//----------------- parameter values init  --------------------//

	}
	void process(void);


	LOOPERQUEUE_OBJ_t looper_msg;
	uint8_t recordStatus;
};

#endif /* LOOPER_LOOPER_HPP_ */
