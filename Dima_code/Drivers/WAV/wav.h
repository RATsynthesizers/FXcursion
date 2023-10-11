/*
 * wav.h
 *
 *  Created on: Sep 17, 2022
 *      Author: Predtech
 */

#ifndef __WAV_H__

#define __WAV_H__

#ifdef __cplusplus

extern "C" {

#endif

#include "stm32h7xx_hal.h"

//Resource interchange file format RIFF, tree structure, the basic unit is chunk. The entire file is composed of chunks

typedef struct RIFF_HEADER {

   uint8_t ChunkID[4];

   uint32_t Riff_Size;//Record the size of the entire RIFF file. In addition to the two variables ID and Size

   uint8_t Riff_Format[4];

} RIFF_HEADER;

typedef struct WAVE_FORMAT {

	uint16_t FormatTag; //Sound format code

	uint16_t Channels; //Sound channel

	uint32_t SamplesPerSec; //Sampling rate

	uint32_t AvgBytesPerSec; //=sampling rate*block to unit

	uint16_t BlockAlign; //Block alignment unit = number of bits required for each sample * sound channel/8

	uint16_t BitsPerSample; //The number of bits required for each sample

} WAVE_FORMAT;



typedef struct FMT_CHUNK {

	uint8_t Fmt_ID[4];

    uint32_t Fmt_Size;//Record the size of fmt

    WAVE_FORMAT WaveFormat;

} FMT_CHUNK;



typedef struct DATA_CHUNK {

	uint8_t Data_ID[4];

    uint32_t Data_Size;//Record the size of the data area

} DATA_CHUNK;



typedef struct WAVE_HEADER {

    RIFF_HEADER RiffHeader;

    FMT_CHUNK   FmtChunk;

    DATA_CHUNK  DataChunk;

} WAVE_HEADER;

#ifdef __cplusplus

}

#endif

#endif /*__WAV_H__*/
