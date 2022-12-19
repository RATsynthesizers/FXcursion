/*
 * Led.cpp
 *
 *  Created on: Jan 21, 2022
 *      Author: rorka
 */

#include "Led.hpp"

void Led::update(void) {
	switch(state) {
	case LED_OFF:
		ledData.RBit = 0 ^ !ledData.polarity;
		ledData.GBit = 0 ^ !ledData.polarity;
		ledData.BBit = 0 ^ !ledData.polarity;
		break;
	case LED_R:
		ledData.RBit = 1 ^ !ledData.polarity;
		ledData.GBit = 0 ^ !ledData.polarity;
		ledData.BBit = 0 ^ !ledData.polarity;
		break;
	case LED_G:
		ledData.RBit = 0 ^ !ledData.polarity;
		ledData.GBit = 1 ^ !ledData.polarity;
		ledData.BBit = 0 ^ !ledData.polarity;
		break;
	case LED_B:
		ledData.RBit = 0 ^ !ledData.polarity;
		ledData.GBit = 0 ^ !ledData.polarity;
		ledData.BBit = 1 ^ !ledData.polarity;
		break;
	case LED_RG:
		ledData.RBit = 1 ^ !ledData.polarity;
		ledData.GBit = 1 ^ !ledData.polarity;
		ledData.BBit = 0 ^ !ledData.polarity;
		break;
	case LED_GB:
		ledData.RBit = 0 ^ !ledData.polarity;
		ledData.GBit = 1 ^ !ledData.polarity;
		ledData.BBit = 1 ^ !ledData.polarity;
		break;
	case LED_RB:
		ledData.RBit = 1 ^ !ledData.polarity;
		ledData.GBit = 0 ^ !ledData.polarity;
		ledData.BBit = 1 ^ !ledData.polarity;
		break;
	case LED_RGB:
		ledData.RBit = 1 ^ !ledData.polarity;
		ledData.GBit = 1 ^ !ledData.polarity;
		ledData.BBit = 1 ^ !ledData.polarity;
		break;
	}
	///
	setLedHWdata(adapter, &ledData);

}


