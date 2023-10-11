#include "Amp.hpp"

void Amp::process(void) {

	for (u8 lr = 0; lr < STEREO; lr++) {
		output[lr] = *input[lr] * volume->val;
	}
}

uint8_t Amp::getPublicParameter(uint8_t i) {
	return volume->val * 255;
}

void Amp::setPublicParameter(uint8_t i, uint8_t val) {
	volume->val = val / 255.;
}

