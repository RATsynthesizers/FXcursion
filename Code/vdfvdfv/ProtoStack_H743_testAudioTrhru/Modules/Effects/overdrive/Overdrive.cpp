#include "Overdrive.hpp"

void Overdrive::process(void) {
	float tmp[STEREO];
	float abs[STEREO];
	for (u8 lr = 0; lr < STEREO; lr++) {
		// Apply drive
		tmp[lr] = *input[lr] * (drive->val+0.1) *10.0;
		// Apply soft clipping
		abs[lr] = std::abs(tmp[lr]);
		if (abs[lr] > bias->val) {
			tmp[lr] = tmp[lr] > 0 ? (bias->val) : -(bias->val);
		} else {
			tmp[lr] = tmp[lr] * (3.0f - (tmp[lr] * tmp[lr] * 2.0f)) / 3.0f;
		}
		// Apply output gain
		tmp[lr] *= gain->val;
		// Clamp to [-1, 1]
		_CLIP_LIMIT(tmp[lr], -1.0f, 1.0f);
		// Output stereo samples
		output[lr] = (tmp[lr] * mix->val) + (*input[lr] * (1 - mix->val));
	}
}

uint8_t Overdrive::getPublicParameter(uint8_t i) {
	return (this->p + i)->val * 255;
}

void Overdrive::setPublicParameter(uint8_t i, uint8_t val) {
	(this->p + i)->val = val / 255.;
}
