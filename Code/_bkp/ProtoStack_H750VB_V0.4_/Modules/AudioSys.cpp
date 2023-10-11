/*
 * System.cpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "AudioSys.hpp"
#include "Handy.h"

//__weak void includeModules(void) {;}

void AudioSys::getAudioSamplesInInput(SAIadapter_TypeDef* adapter) {
	for (u8 lr = 0; lr < STEREO; lr++)		// convert input, left & right
		inputModule->output[lr] = ((float) saiAdapter_getNextSample(adapter)) / MY_INT16_MAX;
}

void AudioSys::setAudioSamplesToOutput(SAIadapter_TypeDef* adapter) {
	for (u8 lr = 0; lr < STEREO; lr++) {		// convert output, left & right
		int16_t tmp = (int16_t)  (*(outputModule->input[lr]) * MY_INT16_MAX);
		saiAdapter_setNextSample(adapter, tmp);
	}

}

void AudioSys::update(void) {
	if(saiAdapterCheckUpdate(adapter1) && saiAdapterCheckUpdate(adapter2)) {
		for(uint8_t i = 0; i < SAI_HALF_BUF; i+=STEREO) {		// until half of RxBuf & TxBuf is not fully processed
			getAudioSamplesInInput(adapter1);	// load next sample pair in input module
			//getAudioSamplesInInput(adapter2);	// load next sample pair in input module
			for(uint8_t m = 0; m < MAX_MODULES_NUM; m++) {
				modules[m]->process();
			}
			setAudioSamplesToOutput(adapter1);
			//setAudioSamplesToOutput(adapter2);
		}
	}
}
