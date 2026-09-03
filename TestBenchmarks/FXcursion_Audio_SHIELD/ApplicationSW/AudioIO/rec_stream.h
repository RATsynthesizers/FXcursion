/**
 * @file      rec_stream.h
 *
 * @details   Staging for the recorder stream on its way to the interface
 *            controller over SPI. HAL-free on purpose - see rec_spi.h for the
 *            transport that drives it.
 *
 *            ------------------------------------------------------------------
 *            WHY THIS IS A SEPARATE MODULE
 *            ------------------------------------------------------------------
 *
 *            The recorder interleaves into DTCM, because its writes are
 *            scattered - one store per (frame, slot) in the middle of the audio
 *            block - and DTCM is zero wait state. But DMA1 and DMA2 are masters
 *            in domain D2 and CANNOT ADDRESS DTCM AT ALL, so the block has to be
 *            copied into RAM_D2 before any transfer can read it. That is what
 *            this module owns: two staging blocks, and the small state machine
 *            that decides which one the audio ISR may write and which one the
 *            transport may hand to the DMA.
 *
 *            That state machine is worth testing on the host, and a HAL call
 *            anywhere in this file would make that impossible - which is the
 *            same split already used for chan_map/audio_io and
 *            ctrl_link/ctrl_uart.
 *
 *            ------------------------------------------------------------------
 *            THE INVARIANT
 *            ------------------------------------------------------------------
 *
 *            At most one block is in flight and at most one is waiting, so two
 *            staging blocks are exactly enough. A block is only ever written
 *            into a half that is NEITHER in flight NOR waiting.
 *
 *            When no such half exists the block is DROPPED and counted. That is
 *            the whole reason this is a state machine rather than a toggle: the
 *            wrong answer here is to overwrite the buffer the DMA is reading,
 *            which does not drop a block, it corrupts one - and it corrupts it
 *            halfway through, so the interface records a block that is half of
 *            one moment and half of another.
 *
 *            It should never happen. A block is 1333 us and a transfer is 667 us
 *            at the SPI clock this board runs (see main.c), so the previous
 *            transfer has always finished. nOverruns is a bring-up instrument,
 *            and it belongs in PROTO_DIAG with the others that should read zero.
 *
 *            ------------------------------------------------------------------
 *            NO CACHE MAINTENANCE, AND WHY THAT IS NOT LUCK
 *            ------------------------------------------------------------------
 *
 *            The source is DTCM, which is never cached. The destination is
 *            RAM_D2, which MPU region 0 maps non-cacheable precisely so that
 *            the audio DMA buffers need no clean or invalidate. So the copy in
 *            here is the whole story - there is no maintenance to forget.
 *
 *            If RAM_D2 ever stops being non-cacheable, this module needs a
 *            clean before every transfer and the comment above is a lie. See
 *            mem_map.h.
 *
 *            ------------------------------------------------------------------
 *            ALIGNMENT OF THE STREAM ITSELF
 *            ------------------------------------------------------------------
 *
 *            The interface receives into a circular DMA and de-interleaves by
 *            position, so it has no way to tell where a block begins. Gain or
 *            lose one word and every recorded channel is permanently rotated by
 *            one - quietly, into the wrong file.
 *
 *            RecStream_Enable therefore resets the whole state machine, so
 *            enabling always starts a fresh transfer on the next block boundary
 *            and never mid-block. The interface must arm its receive DMA BEFORE
 *            it asks for the stream; that is what PROTO_CMD_STREAM is for.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef REC_STREAM_H
#define REC_STREAM_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"
#include "fx_defs.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/** Staging blocks. Two: one in flight, one waiting. See the invariant above. */
#define REC_STAGE_QTY                   (2U)

/** 32-bit words in one staged block - 64 frames x 4 slots = 256 words, 1 KiB. */
#define REC_STAGE_WORDS                 (AUDIO_BLOCK_FRAMES * REC_SLOT_QTY)

/** Returned when there is nothing for the transport to start. */
#define REC_STAGE_NONE                  (0xFFU)



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief Counters that should all read zero on a healthy board.
 */
typedef struct stREC_STREAM_STATS
{
    U32 nBlocksSent;        /**< transfers completed                             */
    U32 nBlocksDropped;     /**< no free staging half - see the invariant        */
    U32 nErrors;            /**< transfer failures reported by the transport     */

} REC_STREAM_STATS;



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/** Zero the staging blocks, the state machine and the counters. */
extern STD_RESULT RecStream_Init(void);

/**
 * @brief Start or stop staging.
 *
 * Both edges drop a queued block and leave a transfer that is already in flight
 * strictly alone - see the comment on the implementation for why resetting the
 * in-flight state here would reintroduce exactly the corruption this module is
 * built to prevent, and why letting the stale transfer finish is harmless.
 *
 * Needs no abort and no HAL, which is what lets the protocol layer call it
 * directly.
 */
extern void RecStream_Enable(const BOOLEAN bOn);

extern BOOLEAN RecStream_IsEnabled(void);

/**
 * @brief Copy one interleaved block into a free staging half.
 *
 * Call from the audio ISR, once per block, with Recorder_GetStream(). The
 * caller must serialise this against RecStream_Complete - both mutate the
 * state machine, and on this chip the transfer-complete interrupt and the audio
 * interrupt are different priorities. rec_spi.c holds that critical section,
 * because the means of doing so is CMSIS and this file is deliberately free of
 * it.
 *
 * @param pSrc     interleaved block, REC_SLOT_QTY samples per frame
 * @param nFrames  frames in the block, clamped to AUDIO_BLOCK_FRAMES
 *
 * @return the half the transport should start now, or REC_STAGE_NONE - either
 *         because a transfer is already running and this block is now waiting,
 *         or because the block was dropped
 */
extern U8 RecStream_Stage(const S32* const pSrc, const U16 nFrames);

/**
 * @brief Tell the state machine a transfer finished.
 *
 * @return the next half to start, or REC_STAGE_NONE if nothing is waiting
 */
extern U8 RecStream_Complete(void);

/** Report a failed transfer. Drops what was staged and resynchronises. */
extern void RecStream_Error(void);

/** The staging block, for the transport to hand to the DMA. NULL_PTR if nHalf is not valid. */
extern const S32* RecStream_Buffer(const U8 nHalf);

/** Words actually staged in that half. Zero if nHalf is not valid. */
extern U16 RecStream_Words(const U8 nHalf);

extern const REC_STREAM_STATS* RecStream_Stats(void);



#endif // #ifndef REC_STREAM_H

/****************************************** end of file *******************************************/
