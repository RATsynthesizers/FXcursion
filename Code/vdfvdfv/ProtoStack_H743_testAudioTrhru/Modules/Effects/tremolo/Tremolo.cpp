#include "Tremolo.hpp"

void Tremolo::process(void) {
	for (u8 lr = 0; lr < STEREO; lr++) {
		output[lr] = *input[lr];
	}
}

uint8_t Tremolo::getPublicParameter(uint8_t i) {
	return (this->p + i)->val * 255;
}

void Tremolo::setPublicParameter(uint8_t i, uint8_t val) {
	(this->p + i)->val = val / 255.;
}
