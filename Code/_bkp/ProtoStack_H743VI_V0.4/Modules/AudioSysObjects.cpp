/*
 * Modules.cpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#include "../Modules/AudioSysObjects.hpp"

IN_RAMD1 AudioSys audioSystem; 	// global audio instance

IN_RAMD1 Parameter paramAlloc[MAX_SYNTH_PARAMS];

//
//
//                             -->[rev]--
//  	            -->[amp1]--|
//[SAI1]->[input1]--|          -->[output1]-->[SAI1]
//  	            -->
//
//
//
//

IN_RAMD1 Mixer audioInput(paramAlloc);

IN_RAMD1 Mixer inputMixer(paramAlloc);
IN_RAMD1 Wire audioInputWire(&audioInput, &inputMixer);

//IN_RAMD1 Reverb rev1(paramAlloc);
//IN_RAMD1 Wire mixer2rev1(&inputMixer, &rev1);

IN_RAMD1 Mixer audioOutput(paramAlloc);
IN_RAMD1 Wire audioOutputWire(&inputMixer, &audioOutput);

IN_RAMD1 Looper looper(paramAlloc);
IN_RAMD1 Wire looperWireIn(&audioOutput, &looper);
IN_RAMD1 Wire looperWireOut(&looper, &inputMixer);

//	IN_RAMD1 Amp amp2(paramAlloc);
//	IN_RAMD1 Wire amp1_2amp2(&amp1, &amp2);
//
//	IN_RAMD1 Amp amp3(paramAlloc);
//	IN_RAMD1 Wire amp2_2amp3(&amp2, &amp3);
//
//	IN_RAMD1 Amp amp4(paramAlloc);
//	IN_RAMD1 Wire amp3_2amp4(&amp3, &amp4);
//
//	IN_RAMD1 Amp amp5(paramAlloc);
//	IN_RAMD1 Wire amp4_2amp5(&amp4, &amp5);

//	IN_RAMD1 Reverb rev1(paramAlloc);
//	IN_RAMD1 Wire amp5_2rev1(&amp1, &rev1);

//	IN_RAMD1 Reverb rev2(paramAlloc);
//	IN_RAMD1 Wire rev1_2rev2(&rev1, &rev2);

//	IN_RAMD1 Reverb rev3(paramAlloc);
//	IN_RAMD1 Wire rev2_2rev3(&rev2, &rev3);



// module update sequence (update MAX_MODULES_NUM)
void AudioSys::includeModules() {
	this->modules[1] = &inputMixer;
	//this->modules[1] = &rev1;
	this->modules[0] = &looper;
//		this->modules[1] = &amp2;
//		this->modules[2] = &amp3;
//		this->modules[3] = &amp4;
//		this->modules[4] = &amp5;
//		this->modules[1] = &rev1;
	//this->modules[2] = &rev2;
	//this->modules[3] = &rev3;
	this->inputModule = &audioInput;
	this->outputModule = &audioOutput;
}

void AudioSys::init() {
	saiAdapter_Init(&sai1adapter,HW_SAI1);
	saiAdapter_Init(&sai2adapter,HW_SAI2);

	this->includeModules();
}

/////////////// SAI cplt & halfCplt callbacks ///////////////

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai) {

	if(hsai == &hsai_BlockA1) {
		sai1adapter.RxHalfFlag=1;//++;
		audioSystem.update();
	}
	if(hsai == &hsai_BlockB2) {
		sai2adapter.RxHalfFlag=1;//++;
	}
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai) {

	if(hsai == &hsai_BlockA1) {
		sai1adapter.RxFlag=1;//++;
		audioSystem.update();
	}
	if(hsai == &hsai_BlockB2) {
		sai2adapter.RxFlag=1;//++;
	}
}


