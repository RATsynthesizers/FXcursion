/*
 * LY68L6400.c
 *
 *  Created on: Feb 11, 2023
 *      Author: rorka
 */

#include "LY68L6400.h"

PSRAM_TypeDef psram;	// psram adapter instance

// test function
// size < MAX_READ_CHUNK_SIZE/4 of uint16_t
void PsramWrite(PSRAM_TypeDef* psram, uint32_t* pData, uint8_t size, uint32_t addr) {
	psram->fakeTxBuf[0] = (PSRAM_SLOW_WRITE << 24) | addr;		// cmd 1 byte + addr 3 bytes
	for(uint16_t i = 0; i < size; i++ )
		psram->fakeTxBuf[1+i] =	pData[i];
	psram->readInProgress = 0;
	psram->writeInProgress = 1;
	HAL_SPI_Transmit_DMA(&PSRAM_HSPI, (uint8_t*)(psram->fakeTxBuf), size + 1); // cmd + addr + data send
}

// test function
// size < MAX_READ_CHUNK_SIZE/2 of uint16_t
void PsramRead(PSRAM_TypeDef* psram, uint8_t size, uint32_t addr) {
	psram->fakeTxBuf[0] = (PSRAM_SLOW_READ << 24) | addr;		// cmd 1 byte + addr 3 bytes
	for(uint16_t i = 0; i < size; i++ )
		psram->fakeTxBuf[1+i] =	0; // clear fake tx buf
	psram->writeInProgress = 0;
	psram->readInProgress = 1;
	HAL_SPI_TransmitReceive_DMA(&PSRAM_HSPI, (uint8_t*)(psram->fakeTxBuf), (uint8_t*)(psram->rxBuf), size + 1);
}




