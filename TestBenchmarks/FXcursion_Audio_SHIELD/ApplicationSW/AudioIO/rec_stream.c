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

U8 RecStream_Stage(const S32* const pSrc, const U16 nFrames)
{
    U8  nStart = (U8)REC_STAGE_NONE;
    U8  nFree;
    U16 nWords;
    U16 i;

    if ((bEnabled == FALSE) || (pSrc == NULL_PTR) || (nFrames == 0U))
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

    nWords = (U16)(((nFrames > (U16)AUDIO_BLOCK_FRAMES) ? (U16)AUDIO_BLOCK_FRAMES : nFrames)
                   * REC_SLOT_QTY);

    for (i = 0U; i < nWords; i++)
    {
        aStage[nFree][i] = pSrc[i];
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
