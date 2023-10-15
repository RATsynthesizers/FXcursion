/*
 * AudioTranfer.h
 *
 *  Created on: Sep 14, 2023
 *      Author: user
 */

#ifndef AUDIOTRANSFER_AUDIOTRANSFER_H_
#define AUDIOTRANSFER_AUDIOTRANSFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define READAUDIOBUF_SIZE  4
#define WRITEAUDIOBUF_SIZE 0x2000

#include "cmsis_os.h"
#include "string.h"
#include "spi.h"

void audioTransfer_Init();

extern int16_t readAudioBuffer[];
extern int16_t writeAudioBuffer[];

#ifdef __cplusplus
}
#endif


#endif /* AUDIOTRANSFER_AUDIOTRANSFER_H_ */
