/**
 * @file      recorder.c
 *
 * @details   Recorder tap implementation. See recorder.h for the model.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "recorder.h"

#include "mem_map.h"
#include "Effects/fx_common.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

#define REC_SAMPLE_SCALE            (8388608.0f)    /* 2^23 */
#define REC_SAMPLE_MAX              (8388607L)
#define REC_SAMPLE_MIN              (-8388608L)



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Interleaved stream block: [frame][slot]. DTCM.
 *
 * 64 frames x 4 slots x 4 B = 1 KiB.
 */
static S32 aStream[AUDIO_BLOCK_FRAMES * REC_SLOT_QTY] IN_DTCM MEM_ALIGN(32);

static U16 nStreamFrames IN_DTCM;

/* Cached from the grid so the ISR does not have to walk the slot table. */
static U8  aSlotOfChain[CHAIN_MAX_QTY] IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static S32 ToStream(const FLOAT32 fValue)
{
    S32 nSample = (S32)(FxUtil_Clamp(fValue, -1.0f, 1.0f) * REC_SAMPLE_SCALE);

    if (nSample > REC_SAMPLE_MAX)
    {
        nSample = REC_SAMPLE_MAX;
    }
    else if (nSample < REC_SAMPLE_MIN)
    {
        nSample = REC_SAMPLE_MIN;
    }
    else
    {
        do_nothing();
    }

    return nSample;
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT Recorder_Init(void)
{
    U16 i;

    for (i = 0U; i < (U16)(AUDIO_BLOCK_FRAMES * REC_SLOT_QTY); i++)
    {
        aStream[i] = 0L;
    }

    for (i = 0U; i < CHAIN_MAX_QTY; i++)
    {
        aSlotOfChain[i] = (U8)REC_SLOT_NONE;
    }

    nStreamFrames = 0U;

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

void Recorder_Apply(const GRID* const pGrid)
{
    U8 nChain;

    for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
    {
        aSlotOfChain[nChain] = pGrid->aRecSlot[nChain];
    }
}

//--------------------------------------------------------------------------------------------------

void Recorder_BeginBlock(const U16 nFrames)
{
    U16 i;
    const U16 nTotal = (U16)(nFrames * REC_SLOT_QTY);

    // Silence everything. Slots without a tap must transmit silence, not the
    // previous block's audio.
    for (i = 0U; i < nTotal; i++)
    {
        aStream[i] = 0L;
    }

    nStreamFrames = nFrames;
}

//--------------------------------------------------------------------------------------------------

void Recorder_Process(const GRID* const pGrid,
                      const U8 nChain,
                      FLOAT32* const apChain[],
                      const U16 nFrames)
{
    const U8 nSlotBase = aSlotOfChain[nChain];
    const U8 nWidth    = pGrid->aWidth[nChain];
    U8       p;
    U16      i;

    if (nSlotBase == (U8)REC_SLOT_NONE)
    {
        return;
    }

    for (p = 0U; p < nWidth; p++)
    {
        const FLOAT32* const pIn   = apChain[p];
        const U8             nSlot = nSlotBase + p;

        if (nSlot >= (U8)REC_SLOT_QTY)
        {
            break;                          // cannot happen; cheap insurance
        }

        for (i = 0U; i < nFrames; i++)
        {
            aStream[((U16)i * REC_SLOT_QTY) + nSlot] = ToStream(pIn[i]);
        }
    }

    // Audio passes through untouched: a tap must be inaudible.
}

//--------------------------------------------------------------------------------------------------

const S32* Recorder_GetStream(U16* const pFrameQty)
{
    if (pFrameQty != NULL_PTR)
    {
        *pFrameQty = nStreamFrames;
    }

    return aStream;
}

/****************************************** end of file *******************************************/
