/*
 * encoder.h
 *
 *  Created on: Nov 16, 2025
 *      Author: ga
 */

#ifndef BUTTON_HPP_
#define BUTTON_HPP_


#include "gpio.h"
#include "general.h"
#include "common_cfg.h"


typedef enum enButtonState
{
	BTN_RELEASED 	= 0,
	BTN_PRESSED	 	= 1,
	BTN_LONG_PRESS 	= 2,

} ButtonState;


class Button {
public:
	Button(GPIO_TypeDef* _buttonPort,
		   U16 _buttonPin)
	{
		buttonPort = _buttonPort;
		buttonPin = _buttonPin;

		state = BTN_RELEASED;
	}

	void update(void);

	ButtonState getState(void) { return state; }

	BOOLEAN wasChanged(void) { return bWasChanged; }

	void clearChangedFlag(void) { bWasChanged = FALSE; }

private:
	ButtonState state;
	BOOLEAN bWasChanged;

	GPIO_TypeDef* buttonPort;
	U16 buttonPin;

	BOOLEAN rawPinState; 		  // Stores the last raw pin read (0 or 1)
	U32 stateChangeTime;	  	  // Time (in ms) when the rawPinState last changed
	const U32 DEBOUNCE_TIME = 50; // 50ms is a safe debounce window

	U32 pressStartTime; 			   // Time when the button state became stable BTN_PRESSED
	const U32 LONG_PRESS_TIME = 1000;  // 1000ms (1 second) for a long press event

	/////////////////////////////////////////////////
};

#endif /* BUTTON_H_ */
