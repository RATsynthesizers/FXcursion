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

Button btnFunc(BTN_FUNC_GPIO_Port,
			   BTN_FUNC_Pin);

void Button::update(void)
{

	ButtonState prevState = state;
	U32 currentTick = HAL_GetTick(); // Get the current time once

	// Read the raw pin value
	BOOLEAN currentRaw = (BOOLEAN) HAL_GPIO_ReadPin(buttonPort, buttonPin);

	// --- 1. Debouncing Logic (Detecting stable transition) ---
	if (currentRaw != rawPinState)
	{
		// Pin state changed: reset the debounce timer and update the raw state
		stateChangeTime = currentTick;
		rawPinState = currentRaw;
	}
	else
	{
		// Pin is stable: check if the debounce time has passed
		if ((currentTick - stateChangeTime) >= DEBOUNCE_TIME)
		{
			// The pin is stable and debounced. Determine the stable state.
			ButtonState stableState = (GPIO_PIN_SET == currentRaw) ? BTN_RELEASED : BTN_PRESSED;

			// --- 2. State Transition Logic ---
			if (stableState != state)
			{
				// A stable change (RELEASE -> PRESSED OR PRESSED -> RELEASE)
				state = stableState;
				bWasChanged = TRUE;

				if (state == BTN_PRESSED)
				{
					// Button just entered the stable PRESSED state. Start the long press timer.
					pressStartTime = currentTick;
				}
			}
			// --- 3. Long Press Logic (Only check if currently in PRESSED state) ---
			else if (state == BTN_PRESSED)
			{
				if ((currentTick - pressStartTime) >= LONG_PRESS_TIME)
				{
					// Time elapsed! Transition to LONG_PRESS state
					state = BTN_LONG_PRESS;
					bWasChanged = TRUE;

					// NOTE: We don't reset pressStartTime. The button will stay in
					// BTN_LONG_PRESS until physically released.
				}
			}
		}
	}

	// Ensure that state == BTN_LONG_PRESS is not accidentally marked as changed
	// unless the button actually transitioned to it (handled above).
	// The flag bWasChanged is already set inside the state transition checks.
}



