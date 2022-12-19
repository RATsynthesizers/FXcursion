/*
 * SysGlobals.hpp
 *
 *  Created on: Nov 11, 2020
 *      Author: romte
 */

#ifndef SYSGLOBALS_H_
#define SYSGLOBALS_H_

//////////////////// S Y S   S E T T I N G S /////////////////////
#define SAMPLINGFREQ    (48000.0f)			//the sampling frequency in Hz
#define SAI_IO_BUF_SIZE 128					// only even (for doubleBuffer mode)
#define MIDI_MESSAGE_MAX_LEN 8				// uart4 buf len for midi

/////////// MODULES SETTINGS /////////////
#define MAX_SYNTH_PARAMS        256
#define MAX_MODULES_NUM			2




#endif /* SYSGLOBALS_H_ */
