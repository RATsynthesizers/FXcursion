#include "AudioSysObjects.hpp"

IN_DTCMRAM AudioSys audioSystem; 	// global audio instance
IN_DTCMRAM Parameter paramAlloc[MAX_SYNTH_PARAMS];

IN_DTCMRAM Amp * audioInput[CHANNELS_NUM / 2];
IN_DTCMRAM Amp * audioOutput[CHANNELS_NUM / 2];

IN_DTCMRAM Wire * defaultAudioWire[CHANNELS_NUM / 2];

void AudioSys::includeModules() {

	for (int i = 0; i < CHANNELS_NUM / 2; i++) {

	}

	for (int i = 0; i < CHANNELS_NUM; i++) {
		if (i < CHANNELS_NUM / 2) {
			audioInput[i] = new Amp(paramAlloc);
			audioOutput[i] = new Amp(paramAlloc);
			defaultAudioWire[i] = new Wire(audioInput[i], audioOutput[i]);
		}

		modules[i][CHORUSEFFECT] = new Chorus(paramAlloc);
		modules[i][COMPRESSOREFFECT] = new Compressor(paramAlloc);
		modules[i][DELAYEFFECT] = new Delayy(paramAlloc);
		modules[i][DISTORTIONEFFECT] = new Distortion(paramAlloc);
		modules[i][FLANGEREFFECT] = new Flanger(paramAlloc);
		modules[i][OVERDRIVEEFFECT] = new Overdrive(paramAlloc);
		modules[i][PHASEREFFECT] = new Phaser(paramAlloc);
		modules[i][REVERBEFFECT] = new Reverb(paramAlloc);
		modules[i][TREMOLOEFFECT] = new Tremolo(paramAlloc);
		modules[i][VIBRATOEFFECT] = new Vibrato(paramAlloc);

		modulesInChain[i] = 0;
	}
	inputModule = audioInput[0];
	outputModule = audioOutput[0];
}
