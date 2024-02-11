#include "GUIadapter.h"

IN_RAM_D1_DMA GUIadapter_TypeDef guiadapter;		// global adapter instances

void guiAdapter_Init(GUIadapter_TypeDef *adapter) {
	for (int i = 0; i < GUIUART_INBUF_SIZE; i++) {
		adapter->RxBuf[i] = 0;
	}
	for (int i = 0; i < GUIUART_OUTBUF_SIZE; i++) {
		adapter->TxBuf[i] = 0;
	}

	adapter->updateFlag = 0;
	HAL_UART_Receive_DMA(&huart2, adapter->RxBuf, GUIUART_INBUF_SIZE);
}

/////////////// UART2 callbacks ///////////////
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2)
		guiadapter.updateFlag = 1;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2) {
		HAL_UART_AbortReceive(&huart2);
		guiAdapter_Init(&guiadapter);
	}
}
