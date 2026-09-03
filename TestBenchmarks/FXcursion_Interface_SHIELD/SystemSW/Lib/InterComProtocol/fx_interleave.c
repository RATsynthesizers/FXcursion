/**
 * @file      fx_interleave.c
 *
 * @details   Recorder stream geometry. See fx_interleave.h for the layout.
 *
 *            ############################################################
 *            #  DUPLICATED IN THE INTERFACE CONTROLLER - keep in sync.  #
 *            #  Use TestBenchmarks/sync_shared.py.                      #
 *            ############################################################
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_interleave.h"



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT FxInterleave_Xfer(FX_IL_XFER* const pOut,
                             const U8 nSlot,
                             const U8 nSlotWidth,
                             const U8 nStreamWidth,
                             const U32 nFrames)
{
    U32 nStrideBytes;

    if (pOut == NULL_PTR)
    {
        return RESULT_INVALID_PARAM_1;
    }

    /* A chain is mono or a stereo PAIR. Nothing else exists - see the width
       invariant in fx_defs.h - and accepting anything else here would compute a
       transfer that runs off the end of a frame. */
    if ((nSlotWidth != 1U) && (nSlotWidth != 2U))
    {
        return RESULT_INVALID_PARAM_3;
    }

    if ((nStreamWidth == 0U) || (nStreamWidth > (U8)REC_SLOT_QTY))
    {
        return RESULT_INVALID_PARAM_4;
    }

    /*
     * THE CHECK THAT MATTERS.
     *
     * A stereo chain in the last slot, or a slot index at or past the stream
     * width, would read the next frame's samples as if they were this frame's.
     * That is not a transfer to clamp - it is a configuration that must never
     * have been ACKed, so refuse it and let the caller stop the stream rather
     * than record something plausible from the wrong channel.
     */
    if (((U32)nSlot + (U32)nSlotWidth) > (U32)nStreamWidth)
    {
        return RESULT_INVALID_PARAM_2;
    }

    if (nFrames == 0UL)
    {
        return RESULT_INVALID_PARAM_5;
    }

    nStrideBytes = (U32)nStreamWidth * FX_IL_BYTES_PER_SLOT;

    pOut->nSrcOffsetBytes = (U32)nSlot * FX_IL_BYTES_PER_SLOT;
    pOut->nBytesPerBeat   = (U32)nSlotWidth * FX_IL_BYTES_PER_SLOT;
    pOut->nSrcSkipBytes   = nStrideBytes - pOut->nBytesPerBeat;
    pOut->nBeats          = nFrames;
    pOut->nDstBytes       = nFrames * pOut->nBytesPerBeat;

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

U32 FxInterleave_BlockBytes(const U8 nStreamWidth, const U32 nFrames)
{
    U32 nBytes = 0UL;

    if ((nStreamWidth > 0U) && (nStreamWidth <= (U8)REC_SLOT_QTY) && (nFrames > 0UL))
    {
        nBytes = nFrames * (U32)nStreamWidth * FX_IL_BYTES_PER_SLOT;
    }

    return nBytes;
}

/****************************************** end of file *******************************************/
