/*
 * encoder.h
 *
 *  Created on: Nov 16, 2025
 *      Author: ga
 */

#ifndef ENCODER_HPP_
#define ENCODER_HPP_


#include "gpio.h"
#include "general.h"
#include "common_cfg.h"

/*
 * Sign convention for a delivered step, unchanged: +1 is the direction in
 * which an edge on A leaves A differing from B, and every consumer of
 * ENC_MENU / ENC_PARAM reads it as such. One step per edge of A, which on the
 * fitted encoder is one step per detent - see encoder.cpp.
 *
 * ENC_STILL exists only to name zero. It must never be PUBLISHED - see the
 * note on pendingStep() below.
 */
typedef enum enEncoderState
{
	ENC_LEFTTURN  = -1,
	ENC_STILL 	  = 0,
	ENC_RIGHTTURN = 1

} EncoderState;

/*
 * A polled quadrature encoder that accumulates detents instead of exposing a
 * momentary state.
 *
 * WHY IT IS NOT A "state + changed flag" ANY MORE. The old version set its
 * changed flag whenever the decoded state DIFFERED FROM THE PREVIOUS POLL,
 * which includes the transition back to ENC_STILL. Every detent therefore
 * raised the flag twice: once carrying the direction, and once carrying zero.
 * UISurvey published both, and the consumers - AddModuleWindow::selectUp /
 * selectDown, and SystemView's chain navigation - are written as
 *
 *     if (1 == nValue) { forward } else { backward }
 *
 * so a published zero moved the cursor BACKWARDS. It was invisible only
 * because the pubsub mail queue was one item deep and the poll loop ran far
 * faster than the 60 Hz drain, so the phantom was almost always the message
 * that got dropped. Deepening that queue or adding a delay to the poll loop -
 * both of which are now done - would have made it constant.
 *
 * An accumulator has no such state to misreport: a detent either is or is not
 * still owed to the GUI.
 */
class Encoder {
public:
	Encoder(GPIO_TypeDef* _encA_Port,
			uint16_t _encA_Pin,
			GPIO_TypeDef* _encB_Port,
			uint16_t _encB_Pin)
	{
		encA_Port = _encA_Port;
		encA_Pin = _encA_Pin;

		encB_Port = _encB_Port;
		encB_Pin = _encB_Pin;

		nPendingSteps = 0;

		pinValueA = FALSE;
		pinValueB = FALSE;

		/*
		 * These objects are constructed at static-init time, long before
		 * MX_GPIO_Init has configured the pins, so the constructor cannot
		 * sample them. The first update() therefore only latches what it
		 * finds and counts nothing - otherwise an encoder that happens to
		 * rest with A high would book a phantom detent at every boot.
		 */
		bPrimed = FALSE;
	}

	void update(void);

	/*
	 * The next step owed to the GUI: -1, 0 or +1. Does NOT consume it.
	 *
	 * Deliberately one step at a time rather than the whole accumulated
	 * delta, because every consumer compares against 1 and -1 exactly. A
	 * value of 2 would fail `1 == nValue` and be taken for a reverse turn -
	 * the same bug in a new place. A fast turn now produces several
	 * consecutive +-1 messages, which Model::tick drains in one frame.
	 */
	S8 pendingStep(void) const
	{
		if (nPendingSteps > 0)
		{
			return 1;
		}

		if (nPendingSteps < 0)
		{
			return -1;
		}

		return 0;
	}

	/*
	 * Consume one step, once it has actually been handed over.
	 *
	 * Kept separate from pendingStep so the caller can publish FIRST and
	 * only consume if the publish succeeded. A full queue then costs
	 * latency instead of a lost detent, which is what the old
	 * clear-the-flag-then-publish order could not do.
	 */
	void stepDelivered(void)
	{
		if (nPendingSteps > 0)
		{
			nPendingSteps--;
		}
		else if (nPendingSteps < 0)
		{
			nPendingSteps++;
		}
	}

private:

	/*
	 * Ceiling on undelivered detents. Reached only if the GUI has stopped
	 * draining entirely, and in that case replaying a long spin once it
	 * recovers is worse than dropping the excess.
	 */
	static const S8 MAX_PENDING_STEPS = 32;

	S8 nPendingSteps;

	BOOLEAN bPrimed;

	BOOLEAN pinValueA;
	BOOLEAN pinValueB;

	GPIO_TypeDef* encA_Port;
	uint16_t encA_Pin;

	GPIO_TypeDef* encB_Port;
	uint16_t encB_Pin;

	/////////////////////////////////////////////////
};

extern Encoder encMenu;
extern Encoder encParam;

#endif /* HW_DRIVERS_ENCDRIVER_ENCODER_H_ */
