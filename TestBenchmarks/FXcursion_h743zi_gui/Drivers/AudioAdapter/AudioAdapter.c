/*
 * AudioAdapter.c
 *
 *  Created on: 8 июл. 2023 г.
 *      Author: Predtech
 */

#include "AudioAdapter.h"

IN_RAM_D1_DMA AudioAdapter_TypeDef audioAdapter;	// global adapter instances

#define AUDIOUART_INBUF_SIZE  4
#define AUDIOUART_OUTBUF_SIZE 5

void audioAdapter_Init(AudioAdapter_TypeDef *adapter) {
	for (int i = 0; i < AUDIOUART_INBUF_SIZE; i++)
		adapter->RxBuf[i] = 0;
	for (int i = 0; i < AUDIOUART_OUTBUF_SIZE; i++)
		adapter->TxBuf[i] = 0;

	adapter->updateFlag = 0;

	HAL_UART_Receive_DMA(&huart2, adapter->RxBuf, AUDIOUART_INBUF_SIZE);
}

/////////////// UART2 callbacks ///////////////
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2)
		audioAdapter.updateFlag = 1;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2) {
		HAL_UART_AbortReceive(&huart2);
		audioAdapter_Init(&audioAdapter);
	}
}

