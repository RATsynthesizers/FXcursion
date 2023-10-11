/*
 * MySAIadapter.c
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "MySAIadapter.h"


SAIadapter_TypeDef sai1adapter;		// global adapter instances
SAIadapter_TypeDef sai2adapter;

void saiAdapter_Init(SAIadapter_TypeDef* adapter, HW_SAI_Instance sai_instatnce) {
	// enable SAI rx tx (sai1 A-rx, B-tx, sai2 A-rx, B-tx)
	if(sai_instatnce == HW_SAI1) {
		HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t*) adapter->RxData, SAI_IO_BUF_SIZE);
		HAL_SAI_Transmit_DMA(&hsai_BlockB1, (uint8_t*) adapter->TxData, SAI_IO_BUF_SIZE);
	}
	if(sai_instatnce == HW_SAI2) {
		HAL_SAI_Receive_DMA(&hsai_BlockA2, (uint8_t*) adapter->RxData, SAI_IO_BUF_SIZE);
		HAL_SAI_Transmit_DMA(&hsai_BlockB2, (uint8_t*) adapter->TxData, SAI_IO_BUF_SIZE);
	}

	// disable irq and init all rx flags
	__disable_irq();
	adapter->saiInstance = sai_instatnce;
	adapter->RxFlag = 0;			// rx & tx at the same time -> use only rx flags
	adapter->RxHalfFlag = 0;
	adapter->BufOffset = 0;
	adapter->rxBufPosition = 0;
	adapter->txBufPosition = 0;
	for(int i = 0; i < SAI_IO_BUF_SIZE; i++) {
		adapter->RxData[i] = 0;
		adapter->TxData[i] = 0;
	}
	__enable_irq();


}



int16_t saiAdapter_getNextSample(SAIadapter_TypeDef* adapter) {
	int16_t tmp =  adapter->RxData[adapter->BufOffset + adapter->rxBufPosition];
	adapter->rxBufPosition+=1;
	if(adapter->rxBufPosition >= SAI_HALF_BUF) {
		adapter->rxBufPosition = 0;
	}
	return tmp;
}

void saiAdapter_setNextSample(SAIadapter_TypeDef* adapter, int16_t sample) {
	adapter->TxData[adapter->BufOffset + adapter->txBufPosition] = sample;
	adapter->txBufPosition+=1;
	if(adapter->txBufPosition >= SAI_HALF_BUF) {
		adapter->txBufPosition = 0;
	}
}

HW_SAI_UpdateStatus saiAdapterCheckUpdate(SAIadapter_TypeDef* adapter) {
	if(adapter->RxHalfFlag) {
		adapter->RxHalfFlag = 0;
		adapter->BufOffset = 0;
		return HW_SAI_SWITCH_BUF;
	}
	if(adapter->RxFlag) {
		adapter->RxFlag = 0;
		adapter->BufOffset = SAI_HALF_BUF;
		return HW_SAI_SWITCH_BUF;
	}
	return HW_SAI_CURRENT_BUF;
}

/////////////// SAI cplt & halfCplt callbacks ///////////////

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai) {
	if(hsai == &hsai_BlockA1) {
		sai1adapter.RxHalfFlag=1;//++;
	}
	if(hsai == &hsai_BlockA2) {
		sai2adapter.RxHalfFlag=1;//++;
	}
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai) {
	if(hsai == &hsai_BlockA1) {
		sai1adapter.RxFlag=1;//++;
	}
	if(hsai == &hsai_BlockA2) {
		sai2adapter.RxFlag=1;//++;
	}


}

