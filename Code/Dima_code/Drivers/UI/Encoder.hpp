/*
 * encoder.hpp
 *
 *  Created on: Jan 20, 2022
 *      Author: rorka
 */

#ifndef HW_ENCODER_HPP_
#define HW_ENCODER_HPP_

#include "ui_adapter_reg.h"

#define ENCS_NUM 5

typedef enum {ENC_LEFTTURN = -1, ENC_STILL = 0, ENC_RIGHTTURN = 1} Encoder_States;

class Encoder {
public:
	Encoder (UIadapter_reg_TypeDef* adapter_, uint8_t AByteNum_, uint8_t ABitNum_, uint8_t BByteNum_, uint8_t BBitNum_, UI_Polarity_Types polarity_) {
		encData.AByteNum = AByteNum_;
		encData.ABitNum = ABitNum_;
		encData.BByteNum = BByteNum_;
		encData.BBitNum = BBitNum_;
		encData.polarity = polarity_;
		adapter = adapter_;
		state = ENC_STILL;
		pinValueA = !((bool)polarity_);     // active low
		pinValueB = !((bool)polarity_);     // active low
	}

	void update(void);

	Encoder_States getState(void) { return state; }

private:
	Encoder_States state;
	bool pinValueA;
	bool pinValueB;
	UIadapter_reg_TypeDef* adapter;

	////////////// HW CONNECTION ////////////////////
	UiEncData_TypeDef encData;
	/////////////////////////////////////////////////
};

extern Encoder enc1;
extern Encoder enc2;
extern Encoder enc3;
extern Encoder enc4;
extern Encoder enc5;
extern Encoder* encs[ENCS_NUM];

#endif /* HW_ENCODER_HPP_ */
