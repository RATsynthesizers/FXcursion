/*
 * System.cpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "AudioSys.hpp"
#include "Handy.h"

//__weak void includeModules(void) {;}

void AudioSys::getAudioSamplesInInput(SAIadapter_TypeDef *adapter) {
	for (u8 lr = 0; lr < STEREO; lr++)		// convert input, left & right
		inputModule->output[lr] = ((float) saiAdapter_getNextSample(adapter))
				/ MY_INT16_MAX;
}

void AudioSys::setAudioSamplesToOutput(SAIadapter_TypeDef *adapter) {
	for (u8 lr = 0; lr < STEREO; lr++) {		// convert output, left & right
		int16_t tmp = (int16_t) (*(outputModule->input[lr]) * MY_INT16_MAX);
		saiAdapter_setNextSample(adapter, tmp);
	}
}

void AudioSys::updateGUI(UARTadapter_TypeDef *adapter) {
	switch (adapter->msgIn.cmd) {
	case REQUEST_PARAM:
		for (int i = 0; i < MODULE_PARAMS_NUM; i++) {
			adapter->msgOut.params[i] =
			modules[adapter->msgIn.chain][adapter->msgIn.fxIdx]->getPublicParameter(i);
		}
		prepareUARTMsgTx(adapter);
		transmitUARTmsg(adapter);

		break;
	case CHANGE_PARAM:
		modules[adapter->msgIn.chain][adapter->msgIn.fxIdx]->setPublicParameter(
				adapter->msgIn.parameterIndex, adapter->msgIn.parameterValue);
		break;
	default:
		break;
	}
	adapter->update_flag = 0; // reset update flag
}


void AudioSys::update(void) {
// update parameters from GUI if needed
if (UARTadapterCheckUpdate(&audio_uart_adapter)) {
	this->updateGUI(&audio_uart_adapter);
}
modulesInChain[0] = 1;
// get audio samples if needed
if (saiAdapterCheckUpdate(adapter1) && saiAdapterCheckUpdate(adapter2)) {
	for (uint8_t i = 0; i < SAI_HALF_BUF; i += STEREO) {// until half of RxBuf & TxBuf is not fully processed
		getAudioSamplesInInput(adapter1);// load next sample pair in input module
		getAudioSamplesInInput(adapter2);	// load next sample pair in input module
		for (int j = 0; j < CHANNELS_NUM; j++)
			for (uint8_t m = 0; m < modulesInChain[j]; m++) {// -2 i/o modules
				modules[j][m]->process();
			}

		setAudioSamplesToOutput(adapter1);
		//setAudioSamplesToOutput(adapter2);
	}
}
}
