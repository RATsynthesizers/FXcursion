#include "Amp.hpp"

void Amp::process(void) {
	for (u8 lr = 0; lr < STEREO; lr++) {
		output[lr] = output[lr] * outputVolume;
	}
}
