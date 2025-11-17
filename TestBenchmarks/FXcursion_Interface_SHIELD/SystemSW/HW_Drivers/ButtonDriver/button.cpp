/*
 * encoder.cpp
 *
 *  Created on: Nov 16, 2025
 *      Author: ga
 */

#include "button.hpp"

Button btnYes(BTN_YES_GPIO_Port,
			  BTN_YES_Pin);

Button btnNo(BTN_NO_GPIO_Port,
			 BTN_NO_Pin);

Button btnUp(BTN_UP_GPIO_Port,
			 BTN_UP_Pin);

Button btnDown(BTN_DOWN_GPIO_Port,
			   BTN_DOWN_Pin);

Button btnFoot(MY_FOOT1_GPIO_Port,
			   MY_FOOT1_Pin);

void Button::update(void) {

	ButtonState prevState = state;

	// Read the raw pin value
	BOOLEAN currentRaw = (BOOLEAN) HAL_GPIO_ReadPin(buttonPort, buttonPin);

	// 1. Check if the pin state has changed
	if (currentRaw != rawPinState)
	{
		// Pin state changed: reset the timer and update the raw state
		stateChangeTime = HAL_GetTick();
		rawPinState = currentRaw;
	}
	// 2. Check if the pin state has been stable long enough
	else
	{
		if ((HAL_GetTick() - stateChangeTime) >= DEBOUNCE_TIME)
		{
			// The pin has been stable for 50ms. Update the debounced state.
			if (GPIO_PIN_SET == currentRaw)
			{
				state = BTN_RELEASED;
			}
			else
			{
				state = BTN_PRESSED;
			}
			// Important: stateChangeTime is NOT reset here. It only resets when
			// the raw pin value changes again, which handles bounce.

			if (state != prevState)
			{
				bWasChanged = TRUE;
			}
		}
	}
}




