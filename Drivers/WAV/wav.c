/*
 * wav.c
 *
 *  Created on: Sep 17, 2022
 *      Author: Predtech
 */

#include "wav.h"

WAVE_HEADER RecorderWaveHeader = {

	{{0x52, 0x49, 0x46, 0x46},

	(sizeof(WAVE_HEADER) - 8),//The size of the entire wave file, initial value

	{0x57, 0x41, 0x56, 0x45}},

	{{0x66, 0x6d, 0x74, 0x20},

    16,

	{1,//Encoding method. Linear PCM encoding

	2,//Stereo

	48000,//Sampling rate is 48k

	192000,//4 bytes per stereo sample

	4,//4 	 bytes per stereo sample

	16}},//each sample requires 16 bits

	{{0x64, 0x61, 0x74, 0x61},

	0} //data length is initialized to 0}

};
