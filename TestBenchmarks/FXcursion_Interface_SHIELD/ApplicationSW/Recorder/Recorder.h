/*
 * Recorder.h
 *
 *  Created on: 22 февр. 2026 г.
 *      Author: ga
 */

#ifndef RECORDER_H_
#define RECORDER_H_

#include "general.h"

STD_RESULT RecorderInit(void);

/**
 * @brief Chunks the de-interleave has completed that the writer has not taken.
 *
 * One chunk is REC_CHUNK_SAMPLES, 21.3 ms. Compare against REC_CHUNKS for how
 * close the ring is to overrunning.
 */
uint32_t Recorder_BacklogChunks(void);

/**
 * @brief TRUE when the recorder has nothing outstanding for the card.
 *
 * TRUE while idle - an idle recorder is not behind, it simply is not consuming
 * the ring. While recording, TRUE means the backlog is below the threshold the
 * writer wakes on, i.e. there is no work waiting rather than literally zero.
 *
 * This is what makes a "saving" indicator honest after a long loop write: the
 * loop file being closed does not mean the machine has caught up, and starting
 * another card operation before it has is how a backlog turns into an overrun.
 */
BOOLEAN Recorder_IsCaughtUp(void);

/***************************************************************************************************
* Loop transport routing
*
* The de-interleave gains a fifth route while a loop transfer runs - the loop
* slots are contiguous on the wire, so they cost one route rather than one per
* slot, which is the only reason they fit alongside the four recorder planes.
*
* WHAT ARRIVES IS S32, NOT PACKED 24-BIT. The MDMA writes the wire words as
* they are; it cannot narrow on the way past. So the region armed here fills
* with one 32-bit word per sample and something has to pack it to three bytes
* before it reaches the card. That choice is the session layer's, and it decides
* how large the region has to be - 4/3 of the payload if the pack happens later,
* exactly the payload if it happens first.
***************************************************************************************************/

/**
 * @brief Point the loop route at a region and start filling it.
 *
 * Arm BEFORE telling the audio board to widen the stream: the route is only
 * added to a block when this is armed, so arming late loses the first blocks
 * silently.
 *
 * @param nBase   destination, WORD ALIGNED
 * @param nBytes  how much of it may be written
 */
STD_RESULT Recorder_ArmLoopDest(const uint32_t nBase, const uint32_t nBytes);

/**
 * @brief Stop routing loop slots.
 *
 * Disarm BEFORE the stream narrows. While armed, the route keeps writing
 * whatever the wire leaves in slots the audio board has already stopped filling.
 */
void Recorder_DisarmLoopDest(void);

/** @brief Bytes written into the armed region so far. */
uint32_t Recorder_LoopBytesTaken(void);


#endif /* RECORDER_RECORDER_H_ */
