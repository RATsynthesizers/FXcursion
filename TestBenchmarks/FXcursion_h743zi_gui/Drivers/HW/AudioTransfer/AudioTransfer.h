/*
 * AudioTranfer.h
 *
 *  Created on: Sep 14, 2023
 *      Author: user
 */

#ifndef HW_AUDIOTRANSFER_AUDIOTRANSFER_H_
#define HW_AUDIOTRANSFER_AUDIOTRANSFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define READAUDIOBUF_SIZE  4
#define WRITEAUDIOBUF_SIZE 0x2000

#include "cmsis_os.h"
#include "string.h"
#include "spi.h"


#define AUDIO_TRANSFER_SPI hspi1

void audioTransfer_Init();

extern int16_t readAudioBuffer[];
extern int16_t writeAudioBuffer[];

#ifdef __cplusplus
}
#endif


#endif /* HW_AUDIOTRANSFER_AUDIOTRANSFER_H_ */
