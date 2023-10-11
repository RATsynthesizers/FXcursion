/*
 * Modules.hpp
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#ifndef AUDIOSYSOBJECTS_HPP_
#define AUDIOSYSOBJECTS_HPP_

#include "libModules/Parameter.hpp"
#include "libModules/Wire.hpp"
#include "amp/Amp.hpp"
#include "reverb/Reverb.hpp"

#include "AudioSys.hpp"

extern Parameter paramAlloc[MAX_SYNTH_PARAMS];
extern Amp audioInput1;
extern Amp audioOutput1;
extern Amp amp1;
extern Reverb rev;
extern Wire amp2rev;
extern Wire audioInput1Wire;
extern Wire audioOutput1Wire;

extern AudioSys audioSystem; // global audio instance




#endif /* AUDIOSYSOBJECTS_HPP_ */
