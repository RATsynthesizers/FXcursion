/**
 * @file      recorder.h
 *
 * @details   The recorder tap: passes audio through unchanged and copies it into
 *            the stream sent to the interface controller over SPI.
 *
 *            ------------------------------------------------------------------
 *            SLOT ASSIGNMENT NEEDS NO ARBITRATION
 *            ------------------------------------------------------------------
 *
 *            A chain's recorder occupies the stream slots corresponding to its
 *            own planes. Since chain widths always sum to AUDIO_CH_QTY, and
 *            REC_SLOT_QTY == AUDIO_CH_QTY, "a recorder in every chain" needs
 *            exactly REC_SLOT_QTY slots in every topology:
 *
 *                4 mono            1+1+1+1 = 4
 *                stereo + 2 mono   2+1+1   = 4
 *                2 mono + stereo   1+1+2   = 4
 *                2 stereo          2+2     = 4
 *
 *            The map is a pure function of the topology. It cannot conflict, so
 *            no configuration is ever rejected for want of a recorder slot.
 *
 *            ------------------------------------------------------------------
 *            THE STREAM IS FIXED WIDTH
 *            ------------------------------------------------------------------
 *
 *            REC_SLOT_QTY slots are always transmitted, interleaved, with
 *            silence in unused ones. Keeping the stride constant means the
 *            interface never has to reprogram its MDMA de-interleave descriptor
 *            when the number of active taps changes - only if the bit depth
 *            changes. The slot map in PROTO_ACK is informational: it tells the
 *            interface which chain landed in which slot so recordings get the
 *            right names.
 *
 *            NOTE FOR THE INTERFACE SIDE: samples are S32 carrying a 24-bit
 *            value, so the interleave stride is REC_SLOT_QTY * 4 bytes. The
 *            current Recorder.c on the interface assumes 16-bit samples and an
 *            8-byte stride, and its SPI1 slave is configured 16BIT against this
 *            board's 32BIT master; all of that must change together with this.
 *
 *            TRANSPORT: rec_stream.c stages this buffer into RAM_D2 and
 *            rec_spi.c transmits it, one burst per block. Nothing is sent until
 *            the interface asks with PROTO_CMD_STREAM - it de-interleaves by
 *            position, so it has to have armed its receive DMA before the first
 *            word arrives.
 *
 *            ------------------------------------------------------------------
 *            LATENCY
 *            ------------------------------------------------------------------
 *
 *            A tap before the effects and a tap after them are written in the
 *            same block, so recorded tracks are sample-aligned with no
 *            compensation. That stops being true as soon as an effect with
 *            lookahead exists, which is why every effect will need to declare a
 *            latency. There is no such effect today; add the field when the
 *            first one arrives.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef RECORDER_H
#define RECORDER_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"
#include "grid.h"



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/** Clear the stream buffer and the slot map. MUST be called before audio runs. */
extern STD_RESULT Recorder_Init(void);

/** Cache the slot map from a newly applied grid. Call from the super-loop. */
extern void Recorder_Apply(const GRID* const pGrid);

/**
 * @brief Silence every slot for this block.
 *
 * MUST be called once at the top of each block, BEFORE Grid_Process, so that
 * slots without a tap transmit silence rather than the previous block.
 */
extern void Recorder_BeginBlock(const U16 nFrames);

/**
 * @brief Tap one chain into its slots. Passes audio through unchanged.
 *
 * Call from the audio ISR only.
 */
extern void Recorder_Process(const GRID* const pGrid,
                             const U8 nChain,
                             FLOAT32* const apChain[],
                             const U16 nFrames);

/**
 * @brief The interleaved block ready for transmission.
 *
 * The buffer lives in DTCM, which DMA1 and DMA2 cannot reach on the STM32H7, so
 * the platform layer must copy it into a DMA-visible buffer rather than pointing
 * the SPI DMA at it directly.
 *
 * @param pFrameQty  frames in the block, may be NULL
 *
 * @return pointer to nFrames * REC_SLOT_QTY interleaved S32 samples
 */
extern const S32* Recorder_GetStream(U16* const pFrameQty);



#endif // #ifndef RECORDER_H

/****************************************** end of file *******************************************/
