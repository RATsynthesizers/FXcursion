/*
 * Modules.cpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "AudioSysObjects.hpp"

	IN_DTCMRAM AudioSys audioSystem; 	// global audio instance
	IN_DTCMRAM Parameter paramAlloc[MAX_SYNTH_PARAMS];

// IO always goes first, THEN other modules
	IN_DTCMRAM Amp audioInput1(paramAlloc);
	IN_DTCMRAM Amp audioOutput1(paramAlloc);
	IN_DTCMRAM Amp amp1(paramAlloc);

// IO wires always go first, THEN other modules wires
	IN_DTCMRAM Wire audioInput1Wire(&audioInput1, &amp1);
	IN_DTCMRAM Wire audioOutput1Wire(&amp1, &audioOutput1);

	// module update sequence (update MAX_MODULES_NUM)
	void AudioSys::includeModules() {
		this->modules[0] = &amp1;
		this->inputModule = &audioInput1;
		this->outputModule = &audioOutput1;
	}
