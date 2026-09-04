/**
 * @file      rec_stream.c
 *
 * @details   Recorder stream staging. See rec_stream.h for the model and for
 *            the invariant this file exists to hold.
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

#include "rec_stream.h"

#include "mem_map.h"



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * RAM_D2, and it has to be: DMA1 cannot reach the DTCM the recorder interleaves
 * into. 2 x 1 KiB. Cache line aligned because everything the DMA touches is.
 */
static S32 aStage[REC_STAGE_QTY][REC_STAGE_WORDS] IN_DMA_BUF MEM_ALIGN(32);

/* Words staged per half. Always REC_STAGE_WORDS in practice, tracked anyway so
 * a short final block cannot transmit stale words off the end. */
static U16 anWords[REC_STAGE_QTY] IN_DTCM;

static BOOLEAN bEnabled IN_DTCM;

/** A transfer is running out of nSending. */
static BOOLEAN bBusy    IN_DTCM;
static U8      nSending IN_DTCM;

/** A staged half is waiting for the transfer in flight to finish. */
static BOOLEAN bWaiting IN_DTCM;
static U8      nWaiting IN_DTCM;

static REC_STREAM_STATS tStats IN_DTCM;

/*
 * Frame sequence number for the sync slot. Free-running, 16-bit, wrapping.
 *
 * NOT reset by ResetMachine, and that is deliberate. It numbers frames put on
 * the wire, so if the staging machine restarts - a dropped block, a disabled
 * stream, an aborted transfer - the counter keeps going and the receiver sees
 * a GAP of exactly the right size. Resetting it would tell the receiver the
 * link had restarted from frame zero, which is a different fault with a
 * different response, and would hide however many frames were actually lost.
 *
 * Wraps every 65536 frames, 1.37 s at 48 kHz. The receiver's check is
 * "expected + 1" in 16-bit arithmetic, so the wrap costs nothing; a gap is only
 * ambiguous if more than 65535 consecutive frames are lost, by which point the
 * link has been down for over a second and the counter is not the diagnosis.
 */
static U16 nSeq IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief The half that is neither in flight nor waiting.
 *
 * With REC_STAGE_QTY of 2 this is a two-line search, and writing it as a search
 * rather than as "the other one" is deliberate: it is correct for any staging
 * depth, and it cannot return the buffer the DMA is reading no matter what
 * order the two interrupts happen to arrive in.
 */
static U8 FreeHalf(void)
{
    U8 nHalf;
    U8 nFree = (U8)REC_STAGE_NONE;

    for (nHalf = 0U; nHalf < (U8)REC_STAGE_QTY; nHalf++)
    {
        const BOOLEAN bInFlight = ((bBusy   != FALSE) && (nSending == nHalf)) ? TRUE : FALSE;
        const BOOLEAN bQueued   = ((bWaiting != FALSE) && (nWaiting == nHalf)) ? TRUE : FALSE;

        if ((bInFlight == FALSE) && (bQueued == FALSE))
        {
            nFree = nHalf;
            break;
        }
    }

    return nFree;
}

//--------------------------------------------------------------------------------------------------

static void ResetMachine(void)
{
    U8 nHalf;

    bBusy    = FALSE;
    bWaiting = FALSE;
    nSending = 0U;
    nWaiting = 0U;

    for (nHalf = 0U; nHalf < (U8)REC_STAGE_QTY; nHalf++)
    {
        anWords[nHalf] = 0U;
    }
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT RecStream_Init(void)
{
    U8  nHalf;
    U16 i;

    /* .dtcm is NOLOAD, so nothing here is zeroed for us - see mem_map.h. */
    for (nHalf = 0U; nHalf < (U8)REC_STAGE_QTY; nHalf++)
    {
        for (i = 0U; i < (U16)REC_STAGE_WORDS; i++)
        {
            aStage[nHalf][i] = 0L;
        }
    }

    /* The sync word the far side scans for is built from this. Left
       uninitialised it starts at whatever DTCM held, which the slave adopts
       through SEQ_ANY and then treats a later restart as a huge gap. */
    nSeq = 0U;

    bEnabled = FALSE;

    tStats.nBlocksSent    = 0UL;
    tStats.nBlocksDropped = 0UL;
    tStats.nErrors        = 0UL;

    ResetMachine();

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

void RecStream_Enable(const BOOLEAN bOn)
{
    /*
     * bBusy and nSending are deliberately LEFT ALONE in both directions.
     *
     * The obvious implementation resets the whole machine on enable. That is
     * wrong, and wrong in the one way this module exists to prevent: if a
     * transfer is still in flight - enable arriving hard on the heels of a
     * disable - clearing bBusy makes the half the DMA is reading look free, and
     * the next block is written straight into it.
     *
     * Leaving them truthful costs nothing. The stale transfer finishes, and
     * because it is a WHOLE block it is an exact multiple of the REC_SLOT_QTY
     * stride, so it cannot rotate the interface's de-interleave - it only
     * prepends one block of older audio, 1.33 ms, to a recording. Meanwhile
     * Stage picks a genuinely free half and the new stream begins on a block
     * boundary, which is the guarantee that actually matters.
     *
     * A queued block, by contrast, has no such claim on anything: nothing is
     * reading it, so it is dropped on either edge rather than being sent late.
     */
    bWaiting = FALSE;

    bEnabled = (bOn != FALSE) ? TRUE : FALSE;
}

//--------------------------------------------------------------------------------------------------

BOOLEAN RecStream_IsEnabled(void)
{
    return bEnabled;
}

//--------------------------------------------------------------------------------------------------

U8 RecStream_Stage(const S32* const pSrc, const U16 nFrames, const U8 nTotalSlots)
{
    U8  nStart = (U8)REC_STAGE_NONE;
    U8  nFree;
    U16 nUse;
    U16 nWords;
    U16 f;

    if ((bEnabled == FALSE) || (pSrc == NULL_PTR) || (nFrames == 0U))
    {
        return (U8)REC_STAGE_NONE;
    }

    /*
     * Exactly the frame width, not a range. The frame is fixed, so anything
     * else is a caller that disagrees with fx_frame.h about the wire - and a
     * disagreement about where a frame ends is the one error this stream cannot
     * survive.
     *
     * NOT counted as a dropped block, for the same reason a NULL pointer is
     * not: nBlocksDropped means the link could not keep up and audio was lost,
     * and it is the number you look at when a recording has a gap. A caller
     * bug inflating it sends you to investigate the wrong thing.
     */
    if (nTotalSlots != (U8)FX_FRAME_SLOT_QTY)
    {
        return (U8)REC_STAGE_NONE;
    }

    nFree = FreeHalf();

    if (nFree == (U8)REC_STAGE_NONE)
    {
        /* Drop the block. Never write a half the DMA might be reading: that
           would not lose a block, it would splice two. */
        tStats.nBlocksDropped++;
        return (U8)REC_STAGE_NONE;
    }

    nUse   = (nFrames > (U16)AUDIO_BLOCK_FRAMES) ? (U16)AUDIO_BLOCK_FRAMES : nFrames;
    nWords = (U16)(nUse * (U16)nTotalSlots);

    /*
     * Frame by frame at the wire stride: the sync word, REC_SLOT_QTY recorder
     * samples, then the loop slots zeroed for the transport to overwrite.
     *
     * THE SYNC WORD IS THE FIRST THING WRITTEN, and the receiver checks it
     * before it trusts anything else in the frame. It is what makes a slipped
     * word show up as a counted dropout instead of every channel silently
     * moving one place to the left. nSeq free-runs across blocks and across
     * sessions, so a gap in it measures frames LOST rather than frames not
     * sent.
     *
     * The zeroing of the loop slots is not tidiness. Without it an aborted or
     * finished loop transfer leaves its last block in the staging buffer, and
     * the next frame carries that stale audio in slots the interface is still
     * routing.
     */
    for (f = 0U; f < nUse; f++)
    {
        const U16 nDst = (U16)(f * (U16)nTotalSlots);
        const U16 nSrc = (U16)(f * (U16)REC_SLOT_QTY);
        U8        s;

        aStage[nFree][nDst + (U16)FX_FRAME_SYNC_SLOT] = FX_FRAME_SYNC_WORD(nSeq);
        nSeq++;

        for (s = 0U; s < (U8)REC_SLOT_QTY; s++)
        {
            aStage[nFree][nDst + (U16)FX_FRAME_REC_SLOT_BASE + s] = pSrc[nSrc + s];
        }

        for (s = 0U; s < (U8)FX_FRAME_LOOP_SLOT_QTY; s++)
        {
            aStage[nFree][nDst + (U16)FX_FRAME_LOOP_SLOT_BASE + s] = 0L;
        }
    }

    anWords[nFree] = nWords;

    if (bBusy != FALSE)
    {
        bWaiting = TRUE;
        nWaiting = nFree;
    }
    else
    {
        bBusy    = TRUE;
        nSending = nFree;
        nStart   = nFree;
    }

    return nStart;
}

//--------------------------------------------------------------------------------------------------

U8 RecStream_Complete(void)
{
    U8 nStart = (U8)REC_STAGE_NONE;

    tStats.nBlocksSent++;

    if (bWaiting != FALSE)
    {
        /* Straight into the next one - bBusy stays set, so the half that just
           finished is the only free one and the next block lands there. */
        nSending = nWaiting;
        bWaiting = FALSE;
        nStart   = nSending;
    }
    else
    {
        bBusy = FALSE;
    }

    return nStart;
}

//--------------------------------------------------------------------------------------------------

void RecStream_Error(void)
{
    tStats.nErrors++;

    /* Whatever was staged is now of unknown age relative to the stream, and the
       interface has no way to resynchronise by content. Drop everything and let
       the next block start clean. */
    ResetMachine();
}

//--------------------------------------------------------------------------------------------------

const S32* RecStream_Buffer(const U8 nHalf)
{
    return (nHalf < (U8)REC_STAGE_QTY) ? &aStage[nHalf][0] : NULL_PTR;
}

//--------------------------------------------------------------------------------------------------

S32* RecStream_StageSlots(const U8 nHalf)
{
    /* The one writable view of a staged block. See rec_stream.h for why it
       exists at all rather than the loop transport staging its own copy. */
    return (nHalf < (U8)REC_STAGE_QTY) ? &aStage[nHalf][0] : NULL_PTR;
}

//--------------------------------------------------------------------------------------------------

U16 RecStream_Words(const U8 nHalf)
{
    return (nHalf < (U8)REC_STAGE_QTY) ? anWords[nHalf] : 0U;
}

//--------------------------------------------------------------------------------------------------

const REC_STREAM_STATS* RecStream_Stats(void)
{
    return &tStats;
}

/****************************************** end of file *******************************************/
