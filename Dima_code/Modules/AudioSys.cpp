/*
 * System.cpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "../Modules/AudioSys.hpp"
#include "cmsis_os.h"
#include "Handy.h"

//__weak void includeModules(void) {;}

extern osSemaphoreId_t requestParamValuesSemHandle;
extern osMessageQueueId_t UpdateAudioParamsQueueHandle;
extern osMessageQueueId_t GaugeValuesQueueHandle;
extern osMessageQueueId_t AllGaugeValuesQueueHandle;

void AudioSys::getAudioSamplesInInput(SAIadapter_TypeDef *adapter) {
	for (u8 lr = 0; lr < STEREO; lr++)		// convert input, left & right
		inputModule->output[lr] = ((float) saiAdapter_getNextSample(adapter))
				/ MY_INT16_MAX;
}

void AudioSys::setAudioSamplesToOutput(SAIadapter_TypeDef *adapter) {
	for (u8 lr = 0; lr < STEREO; lr++) {		// convert output, left & right
		int16_t tmp = (int16_t) (outputModule->output[lr] * MY_INT16_MAX);
		saiAdapter_setNextSample(adapter, tmp);
	}
}

void AudioSys::update(void) {

	ALLGAUGEVALUES_OBJ_t allGaugeValues_msg;
	osStatus_t status = osSemaphoreAcquire(requestParamValuesSemHandle, 0);
	if (status == osOK) {
		for (uint8_t i = 0; i < 4; i++)
			allGaugeValues_msg.value[i] = modules[1]->p[modules[1]->paramNum - 1
					- i].val;

		osMessageQueuePut(AllGaugeValuesQueueHandle, &allGaugeValues_msg, 0, 0);
	}

	UPDATEAUDIOPARAMS_OBJ_t updateAudioParams_msg;
	status = osMessageQueueGet(UpdateAudioParamsQueueHandle,
			&updateAudioParams_msg, NULL, 0U);
	if (status == osOK) {
		UPDATEGAUGEVALUES_OBJ_t updateGaugeValues_msg;
		float newValue;
		switch (updateAudioParams_msg.audioModule) {
		case MODULE_REVERB:
			switch (updateAudioParams_msg.parameter) {
			case PARAM_DECAY:
				newValue = modules[1]->p[modules[1]->paramNum - 1].val
						+ updateAudioParams_msg.value;
				if (newValue < 0)
					newValue = 0;
				else if (newValue > 1)
					newValue = 1;

				modules[1]->p[modules[1]->paramNum - 1].val = newValue;
				updateGaugeValues_msg.gaugeNum = 0;
				updateGaugeValues_msg.value = newValue;
				osMessageQueuePut(GaugeValuesQueueHandle,
						&updateGaugeValues_msg, 0, 0);
				break;
			case PARAM_REVMIX:
				newValue = modules[1]->p[modules[1]->paramNum - 2].val
						+ updateAudioParams_msg.value;
				if (newValue < 0)
					newValue = 0;
				else if (newValue > 1)
					newValue = 1;

				modules[1]->p[modules[1]->paramNum - 2].val = newValue;
				updateGaugeValues_msg.gaugeNum = 1;
				updateGaugeValues_msg.value = newValue;
				osMessageQueuePut(GaugeValuesQueueHandle,
						&updateGaugeValues_msg, 0, 0);
				break;
			}
			break;
		case MODULE_MIX:
			break;
		}
	}

	for (u8 i = 0; i < NUM_OF_SAI_ADAPTERS; i++)
		if (saiAdapterCheckUpdate(adapter[i]))
			for (uint8_t j = 0; j < SAI_HALF_BUF; j += STEREO) {// until half of RxBuf & TxBuf is not fully processed
				getAudioSamplesInInput(adapter[i]);	// load next sample pair in input module
				for (uint8_t m = 0; m < MAX_MODULES_NUM; m++)
					modules[m]->process();
				outputModule->process();
				setAudioSamplesToOutput(adapter[i]);
			}
}
