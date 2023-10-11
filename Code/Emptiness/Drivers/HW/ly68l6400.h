/*
 * ly68l6400.h
 *
 *  Created on: Aug 31, 2022
 *      Author: romte
 */

#ifndef HW_LY68L6400_H_
#define HW_LY68L6400_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"
#include "spi.h"

////////// S E T T I N G S ////////////////
#define PSRAM_SPI_PACKET_SIZE	8
#define PSRAM_SPI_PREAMBLE_LEN	4 // 1 byte CMD + 3 byte addr
#define PSRAM_SPI_BUF_SIZE 		(PSRAM_SPI_PACKET_SIZE + PSRAM_SPI_PREAMBLE_LEN)
///////////////////////////////////////////

#define PSRAM_WRITE_CMD		0x02
#define PSRAM_READ_CMD		0x03
#define PSRAM_FAST_READ_CMD	0x0B

#define PSRAM_RESET_ENABLE	0x66
#define PSRAM_PERFORM_RESET	0x99
#define PSRAM_SET_BRST_LEN	0xC0
#define PSRAM_READ_ID		0x9F

#define PSRAM_FAST_READ_DUMMY	8


typedef struct {
	uint8_t txBuf[PSRAM_SPI_BUF_SIZE];
	uint8_t rxBuf[PSRAM_SPI_BUF_SIZE];
}PsramSpiTypeDef;

void PsramReset(void);
void PsramWrite(uint32_t addr, uint8_t *txData, uint8_t size);
uint8_t* PsramRead(uint32_t addr, uint8_t size);

extern PsramSpiTypeDef psramSpi;;

#ifdef __cplusplus
}
#endif
#endif /* HW_LY68L6400_H_ */
