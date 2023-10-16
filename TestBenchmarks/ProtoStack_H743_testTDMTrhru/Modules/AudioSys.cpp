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
			for(uint8_t m = 0; m < (modules[0]->instancesNum - IO_MODULES_NUM); m++) {	// -2 i/o modules
				modules[m]->process();
			}

			setAudioSamplesToOutput(adapter1);
			//setAudioSamplesToOutput(adapter2);
		}
	}
}

void AudioSys::updateParams(void) {
	if(guiAdapterCheckUpdate(&guiadapter)) {
		parseGUImsg(&guiadapter);
	}
}

void AudioSys::parseGUImsg(GUIadapter_TypeDef *adapter) {
	uint8_t msgBufIdx = adapter->msgBufIdx;
	switch (adapter->msgBuf[msgBufIdx].cmd) {
	case REQUEST_PARAM:
		parseRequestParamMsg(adapter);
		break;
	default:
		break;
	}
}

void AudioSys::parseRequestParamMsg(GUIadapter_TypeDef *adapter) {
	switch (adapter->msgBuf[adapter->msgBufIdx].fxs[0].name) {
	case T_CHORUSEFFECT:
		parseChorusFx(adapter);
		break;
	default:
		break;
	}
}

// [cmd: 0x00, chain: any, fx.name: 0x00, fx.instance: 0x00, fx.order_index: 0x00, zeroes] - 32 bytes
// const uint8_t buf[] = {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x01};
// HAL_UART_Transmit_DMA(&huart2, buf, 32);

void AudioSys::parseChorusFx(GUIadapter_TypeDef *adapter) {
//	uint8_t itmp = adapter->msgBuf[adapter->msgBufIdx].fxs[0].instance;
	uint8_t oritmp = adapter->msgBuf[adapter->msgBufIdx].fxs[0].order_index;
	uint8_t paramnum = this->modules[oritmp]->paramNum;

	for (uint8_t i = 0; i < paramnum; i++) {
		adapter->TxBuf[i] = (uint8_t) (this->modules[oritmp]->p[i].val * 255.0);
	}
	transmitGUImsg(adapter, paramnum);
	adapter->msgBufIdx = adapter->msgBufIdx -1;
}
