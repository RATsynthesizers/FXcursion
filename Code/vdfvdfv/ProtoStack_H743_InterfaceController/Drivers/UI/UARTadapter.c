#include "UARTadapter.h"

IN_RAM_D1_DMA UARTadapter_TypeDef audio_uart_adapter;		// global adapter instances

void UARTadapter_Init(UARTadapter_TypeDef *adapter) {
	for (int i = 0; i < UART_INMSG_SIZE; i++) {
		adapter->RxBuf[i] = 0;
	}
	for (int i = 0; i < UART_OUTMSG_SIZE; i++) {
		adapter->TxBuf[i] = 0;
	}
	adapter->update_flag = 0;
	HAL_UART_Receive_DMA(&huart2, adapter->RxBuf, UART_INMSG_SIZE);
}

uint8_t UARTadapterCheckUpdate(UARTadapter_TypeDef *adapter) {
	return adapter->update_flag;
}

void transmitUARTmsg(UARTadapter_TypeDef *adapter) {
	HAL_UART_Transmit_DMA(&huart2, adapter->TxBuf, UART_OUTMSG_SIZE);
}

void getUARTMsgRx(UARTadapter_TypeDef* adapter) {
	for(uint8_t i = 0; i < UART_INMSG_SIZE; i++)
		adapter->msgIn.params[i] = adapter->RxBuf[i];
	adapter->update_flag = 1;
}

void prepareUARTMsgTx(UARTadapter_TypeDef* adapter) {
	// copy msg to TxBuf
		adapter->TxBuf[0] = adapter->msgOut.cmd;
		adapter->TxBuf[1] = adapter->msgOut.chain;
		adapter->TxBuf[2] = adapter->msgOut.fxIdx;
		adapter->TxBuf[3] = adapter->msgOut.parameterIndex;
		adapter->TxBuf[4] = adapter->msgOut.parameterValue;

}

/////////////// UART2 callbacks ///////////////
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2)
		getUARTMsgRx(&audio_uart_adapter);
}

// reset DMA buffers & restart DMA rx when other MCU is rebooting
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	huart->RxXferCount = 0;
    UARTadapter_Init(&audio_uart_adapter);
}
