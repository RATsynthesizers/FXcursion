/*
 * System.cpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "AudioSys.hpp"
#include "Handy.h"

IN_DTCMRAM AudioSys audioSystem;

void AudioSys::getAudioSamplesInInput(u8 chnum, SAIadapter_TypeDef *adapter) {
	for (u8 lr = 0; lr < STEREO; lr++)		// convert input, left & right
			{
		inputModule[chnum]->output[lr] = ((float) saiAdapter_getNextSample(
				adapter)) / MY_INT16_MAX;
	}
}

void AudioSys::setAudioSamplesToOutput(u8 chnum, SAIadapter_TypeDef *adapter) {
	for (u8 lr = 0; lr < STEREO; lr++) {		// convert output, left & right
		int16_t tmp = 0;
		if (outputModule[chnum]->prevModule != NULL)
			tmp = (int16_t) (outputModule[chnum]->prevModule->output[lr]
					* MY_INT16_MAX);
		saiAdapter_setNextSample(adapter, tmp);
	}
}

void AudioSys::updateGUI(GUIadapter_TypeDef *adapter) {
	switch (adapter->RxBuf[0]) {
	case REQUEST_PARAM:
		for (int i = 0; i < MODULE_PARAMS_NUM; i++) {
			adapter->TxBuf[i] =
					modules[adapter->RxBuf[1]][adapter->RxBuf[2]]->getPublicParameter(
							i);
		}
		HAL_UART_Transmit_DMA(&huart2, adapter->TxBuf, GUIUART_OUTBUF_SIZE);

		break;
	case CHANGE_PARAM:
		modules[adapter->RxBuf[1]][adapter->RxBuf[2]]->setPublicParameter(
				adapter->RxBuf[3], adapter->RxBuf[4]);
		break;
	case FX_ADD:

		if (adapter->RxBuf[1] < CHANNELS_NUM / 2) {
			Module *module = inputModule[adapter->RxBuf[1]];
			while (module->nextModule)
				module = module->nextModule;
			module->plugNext(modules[adapter->RxBuf[1]][adapter->RxBuf[2]]);
			module = module->nextModule;
			mixer->unplugPrev(adapter->RxBuf[1]);
			mixer->plugPrev(module, adapter->RxBuf[1]);
		} else {
			adapter->RxBuf[1] -= CHANNELS_NUM / 2;
			Module *module = outputModule[adapter->RxBuf[1]];
			if (module->prevModule->prevModule) {
				module = module->prevModule;
				module->unplugNext();
				module->plugNext(
						modules[adapter->RxBuf[1] + CHANNELS_NUM / 2][adapter->RxBuf[2]]);
				module = module->nextModule;
				module->plugNext(outputModule[adapter->RxBuf[1]]);
			} else {
				module->unplugPrev();
				module =
						modules[adapter->RxBuf[1] + CHANNELS_NUM / 2][adapter->RxBuf[2]];
				module->plugNext(outputModule[adapter->RxBuf[1]]);
				mixer->unplugNext(adapter->RxBuf[1]);
				mixer->plugNext(module, adapter->RxBuf[1]);
			}
		}
		break;
	case FX_MOVE_UP:
		if (adapter->RxBuf[1] < CHANNELS_NUM / 2) {
			Module *module = inputModule[adapter->RxBuf[1]]->nextModule;
			for (int i = 0; i < adapter->RxBuf[2]; i++)
				module = module->nextModule;
			if (module->nextModule) {
				Module *tempPrevPrev = module->prevModule->prevModule;
				Module *tempPrev = module->prevModule;
				Module *tempNext = module->nextModule;

				module->unplugNext();
				tempPrev->unplugNext();
				tempPrevPrev->unplugNext();

				tempPrevPrev->plugNext(module);
				module->plugNext(tempPrev);
				tempPrev->plugNext(tempNext);
			} else {
				Module *tempPrevPrev = module->prevModule->prevModule;
				Module *tempPrev = module->prevModule;

				mixer->unplugPrev(adapter->RxBuf[1]);
				tempPrev->unplugNext();
				tempPrevPrev->unplugNext();

				tempPrevPrev->plugNext(module);
				module->plugNext(tempPrev);
				mixer->plugPrev(tempPrev, adapter->RxBuf[1]);
			}
		} else {
			adapter->RxBuf[1] -= CHANNELS_NUM / 2;
			Module *module = mixer->nextModule[adapter->RxBuf[1]];
			for (int i = 0; i < adapter->RxBuf[2]; i++)
				module = module->nextModule;
			if (module->prevModule->prevModule->prevModule) {
				Module *tempPrevPrev = module->prevModule->prevModule;
				Module *tempPrev = module->prevModule;
				Module *tempNext = module->nextModule;

				tempPrevPrev->unplugNext();
				tempPrev->unplugNext();
				module->unplugNext();

				tempPrevPrev->plugNext(module);
				module->plugNext(tempPrev);
				tempPrev->plugNext(tempNext);
			} else {
				Module *tempPrev = module->prevModule;
				Module *tempNext = module->nextModule;

				mixer->unplugNext(adapter->RxBuf[1]);
				tempPrev->unplugNext();
				module->unplugNext();

				mixer->plugNext(module, adapter->RxBuf[1]);
				module->plugNext(tempPrev);
				tempPrev->plugNext(tempNext);
			}
		}
		break;
	case FX_MOVE_DOWN:
		if (adapter->RxBuf[1] < CHANNELS_NUM / 2) {
			Module *module = inputModule[adapter->RxBuf[1]]->nextModule;
			for (int i = 0; i < adapter->RxBuf[2]; i++)
				module = module->nextModule;
			if (module->nextModule->nextModule) {
				Module *tempPrev = module->prevModule;
				Module *tempNext = module->nextModule;
				Module *tempNextNext = module->nextModule->nextModule;

				tempPrev->unplugNext();
				module->unplugNext();
				tempNext->unplugNext();

				tempPrev->plugNext(tempNext);
				tempNext->plugNext(module);
				module->plugNext(tempNextNext);
			} else {
				Module *tempPrev = module->prevModule;
				Module *tempNext = module->nextModule;

				tempPrev->unplugNext();
				module->unplugNext();
				mixer->unplugPrev(adapter->RxBuf[1]);

				tempPrev->plugNext(tempNext);
				tempNext->plugNext(module);
				mixer->plugPrev(module, adapter->RxBuf[1]);
			}
		} else {
			adapter->RxBuf[1] -= CHANNELS_NUM / 2;
			Module *module = mixer->nextModule[adapter->RxBuf[1]];
			for (int i = 0; i < adapter->RxBuf[2]; i++)
				module = module->nextModule;
			if (module->prevModule->prevModule) {
				Module *tempPrev = module->prevModule;
				Module *tempNext = module->nextModule;
				Module *tempNextNext = module->nextModule->nextModule;

				tempPrev->unplugNext();
				module->unplugNext();
				tempNext->unplugNext();

				tempPrev->plugNext(tempNext);
				tempNext->plugNext(module);
				module->plugNext(tempNextNext);
			} else {
				Module *tempNext = module->nextModule;
				Module *tempNextNext = module->nextModule->nextModule;

				mixer->unplugNext(adapter->RxBuf[1]);
				module->unplugNext();
				tempNext->unplugNext();

				mixer->plugNext(tempNext, adapter->RxBuf[1]);
				tempNext->plugNext(module);
				module->plugNext(tempNextNext);
			}
		}
		break;
	case FX_DELETE:
		if (adapter->RxBuf[1] < CHANNELS_NUM / 2) {
			Module *module = inputModule[adapter->RxBuf[1]];
			for (int i = 0; i < adapter->RxBuf[2]; i++)
				module = module->nextModule;
			if (module->nextModule->nextModule) {
				Module *temp = module->nextModule->nextModule;
				module->unplugNext();
				temp->prevModule->unplugNext();
				module->plugNext(temp);
			} else {
				module->unplugNext();
				mixer->unplugPrev(adapter->RxBuf[1]);
				mixer->plugPrev(module, adapter->RxBuf[1]);
			}
		} else {
			adapter->RxBuf[1] -= CHANNELS_NUM / 2;
			Module *module = mixer->nextModule[adapter->RxBuf[1]];
			for (int i = 0; i < adapter->RxBuf[2]; i++)
				module = module->nextModule;

			Module *temp = module->nextModule;
			module->unplugNext();
			module = module->prevModule;
			if (module->nextModule) {
				module->unplugNext();
				module->plugNext(temp);
			} else {
				mixer->unplugNext(adapter->RxBuf[1]);
				mixer->plugNext(temp, adapter->RxBuf[1]);
			}
		}
		break;
	case CHAIN_PLUG_WIRE:
		mixer->connectionBitmap |= 1 << adapter->RxBuf[1];
		break;
	case CHAIN_UNPLUG_WIRE:
		mixer->connectionBitmap &= ((1 << 9) - 1) ^ (1 << adapter->RxBuf[1]);
		break;
	default:
		break;
	}
	adapter->updateFlag = 0; // reset update flag
}

void AudioSys::update(void) {
// update parameters from GUI if needed
	if (guiadapter.updateFlag) {
		this->updateGUI(&guiadapter);
	}

// get audio samples if needed
	if (saiAdapterCheckUpdate(adapter1) || saiAdapterCheckUpdate(adapter2)) {
		for (uint8_t i = 0; i < SAI_HALF_BUF; i += STEREO) { // until half of RxBuf & TxBuf is not fully processed
			getAudioSamplesInInput(0, adapter1); // load next sample pair in input module
			getAudioSamplesInInput(1, adapter2); // load next sample pair in input module

			for (int j = 0; j < CHANNELS_NUM / 2; j++) {
				Module *module = inputModule[j]->nextModule;
				while (module) {
					module->process();
					module = module->nextModule;
				}
			}

			mixer->process();

			for (int j = 0; j < CHANNELS_NUM / 2; j++) {
				Module *module = mixer->nextModule[j];
				while (module) {
					module->process();
					module = module->nextModule;
				}
			}

			setAudioSamplesToOutput(0, adapter1);
			setAudioSamplesToOutput(1, adapter2);
		}
	}
}

void AudioSys::includeModules() {
	mixer = new Mixer();
	for (int i = 0; i < CHANNELS_NUM; i++) {
		if (i < CHANNELS_NUM / 2) {
			inputModule[i] = new Amp();
			outputModule[i] = new Amp();
			mixer->plugPrev(inputModule[i], i);
			mixer->plugNext(outputModule[i], i);
		}
		modules[i][CHORUSEFFECT] = new Chorus(paramAlloc);
		modules[i][COMPRESSOREFFECT] = new Compressor(paramAlloc);
		modules[i][DELAYEFFECT] = new Delay(paramAlloc);
		modules[i][DISTORTIONEFFECT] = new Distortion(paramAlloc);
		modules[i][FLANGEREFFECT] = new Flanger(paramAlloc);
		modules[i][OVERDRIVEEFFECT] = new Overdrive(paramAlloc);
		modules[i][PHASEREFFECT] = new Phaser(paramAlloc);
		modules[i][REVERBEFFECT] = new Reverb(paramAlloc);
		modules[i][TREMOLOEFFECT] = new Tremolo(paramAlloc);
		modules[i][VIBRATOEFFECT] = new Vibrato(paramAlloc);
	}
}

