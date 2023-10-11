/*
 * LY68L6400.c
 *
 *  Created on: Feb 11, 2023
 *      Author: rorka
 */

#include "LY68L6400.h"

void PsramWrite(uint16_t* pData, uint16_t size, uint32_t addr) {
	uint16_t tmp[size + 2];		// size + 1byte cmd + 3byte addr
	tmp[0] = (PSRAM_SLOW_WRITE << 8) | (0xFF&(addr >> 16));		// cmd + addr (higher 8 bits)
	tmp[1] = 0xFFFF&(addr);										// addr (lower 16 bits)
	for(uint16_t i = 0; i < size; i++)
		tmp[i+2] = pData[i];

	HAL_SPI_Transmit(&hspi1, (uint8_t*)&tmp, size + 2, 1);		// cmd + addr + data send
}

// real buf size should be size + 2 of u16
void PsramRead(uint16_t* pBuf, uint16_t size, uint32_t addr) {
	uint16_t tmp[size + 2];		// size + 1byte cmd + 3byte addr
	tmp[0] = (PSRAM_SLOW_READ << 8) | (0xFF&(addr >> 16));		// cmd + addr (higher 8 bits)
	tmp[1] = 0xFFFF&(addr);										// addr (lower 16 bits)
	for(uint16_t i = 0; i < size; i++)
		tmp[i+2] = 0;

	HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&tmp, (uint8_t*)pBuf, size + 2, 1);

}

