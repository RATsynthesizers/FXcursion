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


#endif /* RECORDER_RECORDER_H_ */
