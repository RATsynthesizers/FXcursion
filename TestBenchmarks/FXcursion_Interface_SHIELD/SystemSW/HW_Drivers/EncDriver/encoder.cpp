/*
 * encoder.cpp
 *
 *  Created on: Nov 16, 2025
 *      Author: ga
 */

#include "encoder.hpp"

Encoder encMenu(ENC_MENU_A_GPIO_Port,
				ENC_MENU_A_Pin,
				ENC_MENU_B_GPIO_Port,
				ENC_MENU_B_Pin);

Encoder encParam(ENC_PARAM_A_GPIO_Port,
				 ENC_PARAM_A_Pin,
				 ENC_PARAM_B_GPIO_Port,
				 ENC_PARAM_B_Pin);

/*
 * Poll the two channels and book one step on EVERY edge of channel A.
 *
 * ONE STEP PER EDGE OF A, which on this hardware is one step per detent.
 * That is a property of the fitted encoder, measured on the bench rather than
 * assumed: a detent here moves A once, not through a whole quadrature cycle.
 * Counting both edges therefore gives exactly one message per click, in both
 * directions, which is what the GUI wants and what it used to get.
 *
 * I briefly restricted this to the rising edge, on the theory that the old
 * code's two-per-cycle rate was being decimated by the one-deep mail queue and
 * that halving it would preserve the feel. That was wrong in a way worth
 * recording: with one A edge per detent the old code was ALREADY one step per
 * detent, nothing was being decimated for a hand-speed turn, and rising-only
 * simply lost every other click.
 *
 * DIRECTION is (currentA != currentB), evaluated on whichever edge just
 * happened, and it is consistent across both edges. Clockwise walks
 * (0,0)->(1,0)->(1,1)->(0,1), whose two A edges land on (A=1,B=0) and
 * (A=0,B=1) - A differs from B in both. Counter-clockwise walks
 * (0,0)->(0,1)->(1,1)->(1,0), giving (A=1,B=1) and (A=0,B=0) - A equals B in
 * both. So the test needs no knowledge of which edge it is looking at.
 *
 * WHAT THE ACCUMULATOR STILL FIXES, independently of the edge count: the old
 * version raised a "changed" flag on the transition back to ENC_STILL as well,
 * so each detent also published a zero, and the consumers read zero as the
 * opposite direction. See the note in encoder.hpp. That defect was masked by
 * the shallow queue and would have become permanent once the queue was
 * deepened - it is not the same thing as the edge count, and reverting the
 * edge count does not bring it back.
 *
 * STILL NOT DONE: there is no filtering. A bouncing contact produces extra A
 * edges and each books a step. The buttons get a 50 ms debounce; the encoders
 * get nothing, and always did. The proper fix is a 4-state transition table
 * that rejects illegal state pairs, worth doing if jitter shows up in use.
 */
void Encoder::update(void) {

	BOOLEAN currentA = (BOOLEAN) HAL_GPIO_ReadPin(encA_Port, encA_Pin);
	BOOLEAN currentB = (BOOLEAN) HAL_GPIO_ReadPin(encB_Port, encB_Pin);

	if (FALSE == bPrimed)
	{
		/* First poll after the pins really exist: latch, do not count. */
		pinValueA = currentA;
		pinValueB = currentB;
		bPrimed   = TRUE;
		return;
	}

	if (currentA != pinValueA)
	{
		const S8 nStep = (currentA != currentB) ? 1 : -1;

		/* Saturate rather than wrap. */
		if ((nStep > 0) && (nPendingSteps < MAX_PENDING_STEPS))
		{
			nPendingSteps++;
		}
		else if ((nStep < 0) && (nPendingSteps > -MAX_PENDING_STEPS))
		{
			nPendingSteps--;
		}
	}

	/* Save current values for the next call (this is crucial!) */
	pinValueA = currentA;
	pinValueB = currentB;
}
