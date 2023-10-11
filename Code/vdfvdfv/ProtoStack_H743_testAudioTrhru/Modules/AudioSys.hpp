/*
 * System.hpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#ifndef AUDIOSYS_HPP_
#define AUDIOSYS_HPP_

#include <UARTadapter.h>
#include "../../Drivers/HW/MySAIadapter.h"
#include "libModules/Module.hpp"
#include "libModules/Parameter.hpp"
#include "libModules/Wire.hpp"

class AudioSys {
public:
	AudioSys() {
		adapter1 = &sai1adapter;
		adapter2 = &sai2adapter;
	}

	virtual void includeModules(void); // user defined module update sequence
	void getAudioSamplesInInput(SAIadapter_TypeDef *adapter);
	void setAudioSamplesToOutput(SAIadapter_TypeDef *adapter);
	void updateGUI(UARTadapter_TypeDef *adapter);
	void update(void);
protected:
	Module *inputModule;
	Module *outputModule;
//TODO: add inputModule2
	Module *inputModule2;
	Module *outputModule2;
	SAIadapter_TypeDef *adapter1;
	SAIadapter_TypeDef *adapter2;
	uint8_t modulesInChain[CHANNELS_NUM];
	Module *modules[CHANNELS_NUM][EXISTING_MODULES];
};

#endif /* AUDIOSYS_HPP_ */
