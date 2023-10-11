/*
 * ly68l6400.c
 *
 *  Created on: Aug 31, 2022
 *      Author: romte
 */

#include "ly68l6400.h"

PsramSpiTypeDef psramSpi;

void PsramReset(void) {
	uint8_t cmd = PSRAM_RESET_ENABLE;
	HAL_SPI_Transmit(&hspi2, &cmd, 1, 1);
	cmd = PSRAM_PERFORM_RESET;
	HAL_SPI_Transmit(&hspi2, &cmd, 1, 1);
	HAL_Delay(1);
}

void PsramWrite(uint32_t addr, uint8_t *txData, uint8_t size) {
	psramSpi.txBuf[0] = PSRAM_WRITE_CMD; // cmd
	psramSpi.txBuf[1] = (uint8_t) ((addr & 0xFF0000) >> 16); // addr hi
	psramSpi.txBuf[2] = (uint8_t) ((addr & 0x00FF00) >> 8 ); // addr mid
	psramSpi.txBuf[3] = (uint8_t) ((addr & 0x0000FF)      ); // addr lo
	if(size > PSRAM_SPI_PACKET_SIZE) {
		for(uint32_t k = 0; k < (size / PSRAM_SPI_PACKET_SIZE + 1); k++) {
			for(uint8_t i = 0; i < size; i++)
				psramSpi.txBuf[PSRAM_SPI_PREAMBLE_LEN+i] = *(txData + i + k);
			HAL_SPI_Transmit(&hspi2, psramSpi.txBuf, size+PSRAM_SPI_PREAMBLE_LEN, 1);
		}
	} else {
		for(uint8_t i = 0; i < size; i++)
			psramSpi.txBuf[PSRAM_SPI_PREAMBLE_LEN+i] = *(txData + i);
		HAL_SPI_Transmit(&hspi2, psramSpi.txBuf, size+PSRAM_SPI_PREAMBLE_LEN, 1);
	}

}
uint8_t* PsramRead(uint32_t addr, uint8_t size) {
	psramSpi.txBuf[0] = PSRAM_READ_CMD; // cmd
	psramSpi.txBuf[1] = (uint8_t) ((addr & 0xFF0000) >> 16); // addr hi
	psramSpi.txBuf[2] = (uint8_t) ((addr & 0x00FF00) >> 8 ); // addr mid
	psramSpi.txBuf[3] = (uint8_t) ((addr & 0x0000FF)      ); // addr lo
	HAL_SPI_TransmitReceive(&hspi2, psramSpi.txBuf, psramSpi.rxBuf, PSRAM_SPI_BUF_SIZE, 1);
	return psramSpi.txBuf + 4; // return pointer to valid rx data
}

