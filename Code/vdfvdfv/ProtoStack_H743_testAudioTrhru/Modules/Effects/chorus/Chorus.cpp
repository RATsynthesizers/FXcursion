#include "Chorus.hpp"

void Chorus::process(void) {
	for (u8 lr = 0; lr < STEREO; lr++) {
		output[lr] = *input[lr];
	}
}

uint8_t Chorus::getPublicParameter(uint8_t i) {
	return (this->p + i)->val * 255;
}

void Chorus::setPublicParameter(uint8_t i, uint8_t val) {
	(this->p + i)->val = val / 255.;
}
