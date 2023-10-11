/*
 * amp.cpp
 *
 *  Created on: Aug 25, 2021
 *      Author: romte
 */

#include "Amp.hpp"


void Amp::process(void) {

	for (u8 lr = 0; lr < STEREO; lr++) {
		output[lr] = *input[lr] * volume->val;
	}
}

