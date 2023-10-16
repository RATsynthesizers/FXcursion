/*
 * GUIadapter.c
 *
 *  Created on: May 6, 2023
 *      Author: romte
 */

#include "GUIadapter.h"

IN_RAM_D1_DMA GUIadapter_TypeDef guiadapter;		// global adapter instances

void guiAdapter_Init(GUIadapter_TypeDef *adapter) {
	for (int i = 0; i < GUIUART_INBUF_SIZE; i++) {
		adapter->RxBuf[i] = 0;
	}
	for (int i = 0; i < GUIUART_OUTBUF_SIZE; i++) {
		adapter->TxBuf[i] = 0;
	}
	adapter->msgBufIdx = 0;
	for (int i = 0; i < MAX_MODULES_NUM; i++) {
		adapter->msgBuf[i].fxsIdx = 0;
	}
	adapter->sync = 0;
	HAL_UART_Receive_DMA(&huart2, adapter->RxBuf, GUIUART_INBUF_SIZE);
}

/////////////// UART2 callbacks ///////////////
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart == &huart2 && guiadapter.sync)
		MyGUIcallback(&guiadapter);
}

inline void MyGUIcallback(GUIadapter_TypeDef *adapter) {
	// put recieved msg in msg buf for further processing
	putGUImsg(&guiadapter);
	// increment msg buffer index
	if (guiadapter.msgBufIdx < MAX_MODULES_NUM) {
		guiadapter.msgBufIdx = guiadapter.msgBufIdx + 1;
	} else {
		guiadapter.msgBufIdx = 0;
	}
// recieve another msg
//	HAL_UART_Receive_DMA(&huart2, adapter->RxBuf, GUIUART_INBUF_SIZE);
}

uint8_t guiAdapterCheckUpdate(GUIadapter_TypeDef *adapter) {
	if(adapter->msgBufIdx)
		return 1;
	else
		return 0;
}

inline void putGUImsg(GUIadapter_TypeDef *adapter) {
	uint8_t idx = adapter->msgBufIdx;
	switch (adapter->RxBuf[0]) {
	case REQUEST_PARAM:
		adapter->msgBuf[idx].cmd 				= adapter->RxBuf[0];
		adapter->msgBuf[idx].chain 				= adapter->RxBuf[1];
		adapter->msgBuf[idx].fxs[0].name 		= adapter->RxBuf[2];
		adapter->msgBuf[idx].fxs[0].instance 	= adapter->RxBuf[3];
		adapter->msgBuf[idx].fxs[0].order_index = adapter->RxBuf[4];
	case CHANGE_PARAM:
		adapter->msgBuf[idx].cmd 				= adapter->RxBuf[0];
		adapter->msgBuf[idx].chain 				= adapter->RxBuf[1];
		adapter->msgBuf[idx].fxs[0].name 		= adapter->RxBuf[2];
		adapter->msgBuf[idx].fxs[0].instance 	= adapter->RxBuf[3];
		adapter->msgBuf[idx].fxs[0].order_index = adapter->RxBuf[4];
		adapter->msgBuf[idx].parameterIndex 	= adapter->RxBuf[5];
		adapter->msgBuf[idx].parameterValue 	= adapter->RxBuf[6];
	}
}

void transmitGUImsg(GUIadapter_TypeDef *adapter, uint8_t size) {
	HAL_UART_Transmit_DMA(&huart2, adapter->TxBuf, size);
}








//	switch (adapter->RxBuf[0]) {
//		case REQUEST_PARAM:
//			switch (adapter->RxBuf[2]) {
//			case T_CHORUSEFFECT:
//
//				break;
//			case T_COMPRESSOREFFECT:
//				break;
//			case T_DELAYEFFECT:
//				break;
//			case T_DISTORTIONEFFECT:
//				break;
//			case T_FLANGEREFFECT:
//				break;
//			case T_OVERDRIVEEFFECT:
//				break;
//			case T_PHASEREFFECT:
//				break;
//			case T_REVERBEFFECT:
//				break;
//			case T_TREMOLOEFFECT:
//				break;
//			case T_VIBRATOEFFECT:
//				break;
//			default:
//				break;
//			}
//			break;
//		case CHANGE_PARAM:
//			break;
//		case CHAIN_REWRITE:
//			break;
//		case LOAD_PROJECT:
//			break;
//		case FX_ADDED:
//			break;
//		case CMD_END:
//			break;
//		default:
//			break;
//		}
