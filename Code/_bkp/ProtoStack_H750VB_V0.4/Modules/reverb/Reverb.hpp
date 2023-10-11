/*
 * reverb.hpp
 *
 *  Created on: Nov 15, 2020
 *      Author: romte
 */

#ifndef REVERB_HPP_
#define REVERB_HPP_

#include "../../DSP_MY/dsp.hpp"
#include "../libModules/Module.hpp"

#define REV_PARAMETERS 48
#define REV_ALLP 4 // number allpass filters in series

class Reverb: public Module {
public:
//------------------ parameter ptrs decl ---------------------//

	Parameter *APfeedbacks[STEREO][REV_ALLP];    // allpass feedbacks
	// length (delay) modulating oscillators
	Parameter *lenModFreqs[STEREO][REV_ALLP];    // mod frequencies
	Parameter *lenModAmps [STEREO][REV_ALLP];    // mod amount
	Parameter *lenModOffsets [STEREO][REV_ALLP]; // mod offset
	Parameter *lenModPhases  [STEREO][REV_ALLP]; // oscillator phases (0 = 0degr, 1 = 90degr)
	Parameter *lenModUpLim;  // Upper modulation limit
	Parameter *lenModLoLim;  // Lower modulation limit
	// lowpass feedback damping filters
	Parameter *dampingLPcutoff;
	// highpass feedback damping filters
	Parameter *dampingHPcutoff;
	// global feedback amount (Decay)
	Parameter *fbAmount;
	Parameter *dryWet;


	Reverb(Parameter *p) :
			Module(p, REV_PARAMETERS) {
//----------------- private variables init -------------------//
		feedbackWire[LEFT]  = 0;
		feedbackWire[RIGHT] = 0;
//----------------- parameter address init  ------------------// TODO print them
		Parameter *tmp = this->p;
		for(u32 i = 0; i < STEREO * REV_ALLP; i++)
			APfeedbacks[i/REV_ALLP][i%REV_ALLP] = tmp++;
		for(u32 i = 0; i < STEREO * REV_ALLP; i++)
			lenModFreqs[i/REV_ALLP][i%REV_ALLP] = tmp++;
		for(u32 i = 0; i < STEREO * REV_ALLP; i++)
			lenModAmps[i/REV_ALLP][i%REV_ALLP] = tmp++;
		for(u32 i = 0; i < STEREO * REV_ALLP; i++)
			lenModOffsets[i/REV_ALLP][i%REV_ALLP] = tmp++;
		for(u32 i = 0; i < STEREO * REV_ALLP; i++)
			lenModPhases[i/REV_ALLP][i%REV_ALLP] = tmp++;
		lenModUpLim  = tmp++;
		lenModLoLim =  tmp++;
		dampingLPcutoff = tmp++;
		dampingHPcutoff = tmp++;
		fbAmount = tmp++;
		dryWet = tmp++;
		this->paramNum = tmp - this->p; // calculate actual number of used parameters TODO print it

		//----------------- parameter values init  ----------------------------//
				const float ap_feedbacks_ini[][REV_ALLP] =	{	{0.3f, 0.4f, 0.4f, 0.4f},
																{0.3f, 0.4f, 0.4f, 0.4f}	};
				const float lenmodfreqs_ini[][REV_ALLP]   = 	{	{.53f, .03f, .02f, .023f},
																	{.51f, .03f, .02f, .023f}	};
				const float lenmodamps_ini[][REV_ALLP] = {	{ALLPASS_DELAYLEN/66.0f, ALLPASS_DELAYLEN/65.0f, ALLPASS_DELAYLEN/78.0f, ALLPASS_DELAYLEN/78.0f},
															{ALLPASS_DELAYLEN/69.0f, ALLPASS_DELAYLEN/67.0f, ALLPASS_DELAYLEN/78.0f, ALLPASS_DELAYLEN/78.0f}	};
				const float lenmodoffsets_ini[][REV_ALLP] = {	{ALLPASS_DELAYLEN*.74f, ALLPASS_DELAYLEN*.33f, ALLPASS_DELAYLEN*.33f, ALLPASS_DELAYLEN*.37f},
																{ALLPASS_DELAYLEN*.75f, ALLPASS_DELAYLEN*.49f, ALLPASS_DELAYLEN*.33f, ALLPASS_DELAYLEN*.37f}	};

				const u8 lenmodphases_ini[][REV_ALLP] =	{	{1,1,1,0},
															{0,0,1,1}	};
				for(u32 i = 0; i < STEREO * REV_ALLP; i++)
					APfeedbacks[i/REV_ALLP][i%REV_ALLP]->val = ap_feedbacks_ini[i/REV_ALLP][i%REV_ALLP];
				for(u32 i = 0; i < STEREO * REV_ALLP; i++)
					lenModFreqs[i/REV_ALLP][i%REV_ALLP]->val = lenmodfreqs_ini[i/REV_ALLP][i%REV_ALLP];
				for(u32 i = 0; i < STEREO * REV_ALLP; i++)
					lenModAmps[i/REV_ALLP][i%REV_ALLP]->val  = lenmodamps_ini[i/REV_ALLP][i%REV_ALLP];
				for(u32 i = 0; i < STEREO * REV_ALLP; i++)
					lenModOffsets[i/REV_ALLP][i%REV_ALLP]->val = lenmodoffsets_ini[i/REV_ALLP][i%REV_ALLP];
				for(u32 i = 0; i < STEREO * REV_ALLP; i++)
					lenModPhases[i/REV_ALLP][i%REV_ALLP]->val  = lenmodphases_ini[i/REV_ALLP][i%REV_ALLP];

				for(u32 i = 0; i < STEREO * REV_ALLP; i++)
					revOSCs[i/REV_ALLP][i%REV_ALLP].setFA(lenmodfreqs_ini[i/REV_ALLP][i%REV_ALLP], lenmodamps_ini[i/REV_ALLP][i%REV_ALLP], lenmodphases_ini[i/REV_ALLP][i%REV_ALLP]);

				inL = 0;
				inR = 0;
				lenModUpLim->val = .95f*ALLPASS_DELAYLEN;
				lenModLoLim->val = .05f*ALLPASS_DELAYLEN;
				dampingLPcutoff->val = 10000.0f/(SAMPLINGFREQ); // normalized
				dampingHPcutoff->val =  1000.0f/(SAMPLINGFREQ);
				fbAmount->val = 0.7f; //1.25
				dryWet->val = 0.6f;  // % dry

				for(u32 i = 0; i < STEREO; i++) {
					dampingFilterLP[i].setFc(dampingLPcutoff->val);
					dampingFilterHP[i].setFc(dampingHPcutoff->val);
				}

				///////// psram APs   //////////
				const uint32_t ap_psram_addresses[][REV_ALLP] = {	{0, ALLPASS_DELAYLEN*4*1, ALLPASS_DELAYLEN*4*2, ALLPASS_DELAYLEN*4*3},
																	{ALLPASS_DELAYLEN*4*4, ALLPASS_DELAYLEN*4*5, ALLPASS_DELAYLEN*4*6, ALLPASS_DELAYLEN*4*7}	};
				for(u32 i = 0; i < STEREO * REV_ALLP; i++)
									APs[i/REV_ALLP][i%REV_ALLP].setPsramAddrs(ap_psram_addresses[i/REV_ALLP][i%REV_ALLP]);

	}
	void init(void) {

	}

	void process(void);


	//	AllPass APs[STEREO][REV_ALLP];        // Allpass filters
		AllPass_PSRAM APs[STEREO][REV_ALLP];	// now public

private:
	//------------------ parameter shortcuts decl & def ---------------//
	float &inL = *(input[LEFT]);
	float &inR = *(input[RIGHT]);
//	float &UpLim = lenModUpLim->val;   // reference to actual value
//	float &LoLim = lenModLoLim->val;
//	float &dmpLP = dampingLPcutoff->val;
//	float &dmpHP = dampingHPcutoff->val;
//	float &decay = fbAmount->val;
//	float &revMix = dryWet->val;

	//------------------- private variables ---------------------------//
	u8 lr = LEFT;						  // left or right state



	gsOsc revOSCs[STEREO][REV_ALLP]; 	  // AP lengh modulators
	OnePoleLp dampingFilterLP[STEREO];    // Lowpass damping
	OnePoleHp dampingFilterHP[STEREO];    // Highpass damping

	float feedbackWire[STEREO];

};




#endif /* REVERB_HPP_ */
