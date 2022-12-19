/*
 * Parameter.hpp
 *
 *  Created on: Nov 11, 2020
 *      Author: romte
 */

#ifndef PARAMETER_HPP_
#define PARAMETER_HPP_

#include <Handy.h>

#include "../../Modules/SysGlobals.h"

class Parameter {
public:
	float val;                          // value
	u32 index;                          // index in module

	Parameter(){
		val = 0;  						// value
		index = 0;
    }
};


#endif /* PARAMETER_HPP_ */
