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
	HAL_UART_Receive_DMA(GUIUART_UART, adapter->RxBuf, GUIUART_INBUF_SIZE);
}

/////////////// UART2 callbacks ///////////////
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == GUIUART_UART)
		guiadapter.updateFlag = 1;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	if (huart == GUIUART_UART) {
		HAL_UART_AbortReceive(GUIUART_UART);
		guiAdapter_Init(&guiadapter);
	}
}
