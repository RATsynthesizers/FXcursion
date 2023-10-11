/*
 * Looper.cpp
 *
 *  Created on: Sep 18, 2022
 *      Author: Predtech
 */

#include "Looper.hpp"
#include "cmsis_os.h"
#include "fatfs.h"

extern uint16_t writeAudioBufferHead;
extern int16_t writeAudioBuffer[_MAX_SS];

extern osSemaphoreId_t writeSDSemHandle;
extern osMessageQueueId_t LooperQueueHandle;

void Looper::process(void) {

	for (u8 lr = 0; lr < STEREO; lr++)
		output[lr] = *input[lr][0];

	osStatus_t status = osSemaphoreAcquire(writeSDSemHandle, 0);
	if(status == osOK)
	{
		if(recordStatus == 0)
		{
			writeAudioBufferHead = 0;
			recordStatus = 1;
		}
		else
		{
			recordStatus = 0;
			looper_msg.recording = 0;
			osMessageQueuePut(LooperQueueHandle, &looper_msg, 0U, 0);
		}
	}

	if(recordStatus == 1)
	{
		if(writeAudioBufferHead == _MAX_SS / 2)
		{
			looper_msg.recording = 1;
			looper_msg.half = 0;
			osMessageQueuePut(LooperQueueHandle, &looper_msg, 0U, 0);
		} else if(writeAudioBufferHead >= _MAX_SS)
		{
			writeAudioBufferHead = 0;
			looper_msg.recording = 1;
			looper_msg.half = 1;
			osMessageQueuePut(LooperQueueHandle, &looper_msg, 0U, 0);
		}

		for(uint8_t lr = 0; lr < STEREO; lr++)
			writeAudioBuffer[writeAudioBufferHead + lr] = (int16_t) (output[lr] * MY_INT16_MAX);
		writeAudioBufferHead += STEREO;

	}
}
