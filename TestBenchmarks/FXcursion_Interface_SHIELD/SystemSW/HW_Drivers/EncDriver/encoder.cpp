/*
 * encoder.cpp
 *
 *  Created on: Nov 16, 2025
 *      Author: ga
 */

#include "encoder.hpp"

Encoder encMenu(ENC_MENU_A_GPIO_Port,
				ENC_MENU_A_Pin,
				ENC_MENU_B_GPIO_Port,
				ENC_MENU_B_Pin);

Encoder encParam(ENC_PARAM_A_GPIO_Port,
				 ENC_PARAM_A_Pin,
				 ENC_PARAM_B_GPIO_Port,
				 ENC_PARAM_B_Pin);

void Encoder::update(void) {

	EncoderState prevState = state;

	BOOLEAN currentA = (BOOLEAN) HAL_GPIO_ReadPin(encA_Port, encA_Pin);
	BOOLEAN currentB = (BOOLEAN) HAL_GPIO_ReadPin(encB_Port, encB_Pin);

	// By default, assume no turn
	state = ENC_STILL;

	// Check if pin A has changed (a common edge to check for)
	if (currentA != pinValueA) // pinValueA is acting as the PREVIOUS A state
	{
		// Pin A changed. Check direction using current B state.

		// If A is leading B (A goes high or low, and B is at the state that indicates direction)
		// If A changes and (A != B) it's a right turn.
		if (currentA != currentB) // Right turn: A/B are 01 or 10, but the sequence matters
		{
			state = ENC_RIGHTTURN;
		}
		else // Left turn: A/B are 00 or 11
		{
			state = ENC_LEFTTURN;
		}
	}
	// else if (currentB != pinValueB) // You could also check B's change, but checking one channel's edge is enough for basic decoding.

	if (state != prevState)
	{
		bWasChanged = TRUE;
	}

	// Save current values for the next call (this is crucial!)
	pinValueA = currentA;
	pinValueB = currentB;
}




