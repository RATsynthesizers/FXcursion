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
#define SAI_IO_BUF_SIZE 8					// only even (for doubleBuffer mode)
#define MIDI_MESSAGE_MAX_LEN 8				// uart4 buf len for midi

/////////// MODULES SETTINGS /////////////
#define MAX_SYNTH_PARAMS        128
#define MAX_MODULES_NUM			32
#define MAX_MODULES_IN_CHAIN	8
#define IO_MODULES_NUM			2

/////////// MODULES NAMES & DEFAULT ORDER /////////////
typedef enum {
	T_CHORUSEFFECT,
	T_COMPRESSOREFFECT,
	T_DELAYEFFECT,
	T_DISTORTIONEFFECT,
	T_FLANGEREFFECT,
	T_OVERDRIVEEFFECT,
	T_PHASEREFFECT,
	T_REVERBEFFECT,
	T_TREMOLOEFFECT,
	T_VIBRATOEFFECT
}GUIfxName_TypeDef;

#endif /* SYSGLOBALS_H_ */
