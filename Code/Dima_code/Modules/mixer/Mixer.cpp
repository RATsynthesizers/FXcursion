/*
 * Mixer.cpp
 *
 *  Created on: Sep 17, 2022
 *      Author: Predtech
 */

#include "Mixer.hpp"

void Mixer::process(void) {

	for (u8 lr = 0; lr < STEREO; lr++)
	{
		output[lr] = 0;
		uint8_t inputsNum = 0;
		for (u8 i = 0; i < MAX_MODULE_INPUTS; i++)
			inputsNum += isInputPlugged[i];
		for (u8 i = 0; i < MAX_MODULE_INPUTS; i++)
			if(isInputPlugged[i])
				output[lr] += *input[lr][i] * volume[i]->val / inputsNum;
	}
}



