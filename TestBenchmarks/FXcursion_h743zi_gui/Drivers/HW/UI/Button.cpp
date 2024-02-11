/*
 * button.cpp
 *
 *  Created on: Jan 20, 2022
 *      Author: rorka
 */

#include "Button.hpp"

#include "Led.hpp"

void Button::update(void) {

	getBtnHWdata(adapter, &btnData);
	pinValue = btnData.btn ^ !btnData.polarity;

	if(!pinValue)
		state = BTN_UNPRESSED;
	else
		state = BTN_PRESSED;
}

