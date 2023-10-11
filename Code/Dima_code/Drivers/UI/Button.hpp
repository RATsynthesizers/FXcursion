/*
 * button.hpp
 *
 *  Created on: Jan 20, 2022
 *      Author: rorka
 */

#ifndef HW_BUTTON_HPP_
#define HW_BUTTON_HPP_

#include "ui_adapter_reg.h"

#define BUTTONS_NUM 12

typedef enum {BTN_UNPRESSED = 0, BTN_PRESSED} Button_States;

class Button {
public:
	Button(UIadapter_reg_TypeDef* adapter_, uint8_t btnRegBit_, UI_Polarity_Types polarity_) {
		adapter = adapter_;
		btnData.btnBit = btnRegBit_;
		btnData.polarity = polarity_;
		state = BTN_UNPRESSED;
		pinValue = !((bool)polarity_);
	}
	void update(void);
	Button_States getState(void) { return state; }

private:
	Button_States state;
	bool pinValue;
	UIadapter_reg_TypeDef* adapter;

	////////////// HW CONNECTION ////////////////////
	UiBtnData_TypeDef btnData;
	/////////////////////////////////////////////////
};

extern Button btnY   ;
extern Button btnN   ;
extern Button btnREC ;
extern Button btnSTP ;
extern Button btnPLY ;
extern Button enc1btn;
extern Button enc2btn;
extern Button enc3btn;
extern Button enc4btn;
extern Button enc5btn;

extern Button* buttons[BUTTONS_NUM];


#endif /* HW_BUTTON_HPP_ */




