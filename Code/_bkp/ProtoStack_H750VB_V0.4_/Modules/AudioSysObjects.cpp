/*
 * Modules.cpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "AudioSysObjects.hpp"

	IN_RAMD1 AudioSys audioSystem; 	// global audio instance

	IN_RAMD1 Parameter paramAlloc[MAX_SYNTH_PARAMS];



	IN_RAMD1 Amp audioInput1(paramAlloc);
	IN_RAMD1 Amp audioOutput1(paramAlloc);
	IN_RAMD1 Amp amp1(paramAlloc);

	IN_RAMD1 Wire audioInput1Wire(&audioInput1, &amp1);
	IN_RAMD1 Wire audioOutput1Wire(&amp1, &audioOutput1);

//	IN_RAMD1 Reverb rev(paramAlloc);
//	IN_RAMD1 Wire amp2rev(&amp1, &rev);



	// module update sequence (update MAX_MODULES_NUM)
	void AudioSys::includeModules() {
		this->modules[0] = &amp1;
//		this->modules[1] = &rev;
		this->inputModule = &audioInput1;
		this->outputModule = &audioOutput1;
	}

