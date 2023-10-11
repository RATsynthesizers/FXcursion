/*
 * Modules.cpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "AudioSysObjects.hpp"

	IN_RAMD1 AudioSys audioSystem; 	// global audio instance

	IN_RAMD1 Parameter paramAlloc[MAX_SYNTH_PARAMS];



	IN_RAMD1 Amp audioInput1(paramAlloc);
	IN_RAMD1 Amp audioOutput1(paramAlloc);
	IN_RAMD1 Amp amp1(paramAlloc);

	IN_RAMD1 Wire audioInput1Wire(&audioInput1, &amp1);
	IN_RAMD1 Wire audioOutput1Wire(&amp1, &audioOutput1);

//	IN_RAMD1 Reverb rev(paramAlloc);
//	IN_RAMD1 Wire amp2rev(&amp1, &rev);



	// module update sequence (update MAX_MODULES_NUM)
	void AudioSys::includeModules() {
		this->modules[0] = &amp1;
//		this->modules[1] = &rev;
		this->inputModule = &audioInput1;
		this->outputModule = &audioOutput1;
	}

//#include "../Drivers/HW/LY68L6400.h"
//void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
//	if(hspi == &PSRAM_HSPI) {
//		// perform dma transfer sequence
//		for (u8 lr = 0; lr < STEREO; lr++) {
//			for (u8 k = 0; k < REV_ALLP; k++) {
//				if (rev.APs[lr][k].dma_tx_ready_flag && psram.readInProgress == 0) {
//					// if tx of idx1 smpl cplt, launch rx of idx2 smpl
//					PsramRead(&psram, 1, rev.APs[lr][k].buffer_PSRAM_addr + 4*rev.APs[lr][k].index2);
//					// wait for next callback
////					--rev.APs[lr][k].dma_tx_ready_flag;
//					return;
//				}
//			}
//		}
//
//	}
//}
//
//void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
//	if(hspi == &PSRAM_HSPI) {
//		// perform dma transfer sequence
//		for (u8 lr = 0; lr < STEREO; lr++) {
//			for (u8 k = 0; k < REV_ALLP; k++) {
//				if (rev.APs[lr][k].dma_rx_ready_flag == 2) {
//					// if rx of idx2 smpl cplt, copy data from psarm rxBuf & launch rx of idxFrac smpl
//					rev.APs[lr][k].buffer_index2 = (float) psram.rxBuf[1];
//					PsramRead(&psram, 1, rev.APs[lr][k].buffer_PSRAM_addr + 4*(rev.APs[lr][k].index2+1));
//					// wait for next callback
//					--rev.APs[lr][k].dma_rx_ready_flag;
//					return;
//				}	else if(rev.APs[lr][k].dma_rx_ready_flag == 1) {
//					// if rx of idxFrac smpl cplt, copy data from psarm rxBuf & launch tx of idx1 smpl of next AP
//					rev.APs[lr][k].buffer_indexFrac = (float) psram.rxBuf[1];
//						if(k+1 == REV_ALLP) { // next AP
//							if(lr == 0)
//								PsramWrite(&psram, (uint32_t*)(&rev.APs[lr+1][0].buffer_index1), 1, rev.APs[lr+1][0].buffer_PSRAM_addr + 4*rev.APs[lr+1][0].index1);
//						} else {
//							PsramWrite(&psram, (uint32_t*)(&rev.APs[lr][k+1].buffer_index1), 1, rev.APs[lr][k+1].buffer_PSRAM_addr + 4*rev.APs[lr][k+1].index1);
//						}
//					// wait for next callback
//					--rev.APs[lr][k].dma_rx_ready_flag;
//					return;
//				}
//			}
//		}
//	}
//}
