/*
 * Modules.hpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#ifndef AUDIOSYSOBJECTS_HPP_
#define AUDIOSYSOBJECTS_HPP_

#include <Effects/delay/Delayy.hpp>
#include "libModules/Parameter.hpp"
#include "libModules/Wire.hpp"

#include "Effects/amp/Amp.hpp"
#include "Effects/chorus/Chorus.hpp"
#include "Effects/compressor/Compressor.hpp"
#include "Effects/distortion/Distortion.hpp"
#include "Effects/flanger/Flanger.hpp"
#include "Effects/overdrive/Overdrive.hpp"
#include "Effects/phaser/Phaser.hpp"
#include "Effects/reverb/Reverb.hpp"
#include "Effects/tremolo/Tremolo.hpp"
#include "Effects/vibrato/Vibrato.hpp"

#include "AudioSys.hpp"

/////////// MODULES NAMES & DEFAULT ORDER /////////////
typedef enum {
	CHORUSEFFECT,
	COMPRESSOREFFECT,
	DELAYEFFECT,
	DISTORTIONEFFECT,
	FLANGEREFFECT,
	OVERDRIVEEFFECT,
	PHASEREFFECT,
	REVERBEFFECT,
	TREMOLOEFFECT,
	VIBRATOEFFECT
}GUIfxName_TypeDef;

extern Amp * audioInput[CHANNELS_NUM / 2];
extern Amp * audioOutput[CHANNELS_NUM / 2];

extern Wire * defaultAudioWire[CHANNELS_NUM / 2];

extern Module * module[CHANNELS_NUM][EXISTING_MODULES];

extern Parameter paramAlloc[MAX_SYNTH_PARAMS];
extern AudioSys audioSystem; // global audio instance

#endif /* AUDIOSYSOBJECTS_HPP_ */
