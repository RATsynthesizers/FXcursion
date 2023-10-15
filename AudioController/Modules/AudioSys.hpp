/*
 * System.hpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#ifndef AUDIOSYS_HPP_
#define AUDIOSYS_HPP_

#include "../../Drivers/HW/GUIadapter.h"
#include "../../Drivers/HW/MySAIadapter.h"
#include "libModules/Module.hpp"
#include "libModules/Parameter.hpp"
#include "libModules/Mixer.hpp"

#include "Effects/amp/Amp.hpp"
#include "Effects/chorus/Chorus.hpp"
#include "Effects/compressor/Compressor.hpp"
#include "Effects/delay/Delay.hpp"
#include "Effects/distortion/Distortion.hpp"
#include "Effects/flanger/Flanger.hpp"
#include "Effects/overdrive/Overdrive.hpp"
#include "Effects/phaser/Phaser.hpp"
#include "Effects/reverb/Reverb.hpp"
#include "Effects/tremolo/Tremolo.hpp"
#include "Effects/vibrato/Vibrato.hpp"

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

class AudioSys {
public:
	AudioSys() {
		adapter1 = &sai1adapter;
		adapter2 = &sai2adapter;
	}

	virtual void includeModules(void); // user defined module update sequence
	void getAudioSamplesInInput(u8 chnum, SAIadapter_TypeDef *adapter);
	void setAudioSamplesToOutput(u8 chnum, SAIadapter_TypeDef *adapter);
	void updateGUI(GUIadapter_TypeDef *adapter);
	void update(void);
protected:
	Parameter paramAlloc[MAX_SYNTH_PARAMS];
	Module *inputModule[CHANNELS_NUM/2];
	Module *outputModule[CHANNELS_NUM/2];
	Module *modules[CHANNELS_NUM][EXISTING_MODULES];
	Mixer *mixer;

	SAIadapter_TypeDef *adapter1;
	SAIadapter_TypeDef *adapter2;
};

extern AudioSys audioSystem; // global audio instance

#endif /* AUDIOSYS_HPP_ */
