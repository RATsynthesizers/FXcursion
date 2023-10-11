/*
 * LY68L6400.h
 *
 *  Created on: Feb 11, 2023
 *      Author: rorka
 */

#ifndef HW_LY68L6400_H_
#define HW_LY68L6400_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"
#include "spi.h"

#define PSRAM_HSPI hspi1

#define PSRAM_SLOW_WRITE 	0x02
#define PSRAM_SLOW_READ		0x03
#define PSRAM_FAST_READ 	0x0B

#define MAX_READ_CHUNK_SIZE 128		// bytes


typedef struct {
	uint32_t fakeTxBuf[MAX_READ_CHUNK_SIZE/4+1];
	uint32_t rxBuf[MAX_READ_CHUNK_SIZE/4+1];
	uint8_t readInProgress;
	uint8_t writeInProgress;
}PSRAM_TypeDef;		// psram adapter data

extern PSRAM_TypeDef psram;	// psram adapter instance

void PsramWrite(PSRAM_TypeDef* psram, uint32_t* pData, uint8_t size, uint32_t addr);
void PsramRead(PSRAM_TypeDef* psram, uint8_t size, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* HW_LY68L6400_H_ */
