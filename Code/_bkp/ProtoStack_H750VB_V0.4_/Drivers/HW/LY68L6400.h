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

#define PSRAM_SLOW_WRITE 	0x02
#define PSRAM_SLOW_READ		0x03
#define PSRAM_FAST_READ 	0x0B


void PsramWrite(uint16_t* pData, uint16_t size, uint32_t addr);
void PsramRead(uint16_t* pBuf, uint16_t size, uint32_t addr);


#ifdef __cplusplus
}
#endif

#endif /* HW_LY68L6400_H_ */
