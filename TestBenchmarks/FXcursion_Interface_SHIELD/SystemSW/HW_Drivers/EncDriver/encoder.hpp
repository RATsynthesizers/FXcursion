/*
 * encoder.h
 *
 *  Created on: Nov 16, 2025
 *      Author: ga
 */

#ifndef ENCODER_HPP_
#define ENCODER_HPP_


#include "gpio.h"
#include "general.h"
#include "common_cfg.h"

typedef enum enEncoderState
{
	ENC_LEFTTURN  = -1,
	ENC_STILL 	  = 0,
	ENC_RIGHTTURN = 1

} EncoderState;

class Encoder {
public:
	Encoder(GPIO_TypeDef* _encA_Port,
			uint16_t _encA_Pin,
			GPIO_TypeDef* _encB_Port,
			uint16_t _encB_Pin)
	{
		encA_Port = _encA_Port;
		encA_Pin = _encA_Pin;

		encB_Port = _encB_Port;
		encB_Pin = _encB_Pin;

		state = ENC_STILL;
		pinValueA = FALSE;
		pinValueB = FALSE;
	}

	void update(void);

	EncoderState getState(void) { return state; }

	BOOLEAN wasChanged(void) { return bWasChanged; }

	void clearChangedFlag(void) { bWasChanged = FALSE; }

private:
	EncoderState state;
	BOOLEAN bWasChanged;

	BOOLEAN pinValueA;
	BOOLEAN pinValueB;

	GPIO_TypeDef* encA_Port;
	uint16_t encA_Pin;

	GPIO_TypeDef* encB_Port;
	uint16_t encB_Pin;

	/////////////////////////////////////////////////
};

extern Encoder encMenu;
extern Encoder encParam;

#endif /* HW_DRIVERS_ENCDRIVER_ENCODER_H_ */
