/*
 * encoder.cpp
 *
 *  Created on: Jan 20, 2022
 *      Author: rorka
 */

#include "Encoder.hpp"

void Encoder::update(void) {

	getEncHWdata(adapter, &encData);
	pinValueA = encData.encA ^ !encData.polarity;
	pinValueB = encData.encB ^ !encData.polarity;

	if (pinValueB && pinValueA)
		state = ENC_RIGHTTURN;
	else if (pinValueB)
		state = ENC_LEFTTURN;
	else
		state = ENC_STILL;
}

