/*
 * led.hpp
 *
 *  Created on: Jan 21, 2022
 *      Author: rorka
 */

#ifndef HW_UI_LED_HPP_
#define HW_UI_LED_HPP_

#include "ui_adapter_reg.h"

#define LEDS_NUM 5

typedef enum {LED_OFF = 0, LED_R, LED_G, LED_B, LED_RG, LED_GB, LED_RB, LED_RGB} Led_States;

class Led {
public:

	Led(UIadapter_reg_TypeDef* adapter_, uint8_t RByteNum_, uint8_t RBitNum_, uint8_t GByteNum_, uint8_t GBitNum_, uint8_t BByteNum_, uint8_t BBitNum_, UI_Polarity_Types polarity_) {
		ledData.RByteNum = RByteNum_;
		ledData.RBitNum = RBitNum_;
		ledData.RBit = 0;
		ledData.GByteNum = GByteNum_;
		ledData.GBitNum = GBitNum_;
		ledData.GBit = 0;
		ledData.BByteNum = BByteNum_;
		ledData.BBitNum = BBitNum_;
		ledData.BBit = 0;
		ledData.polarity = polarity_;
		adapter = adapter_;
		state = LED_OFF;
	}

	void update(void);
	void setState(Led_States state_) { state = state_; update(); };
	Led_States getState(void) { return state; };

private:
	UIadapter_reg_TypeDef* adapter;
	UiRGBLedData_TypeDef ledData;
	Led_States state;
};

extern Led noLed;
extern Led yesLed;
extern Led recLed;
extern Led stpLed;
extern Led plyLed;

extern Led* leds[LEDS_NUM];

#endif /* HW_UI_LED_HPP_ */
