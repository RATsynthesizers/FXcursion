/*
 * Mixer.hpp
 *
 *  Created on: 11 июл. 2023 г.
 *      Author: Predtech
 */

#ifndef LIBMODULES_MIXER_HPP_
#define LIBMODULES_MIXER_HPP_

#include <stddef.h>
#include "Handy.h"
#include "Module.hpp"
#include "../Effects/amp/Amp.hpp"

class Mixer {
public:

	Module *prevModule[CHANNELS_NUM / 2];
	Module *processingModule[CHANNELS_NUM / 2];
	Module *nextModule[CHANNELS_NUM / 2];

	float volumeIn[CHANNELS_NUM / 2];
	float volumeOut[CHANNELS_NUM / 2];

	u16 connectionBitmap;

	Mixer() {
		connectionBitmap = 0b100010001; // 1 to 1, 2 to 2, 3 to 3
		for (int i = 0; i < CHANNELS_NUM / 2; i++) {
			prevModule[i] = NULL;
			processingModule[i] = new Amp();
			nextModule[i] = NULL;
			volumeIn[i] = 1;
			volumeOut[i] = 1;
		}
	}

	void plugNext(Module *nextModule, u8 chnum) {
		this->nextModule[chnum] = nextModule;
		processingModule[chnum]->plugNext(nextModule);
	}

	void plugPrev(Module *prevModule, u8 chnum) {
		this->prevModule[chnum] = prevModule;
	}

	void unplugNext(u8 chnum) {
		//nextModule[chnum]->unplugPrev();
		this->nextModule[chnum] = NULL;
	}

	void unplugPrev(u8 chnum) {
		this->prevModule[chnum] = NULL;
	}

	void process() {

		for (u8 i = 0; i < CHANNELS_NUM / 2; i++) {

			if (!nextModule[i])
				continue;

			float countNextConnections = 0;
			float output[STEREO];
			for (int j = 0; j < STEREO; j++)
				output[j] = 0;

			for (u8 j = 0; j < CHANNELS_NUM / 2; j++) {
				u8 isConnected = connectionBitmap
						>> (j * (CHANNELS_NUM / 2) + i) & 1;
				if (isConnected && prevModule[j]) {
					for (u8 lr = 0; lr < STEREO; lr++)
						output[lr] += prevModule[j]->output[lr];
				}
				countNextConnections += isConnected;
			}

			if (!countNextConnections)
				continue;

			for (int j = 0; j < STEREO; j++)
				processingModule[i]->output[j] = output[j]
						/ countNextConnections;
		}
	}
};

#endif /* LIBMODULES_MIXER_HPP_ */
