/*
 * Reverb.cpp
 *
 *  Created on: Nov 15, 2020
 *      Author: romte
 */

#include "Reverb.hpp"

void Reverb::process(void) {
	for (u8 lr = 0; lr < STEREO; lr++) {
		u8 rl = !(lr); 			// inverse

		for (u8 k = 0; k < REV_ALLP; k++) {
			revOSCs[lr][k].process(); // process modulators
			revOSCs[lr][k].out += lenModOffsets[lr][k]->val; // add offsets
//			_CLIP_LIMIT(revOSCs[lr][k].out, lenModLoLim->val, lenModUpLim->val)    // clip result
			APs[lr][k].setLength(revOSCs[lr][k].out);        // modulate APs length
			APs[lr][k].setFB(APfeedbacks[lr][k]->val);
		}

		feedbackWire[lr] = APs[rl][REV_ALLP-1].out * fbAmount->val;               // swap feedback left to right
		feedbackWire[lr] = dampingFilterLP[lr].process(feedbackWire[lr]); // dampen feedback (LP)
		feedbackWire[lr] = dampingFilterLP[lr].process(feedbackWire[lr]); // dampen feedback (LP)
		feedbackWire[lr] = dampingFilterHP[lr].process(feedbackWire[lr]); // dampen feedback (HP)
		feedbackWire[lr] = dampingFilterHP[lr].process(feedbackWire[lr]); // dampen feedback (HP)
//
		APs[lr][0].process(satAdd( *input[lr], feedbackWire[lr]));        // process first AP in series
		for(u8 k = 1; k < REV_ALLP; k++)                                  // process rest APs in series
			APs[lr][k].process(APs[lr][k-1].out);
		// mix dry and wet
		this->output[lr] = ( APs[lr][REV_ALLP-1].out * (1-dryWet->val) ) + ( *input[lr] * dryWet->val );
	}

	// make first write of index1 sample and then perform other transfers in txcplt callback
	PsramWrite(&psram, (uint32_t*)(&APs[lr][0].buffer_index1), 1, APs[lr][0].buffer_PSRAM_addr + 4*APs[lr][0].index1);
}





