#include "MyButtonController.hpp"
#include <main.h>
#include <touchgfx/hal/HAL.hpp>
#include "../UI/Button.hpp"
#include "../UI/Encoder.hpp"

void MyButtonController::init() {
	previousState = 0xFF;
}

bool MyButtonController::sample(uint8_t &key) {

	for (int i = 0; i < BUTTONS_NUM; i++) {
		if (buttons[i]->getState() == BTN_PRESSED) {
			if (previousState == 0xFF) {
				previousState = 0x00;
				key = i;
				return true;
			}
			return false;
		}
	}

	for (int i = 0; i < ENCS_NUM; i++) {
		if (encs[i]->getState() == ENC_LEFTTURN) {
			if (previousState == 0xFF) {
				previousState = 0x00;
				key = BUTTONS_NUM + i*2;
				return true;
			}
			return false;
		} else if (encs[i]->getState() == ENC_RIGHTTURN) {
			if (previousState == 0xFF) {
				previousState = 0x00;
				key = BUTTONS_NUM + i*2 + 1;
				return true;
			}
			return false;
		}
	}

	previousState = 0xFF;
	return false;
}
