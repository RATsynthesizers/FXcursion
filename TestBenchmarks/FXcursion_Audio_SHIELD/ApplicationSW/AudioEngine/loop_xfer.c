/***************************************************************************************************
* @file     loop_xfer.c
*
* @brief    Audio side of the loop transport. See loop_xfer.h.
*
***************************************************************************************************/

/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "loop_xfer.h"

#include <string.h>

#include "loop_mem.h"
#include "audio_cfg.h"
#include "mem_map.h"

/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

static FX_LOOP_SESSION tSession;

/**
 * Where the next byte comes from, or goes.
 *
 * A byte offset into the CONCATENATION of the looper's planes, not into one of
 * them: a stereo loop goes over the wire as plane 0 in full followed by plane 1
 * in full, which is what the interface stages and what the WAV writer then
 * interleaves. Keeping one offset rather than a plane index plus a position is
 * what makes the plane boundary fall out of the arithmetic instead of being a
 * special case that a partial block could straddle wrongly.
 */
static U32 nByteOfs;

/** First plane of the addressed looper. */
static U8  nPlaneBase;

/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Byte at a given offset in the looper's concatenated planes.
 *
 * @return NULL_PTR when the offset is past the end
 */
static U8* LoopXfer_At(const U32 nOfs)
{
    const U32 nPerPlane = LoopMem_PlaneBytes();
    U8        nPlane;
    U8*       pBase;

    if (nPerPlane == 0UL)
    {
        return NULL_PTR;
    }

    nPlane = (U8)(nPlaneBase + (U8)(nOfs / nPerPlane));

    if (nPlane >= (U8)AUDIO_PLANE_QTY)
    {
        return NULL_PTR;
    }

    pBase = LoopMem_PlaneBase(nPlane, (U8)LOOP_COPY_TAKE);

    if (pBase == NULL_PTR)
    {
        return NULL_PTR;
    }

    return &pBase[nOfs % nPerPlane];
}

/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT LoopXfer_Init(void)
{
    FxLoop_Reset(&tSession);

    nByteOfs   = 0UL;
    nPlaneBase = 0U;

    return RESULT_OK;
}


STD_RESULT LoopXfer_OnOpen(const PROTO_LOOP_OPEN* const pOpen,
                           const U32 nLoopFrames,
                           PROTO_LOOP_STAT* const pStat)
{
    U32        nAvail = 0UL;
    STD_RESULT eResult;

    if ((pOpen == NULL_PTR) || (pStat == NULL_PTR))
    {
        return RESULT_NOT_OK;
    }

    /*
     * What this side can supply or accept, in payload bytes.
     *
     * SAVE: exactly what is recorded. FxLoop_Accept turns an nBytes of 0 into
     *       this, which is how the interface asks a question it cannot answer -
     *       only the audio board knows how long the take is.
     *
     * LOAD: the whole of a plane buffer, since a load overwrites everything.
     *
     * Both go through FxLoop_BytesFor, the SAME function the interface sizes
     * with, so the two cannot arrive at different numbers for the same loop.
     */
    if (pOpen->eDir == (U8)LOOP_DIR_SAVE)
    {
        if (FxLoop_BytesFor(nLoopFrames, pOpen->nPlaneQty,
                            pOpen->eFormat, &nAvail) != RESULT_OK)
        {
            nAvail = 0UL;
        }
    }
    else
    {
        const U32 nMaxFrames = LoopMem_PlaneBytes() / (U32)LOOP_BYTES_PER_SAMPLE;

        if (FxLoop_BytesFor(nMaxFrames, pOpen->nPlaneQty,
                            pOpen->eFormat, &nAvail) != RESULT_OK)
        {
            nAvail = 0UL;
        }
    }

    eResult = FxLoop_Accept(&tSession, pOpen, nAvail, pStat);

    if (eResult == RESULT_OK)
    {
        nPlaneBase = (U8)(tSession.nLooper * (U8)MEM_LOOP_PLANES_PER_LOOPER);
        nByteOfs   = 0UL;
    }

    return eResult;
}


STD_RESULT LoopXfer_OnCtl(const PROTO_LOOP_CTL* const pCtl)
{
    if (pCtl == NULL_PTR)
    {
        return RESULT_NOT_OK;
    }

    /* A control for a session that has already gone. Ignored rather than
       treated as an error on the live one - see FxLoop_OpenReply for why a
       stale message must not kill its successor. */
    if (pCtl->nSession != tSession.nSession)
    {
        return RESULT_NOT_OK;
    }

    if (pCtl->eAction == (U8)LOOP_ACT_ABORT)
    {
        FxLoop_Abort(&tSession, (U8)PROTO_RES_OK);
        return RESULT_OK;
    }

    if (pCtl->eAction != (U8)LOOP_ACT_START)
    {
        return RESULT_NOT_OK;
    }

    nByteOfs = 0UL;

    return FxLoop_Start(&tSession);
}


U8 LoopXfer_StreamWidth(void)
{
    return FxLoop_StreamWidth(&tSession);
}


BOOLEAN LoopXfer_IsRunning(void)
{
    return FxLoop_IsStreaming(&tSession);
}


const FX_LOOP_SESSION* LoopXfer_Session(void)
{
    return &tSession;
}


void LoopXfer_Report(PROTO_LOOP_STAT* const pStat)
{
    FxLoop_Report(&tSession, pStat);
}


STD_RESULT LoopXfer_Block(S32* const pSlots, const U32 nFrames, const U8 nStride)
{
    const U8  nSlots = tSession.nSlotQty;
    U32       nFrame;
    U32       nMoved = 0UL;

    if (pSlots == NULL_PTR)
    {
        return RESULT_NOT_OK;
    }

    /* The loop slots have to fit inside the frame they are being written into.
       A stride too small would have each frame's loop slots overwrite the next
       frame's recorder samples - silently, since nothing downstream can tell a
       corrupted sample from a quiet one. */
    if (nStride < (U8)(nSlots + (U8)REC_SLOT_QTY))
    {
        return RESULT_INVALID_PARAM_3;
    }

    if (FxLoop_IsStreaming(&tSession) == FALSE)
    {
        return RESULT_NOT_OK;
    }

    for (nFrame = 0UL; nFrame < nFrames; nFrame++)
    {
        U8 s;

        for (s = 0U; s < nSlots; s++)
        {
            /*
             * FOUR payload bytes per slot - the loop payload is a BYTE STREAM.
             *
             * The recorder puts one 24-bit sample in each 32-bit slot because
             * its slots ARE channels: the interface de-interleaves them by
             * position into separate planes, so a sample has to sit in a known
             * slot. A loop has no such requirement. It is one contiguous run of
             * bytes lifted as a single transfer, and where a sample boundary
             * falls inside that run is nobody's business until the WAV header
             * is written.
             *
             * This carried three bytes and a pad byte at first, mirroring the
             * recorder out of habit. That cost 25% of the wire AND forced the
             * interface's staging to hold S32 - so a 5.5 MiB slot held only
             * 4.1 MiB of loop, and 20 seconds of stereo became 14.3.
             *
             * At four bytes the staging holds EXACTLY the payload, the far side
             * writes what it received straight to the card with no narrowing
             * anywhere, and a loop moves a third faster.
             */
            U32 nWord = 0UL;
            U8  b;

            for (b = 0U; b < 4U; b++)
            {
                const U32 nOfs = nByteOfs + nMoved;
                U8* const pAt  = LoopXfer_At(nOfs);

                /* Past the end of the payload: pad. A loop rarely ends on a
                   slot boundary, and the far side stops at nBytesTotal rather
                   than at the end of the block, so these bytes are counted but
                   never written to the file. */
                if ((nOfs >= tSession.nBytesTotal) || (pAt == NULL_PTR))
                {
                    continue;
                }

                if (tSession.eDir == (U8)LOOP_DIR_SAVE)
                {
                    nWord |= ((U32)(*pAt)) << (8U * b);
                }
                else
                {
                    *pAt = (U8)((((U32)pSlots[(nFrame * (U32)nStride) + s]) >> (8U * b)) & 0xFFUL);
                }

                nMoved++;
            }

            if (tSession.eDir == (U8)LOOP_DIR_SAVE)
            {
                pSlots[(nFrame * (U32)nStride) + s] = (S32)nWord;
            }
        }

        /* Whole block emitted once the payload is exhausted; the remaining
           slots this block are padding and the session completes below. */
        if ((nByteOfs + nMoved) >= tSession.nBytesTotal)
        {
            break;
        }
    }

    if (nMoved == 0UL)
    {
        return RESULT_OK;
    }

    /*
     * CRC over exactly the bytes that moved, in the order they moved.
     *
     * Taken from the LOOPER, not from the slot words, in both directions. On a
     * save that is the same memory the far side will checksum after unpacking;
     * on a load it is what actually landed. Checksumming the wire words instead
     * would agree with itself even when the pack or unpack was wrong.
     */
    {
        U8* const pStart = LoopXfer_At(nByteOfs);
        U32       nSpan  = nMoved;

        /* One CRC call cannot cross a plane boundary - the planes are separate
           buffers - so split there when the run straddles one. */
        if (pStart != NULL_PTR)
        {
            const U32 nPerPlane = LoopMem_PlaneBytes();
            const U32 nInPlane  = nPerPlane - (nByteOfs % nPerPlane);

            if (nSpan > nInPlane)
            {
                (void)FxLoop_Advance(&tSession, pStart, nInPlane);

                nSpan -= nInPlane;
                (void)FxLoop_Advance(&tSession, LoopXfer_At(nByteOfs + nInPlane), nSpan);
            }
            else
            {
                (void)FxLoop_Advance(&tSession, pStart, nSpan);
            }
        }
        else
        {
            (void)FxLoop_Advance(&tSession, NULL_PTR, nSpan);
        }
    }

    nByteOfs += nMoved;

    return RESULT_OK;
}

/****************************************** end of file *******************************************/
