/**
 * @file      fx_interleave.h
 *
 * @details   Geometry of the interleaved recorder stream, as arithmetic.
 *
 *            ############################################################
 *            #  DUPLICATED IN THE INTERFACE CONTROLLER - keep in sync.  #
 *            #  Use TestBenchmarks/sync_shared.py.                      #
 *            ############################################################
 *
 *            ------------------------------------------------------------------
 *            WHY THIS EXISTS
 *            ------------------------------------------------------------------
 *
 *            The audio controller writes the stream and the interface
 *            de-interleaves it, so both boards need the same answer to "where is
 *            slot N in this block, and how far to the next one". On the
 *            interface that answer is programmed straight into MDMA registers -
 *            source address, bytes per beat, block repeat count, and the
 *            post-beat source skip - and hand-computed strides in register
 *            writes are exactly the kind of thing that is wrong by four bytes
 *            and produces a recording that sounds like a different instrument.
 *
 *            Putting the arithmetic here makes it testable on a PC, which
 *            matters more than usual: the failure mode is not a crash. Get the
 *            stride wrong and every channel lands in the wrong file, with
 *            plausible audio in it, and nothing reports a problem.
 *
 *            ------------------------------------------------------------------
 *            THE LAYOUT
 *            ------------------------------------------------------------------
 *
 *            One block is nFrames frames. Each frame is nStreamWidth slots, and
 *            each slot is one S32 carrying a 24-bit sample:
 *
 *                frame 0            frame 1            frame 2
 *                [s0][s1][s2][s3]   [s0][s1][s2][s3]   [s0][s1][s2][s3]
 *                 ^                  ^
 *                 |<--- stride ----->|      stride = nStreamWidth * 4 bytes
 *
 *            A MONO chain occupies one slot, so its samples are 16 bytes apart
 *            at the default width of 4. A STEREO chain occupies TWO ADJACENT
 *            slots, so its two samples are contiguous - 8 bytes moved per frame
 *            - which is already the interleaved order a stereo WAV file wants,
 *            and is why a stereo pair needs no separate merge step.
 *
 *            nStreamWidth comes from PROTO_ACK, not from a constant. The audio
 *            side reports what it actually committed to, and the interface must
 *            reprogram from that before it trusts the stream.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_INTERLEAVE_H
#define FX_INTERLEAVE_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "fx_defs.h"

/* One wire contract, two compilers: the audio controller builds this as C, the
 * interface controller includes it from C++. */
#ifdef __cplusplus
extern "C" {
#endif



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/** Bytes per slot. S32 carrying 24 bits - see recorder.h on why not 3. */
#define FX_IL_BYTES_PER_SLOT            (4U)



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief One de-interleave transfer: one chain's slots out of the block.
 *
 * Named for what the hardware needs rather than what the format is, because
 * every field maps onto an MDMA register:
 *
 *      nSrcOffsetBytes  added to the block base to get CSAR
 *      nBytesPerBeat    the contiguous run moved per frame
 *      nSrcSkipBytes    CBRUR, the source address update after each beat
 *      nBeats           frames, so CBNDTR's block repeat count is nBeats - 1
 *      nDstBytes        total bytes this transfer writes
 */
typedef struct stFX_IL_XFER
{
    U32 nSrcOffsetBytes;
    U32 nBytesPerBeat;
    U32 nSrcSkipBytes;
    U32 nBeats;
    U32 nDstBytes;

} FX_IL_XFER;



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Work out one chain's transfer out of an interleaved block.
 *
 * @param pOut          filled in on success, untouched otherwise
 * @param nSlot         first slot the chain occupies, from PROTO_ACK.aRecSlot
 * @param nSlotWidth    1 for mono, 2 for a stereo pair
 * @param nStreamWidth  slots per frame, from PROTO_ACK.nStreamWidth
 * @param nFrames       frames in the block
 *
 * @return RESULT_OK, or RESULT_INVALID_PARAM_* when the request does not fit
 *         inside a frame - which is a configuration error, not a runtime one,
 *         and must stop the stream rather than transfer something plausible
 */
extern STD_RESULT FxInterleave_Xfer(FX_IL_XFER* const pOut,
                                    const U8 nSlot,
                                    const U8 nSlotWidth,
                                    const U8 nStreamWidth,
                                    const U32 nFrames);

/** Bytes one whole interleaved block occupies. Zero if the arguments are absurd. */
extern U32 FxInterleave_BlockBytes(const U8 nStreamWidth, const U32 nFrames);



#ifdef __cplusplus
}
#endif

#endif // #ifndef FX_INTERLEAVE_H

/****************************************** end of file *******************************************/
