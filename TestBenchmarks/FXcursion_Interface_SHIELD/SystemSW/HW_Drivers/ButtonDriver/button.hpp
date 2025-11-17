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
	BTN_RELEASED = 0,
	BTN_PRESSED	 = 1

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

	/////////////////////////////////////////////////
};

extern Button btnYes;
extern Button btnNo;
extern Button btnUp;
extern Button btnDown;
extern Button btnFoot;

#endif /* BUTTON_H_ */
