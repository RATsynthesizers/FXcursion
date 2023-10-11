/*
 * Modules.hpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#ifndef AUDIOSYSOBJECTS_HPP_
#define AUDIOSYSOBJECTS_HPP_

#include "../Modules/looper/Looper.hpp"
#include "../Modules/mixer/Mixer.hpp"
#include "../Modules/AudioSys.hpp"
#include "../Modules/libModules/Parameter.hpp"
#include "../Modules/libModules/Wire.hpp"
#include "../Modules/reverb/Reverb.hpp"

extern Parameter paramAlloc[MAX_SYNTH_PARAMS];
extern Mixer audioInput;
extern Mixer audioOutput;
//extern Reverb rev1;
extern Mixer inputMixer;
extern Looper looper;
extern Wire audioInputWire;
extern Wire audioOutputWire;

extern AudioSys audioSystem; // global audio instance

#endif /* AUDIOSYSOBJECTS_HPP_ */
