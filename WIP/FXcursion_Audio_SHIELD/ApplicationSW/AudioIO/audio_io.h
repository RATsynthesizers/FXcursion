/**
 * @file      audio_io.h
 *
 * @details   The timebase. Everything the engine needs to be driven by real
 *            converters instead of by a test harness.
 *
 *            ------------------------------------------------------------------
 *            ONE INTERRUPT RUNS THE WHOLE SYSTEM
 *            ------------------------------------------------------------------
 *
 *            There is no RTOS and no scheduler. The audio engine is driven by
 *            exactly one interrupt: SAI1 block A's receive DMA, half complete
 *            and complete. Everything else is a free-running circular stream
 *            whose pointer is locked to that one by hardware.
 *
 *            That is possible because SAI1_A is the only clock master on the
 *            board. SAI1_B is synchronous to it internally, SAI2's two blocks
 *            are synchronous to it externally, and all four therefore share one
 *            frame sync. When SAI1_A's DMA reaches the halfway point, every
 *            other main stream is at the same place in its own buffer.
 *
 *            ------------------------------------------------------------------
 *            START ORDER IS NOT ARBITRARY
 *            ------------------------------------------------------------------
 *
 *            HAL_SAI_Receive_DMA and HAL_SAI_Transmit_DMA both enable the block
 *            they are given, and a synchronous block that is enabled after its
 *            clock source has already started can latch onto the wrong frame.
 *
 *            So AudioIO_Start arms every dependent block first and the master
 *            LAST:
 *
 *                SAI2_B (TX, sync ext)      \
 *                SAI2_A (RX, sync ext)       |  all wait for a clock
 *                SAI1_B (TX, sync internal) /
 *                SAI1_A (RX, MASTER)        <- starts the clock, so last
 *
 *            Get this backwards and the symptom is channels 2 and 3 swapped, or
 *            offset by a sample, intermittently and depending on start-up
 *            timing. AudioIO_PhaseFaults exists to catch exactly that.
 *
 *            ------------------------------------------------------------------
 *            WHAT HAPPENS EACH BLOCK
 *            ------------------------------------------------------------------
 *
 *                ChanMap_Gather     two stereo DMA halves -> one 4ch block,
 *                                   sign extending 24-bit samples
 *                AudioSys_Process   the engine
 *                ChanMap_Scatter    back out to the two transmit halves
 *                HpBus_Process      the monitor sum, written ahead of the
 *                                   headphone DMA rather than into it
 *                load + phase       DWT cycle count against the block budget,
 *                                   and a check that SAI2 has not slipped
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef AUDIO_IO_H
#define AUDIO_IO_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"

#include "audio_io_cfg.h"



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Silence the transmit buffers, reset the headphone bus, start the cycle
 *        counter.
 *
 * Call after AudioSys_Init and after the codecs have been configured, but
 * before AudioIO_Start. Does not touch the converters.
 */
extern STD_RESULT AudioIO_Init(void);

/**
 * @brief Arm every stream and start the clock. Audio runs from here.
 *
 * @return RESULT_NOT_OK if any stream refused to start; nothing is left running
 *         in that case
 */
extern STD_RESULT AudioIO_Start(void);

/**
 * @brief Stop every stream. The master goes first, so the slaves stop clocking.
 */
extern STD_RESULT AudioIO_Stop(void);

/**
 * @brief Super-loop hook. Restarts the streams after a converter error.
 *
 * The error itself is noticed in interrupt context, but recovery means stopping
 * and re-arming five DMA streams, which is not interrupt work. Call this
 * regularly from the main loop.
 */
extern void AudioIO_Service(void);

/** TRUE between a successful AudioIO_Start and the next AudioIO_Stop. */
extern BOOLEAN AudioIO_IsRunning(void);

/** Blocks processed since start. Wraps; useful only as a liveness check. */
extern U32 AudioIO_BlockCount(void);

/**
 * @brief Blocks where SAI2's DMA was not in the same half as SAI1's.
 *
 * Must stay at zero. Anything else means the two converters are not sharing a
 * frame - a start-order bug, or a clock that is not actually shared - and
 * channels 2 and 3 cannot be trusted.
 */
extern U32 AudioIO_PhaseFaults(void);

/** Converter error interrupts seen, across all five streams. */
extern U32 AudioIO_StreamErrors(void);



#endif // #ifndef AUDIO_IO_H

/****************************************** end of file *******************************************/
