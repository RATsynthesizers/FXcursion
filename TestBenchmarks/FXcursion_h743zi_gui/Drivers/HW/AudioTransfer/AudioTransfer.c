/*
 * AudioTransfer.c
 *
 *  Created on: Sep 14, 2023
 *      Author: user
 */

#include "AudioTransfer.h"

volatile uint8_t looper_half = 0;
IN_RAM_D1_DMA int16_t readAudioBuffer[READAUDIOBUF_SIZE];
volatile uint16_t writeAudioPointer = 0;
 int16_t writeAudioBuffer[WRITEAUDIOBUF_SIZE];

extern osSemaphoreId_t AudioTransferSemaphoreHandle;

void audioTransfer_Init() {
	for (int i = 0; i < WRITEAUDIOBUF_SIZE; i++)
		writeAudioBuffer[i] = 0;
	for (int i = 0; i < READAUDIOBUF_SIZE; i++)
		readAudioBuffer[i] = 0;

	HAL_SPI_Receive_DMA(&(AUDIO_TRANSFER_SPI), (uint8_t*) readAudioBuffer, READAUDIOBUF_SIZE);
}

uint16_t readingBufTime = 0;
uint32_t temp = 0;
int16_t checking;

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == &(AUDIO_TRANSFER_SPI)) {
		for (int i = 0; i < READAUDIOBUF_SIZE; i++)
		{
			checking = readAudioBuffer[i];
			writeAudioBuffer[writeAudioPointer + i] = readAudioBuffer[i];
		}

		writeAudioPointer += READAUDIOBUF_SIZE;

		if (writeAudioPointer == WRITEAUDIOBUF_SIZE / 2) {
			readingBufTime = HAL_GetTick() - temp;
			temp = HAL_GetTick();

			looper_half = 0;
			osSemaphoreRelease(AudioTransferSemaphoreHandle);
		}else if (writeAudioPointer == WRITEAUDIOBUF_SIZE) {
			readingBufTime = HAL_GetTick() - temp;
			temp = HAL_GetTick();
			writeAudioPointer = 0;
			looper_half = 1;
			osSemaphoreRelease(AudioTransferSemaphoreHandle);
		}
	}
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
	if (hspi == &(AUDIO_TRANSFER_SPI)) {
		HAL_SPI_Abort(&(AUDIO_TRANSFER_SPI));
		audioTransfer_Init();
	}
}
