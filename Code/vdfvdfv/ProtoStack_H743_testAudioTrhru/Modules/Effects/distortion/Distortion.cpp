#include "Distortion.hpp"

void Distortion::process(void) {
	for (u8 lr = 0; lr < STEREO; lr++) {
		output[lr] = *input[lr];
	}
}

uint8_t Distortion::getPublicParameter(uint8_t i) {
	return (this->p + i)->val * 255;
}

void Distortion::setPublicParameter(uint8_t i, uint8_t val) {
	(this->p + i)->val = val / 255.;
}
