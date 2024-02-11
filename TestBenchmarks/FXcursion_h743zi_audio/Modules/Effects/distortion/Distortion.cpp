#include "Distortion.hpp"

void Distortion::process(void) {
	for (u8 lr = 0; lr < STEREO; lr++) {
		output[lr] = prevModule->output[lr];
	}
}
