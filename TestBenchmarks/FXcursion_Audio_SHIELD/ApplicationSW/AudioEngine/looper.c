/**
 * @file      looper.c
 *
 * @details   Looper implementation. See looper.h for the model.
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

#include "looper.h"

#include "mem_map.h"
#include "loop_mem.h"
#include "params.h"
#include "Effects/fx_common.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/** Planes per looper. 4 planes / 2 loopers. */
#define LOOP_PLANES_PER_LOOPER      (AUDIO_PLANE_QTY / LOOPER_QTY)

/* Packed loop audio is the same 24-bit format as the converters, so it shares
 * their full-scale definition rather than restating it. */
#define LOOP_SAMPLE_SCALE           (AUDIO_FULLSCALE)
#define LOOP_SAMPLE_MAX             (AUDIO_SAMPLE_MAX)
#define LOOP_SAMPLE_MIN             (AUDIO_SAMPLE_MIN)



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

/**
 * @brief Per-chain transport.
 */
typedef struct stLOOP_TRANSPORT
{
    U8      eState;             /**< PROTO_TRANSPORT_ACT                         */
    U8      nReserved[3];
    U32     nPos;               /**< playhead, frames                            */

} LOOP_TRANSPORT;

/**
 * @brief Per-pair length.
 */
typedef struct stLOOP_LENGTH
{
    U8      nBars;              /**< 0 = empty                                   */
    U8      nReserved[3];
    U32     nFrames;            /**< 0 = empty                                   */
    FLOAT32 fRecordedBpm;       /**< tempo at capture; re-lock placeholder       */

} LOOP_LENGTH;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * There is no loop buffer here any more.
 *
 * Loop audio lives in the two SDRAM banks - see loop_mem.h. This module works
 * on a one-block DTCM window per plane and knows nothing about where the audio
 * actually is, which is why moving it out of the QSPI PSRAM changed the
 * include below and nothing else here.
 */
static LOOP_TRANSPORT aTransport[CHAIN_MAX_QTY] IN_DTCM;
static LOOP_LENGTH    aLength[LOOPER_QTY]       IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static U8 LooperOfPlane(const U8 nPlane)
{
    return (U8)(nPlane / LOOP_PLANES_PER_LOOPER);
}

//--------------------------------------------------------------------------------------------------

static void Pack24(U8* const pDst, const FLOAT32 fValue)
{
    S32 nSample = (S32)(FxUtil_Clamp(fValue, -1.0f, 1.0f) * LOOP_SAMPLE_SCALE);

    if (nSample > LOOP_SAMPLE_MAX)
    {
        nSample = LOOP_SAMPLE_MAX;
    }
    else if (nSample < LOOP_SAMPLE_MIN)
    {
        nSample = LOOP_SAMPLE_MIN;
    }
    else
    {
        do_nothing();
    }

    pDst[0] = (U8)((U32)nSample & 0xFFU);
    pDst[1] = (U8)(((U32)nSample >> 8U) & 0xFFU);
    pDst[2] = (U8)(((U32)nSample >> 16U) & 0xFFU);
}

//--------------------------------------------------------------------------------------------------

static FLOAT32 Unpack24(const U8* const pSrc)
{
    U32 nRaw = (U32)pSrc[0] | ((U32)pSrc[1] << 8U) | ((U32)pSrc[2] << 16U);
    S32 nSample;

    if ((nRaw & 0x00800000UL) != 0UL)
    {
        nRaw |= 0xFF000000UL;               // sign extend from 24 to 32 bits
    }

    nSample = (S32)nRaw;

    return (FLOAT32)nSample * (1.0f / LOOP_SAMPLE_SCALE);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Ask the store for a window at nPos on every plane of a chain.
 */
static void ArmChain(const U8 nChain, const U32 nPos, const U32 nLen, const BOOLEAN bBlank)
{
    const GRID* const pGrid  = Grid_Active();
    const U8          nBase  = pGrid->aPlaneBase[nChain];
    const U8          nWidth = pGrid->aWidth[nChain];
    U8                p;

    for (p = 0U; p < nWidth; p++)
    {
        if (bBlank != FALSE)
        {
            LoopMem_ArmBlank((U8)(nBase + p), nPos, nLen);
        }
        else
        {
            LoopMem_Arm((U8)(nBase + p), nPos, nLen);
        }
    }
}

//--------------------------------------------------------------------------------------------------

static BOOLEAN PairIsIdle(const GRID* const pGrid, const U8 nLooper)
{
    BOOLEAN bIdle = TRUE;
    U8      nChain;

    for (nChain = 0U; nChain < pGrid->nChainQty; nChain++)
    {
        if (LooperOfPlane(pGrid->aPlaneBase[nChain]) == nLooper)
        {
            if (aTransport[nChain].eState != (U8)TRANSPORT_STOP)
            {
                bIdle = FALSE;
                break;
            }
        }
    }

    return bIdle;
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT Looper_Init(void)
{
    U8 i;

    for (i = 0U; i < CHAIN_MAX_QTY; i++)
    {
        aTransport[i].eState       = (U8)TRANSPORT_STOP;
        aTransport[i].nReserved[0] = 0U;
        aTransport[i].nReserved[1] = 0U;
        aTransport[i].nReserved[2] = 0U;
        aTransport[i].nPos         = 0UL;
    }

    for (i = 0U; i < LOOPER_QTY; i++)
    {
        aLength[i].nBars        = 0U;
        aLength[i].nReserved[0] = 0U;
        aLength[i].nReserved[1] = 0U;
        aLength[i].nReserved[2] = 0U;
        aLength[i].nFrames      = 0UL;      // 0 = empty; the PSRAM garbage is never read
        aLength[i].fRecordedBpm = 0.0f;
    }

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

void Looper_TopologyChanged(void)
{
    U8 i;

    // Stop only. The recorded audio and the lengths are left alone - see the
    // rationale in looper.h.
    for (i = 0U; i < CHAIN_MAX_QTY; i++)
    {
        aTransport[i].eState = (U8)TRANSPORT_STOP;
        aTransport[i].nPos   = 0UL;
    }

    // The windows describe positions in a plane layout that no longer applies.
    LoopMem_Invalidate();
}

//--------------------------------------------------------------------------------------------------

void Looper_Apply(const PROTO_CFG* const pCfg, const GRID* const pGrid)
{
    U8 nLooper;

    for (nLooper = 0U; nLooper < LOOPER_QTY; nLooper++)
    {
        const U8 nBars = pCfg->aLoopBars[nLooper];

        // Length is shared by the pair, so it may only change when BOTH of its
        // transports are stopped. Truncating audio playing on the other chain
        // would be worse than refusing.
        if (PairIsIdle(pGrid, nLooper) != FALSE)
        {
            aLength[nLooper].nBars = nBars;

            if (nBars == 0U)
            {
                aLength[nLooper].nFrames = 0UL;
            }
            else
            {
                U32 nFrames = (U32)nBars * Params_BarFrames();

                if (nFrames > LOOP_MAX_FRAMES)
                {
                    nFrames = LOOP_MAX_FRAMES;      // GUI should have bounded this
                }

                aLength[nLooper].nFrames = nFrames;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------

STD_RESULT Looper_Transport(const U8 nChain, const U8 eAction)
{
    STD_RESULT eResult = RESULT_OK;

    if (nChain >= CHAIN_MAX_QTY)
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else
    {
        const U8 nLooper = LooperOfPlane(Grid_Active()->aPlaneBase[nChain]);

        switch (eAction)
        {
            case (U8)TRANSPORT_RECORD:
                if (aLength[nLooper].nFrames == 0UL)
                {
                    eResult = RESULT_NOT_OK;        // no bar count set yet
                }
                else
                {
                    aTransport[nChain].nPos       = 0UL;
                    aLength[nLooper].fRecordedBpm = Params_Tempo()->fBpm;
                    aTransport[nChain].eState     = (U8)TRANSPORT_RECORD;
                    // Blank: a take must start on the sample the player hit
                    // the button on, and the first block overwrites the whole
                    // window anyway.
                    ArmChain(nChain, 0UL, aLength[nLooper].nFrames, TRUE);
                }
                break;

            case (U8)TRANSPORT_OVERDUB:
            case (U8)TRANSPORT_PLAY:
                if (aLength[nLooper].nFrames == 0UL)
                {
                    eResult = RESULT_NOT_OK;
                }
                else
                {
                    aTransport[nChain].eState = eAction;
                    // PLAY and OVERDUB both need the audio that is already
                    // there, so these wait the one block for the fetch.
                    ArmChain(nChain, aTransport[nChain].nPos, aLength[nLooper].nFrames, FALSE);
                }
                break;

            case (U8)TRANSPORT_STOP:
                aTransport[nChain].eState = (U8)TRANSPORT_STOP;
                break;

            case (U8)TRANSPORT_CLEAR:
                // Only the transport and the length are cleared. The PSRAM
                // contents are left alone: erasing 8.6 MiB per plane would take
                // the best part of a second, and nothing reads an empty loop.
                aTransport[nChain].eState = (U8)TRANSPORT_STOP;
                aTransport[nChain].nPos   = 0UL;
                aLength[nLooper].nFrames  = 0UL;
                aLength[nLooper].nBars    = 0U;
                ArmChain(nChain, 0UL, 0UL, FALSE);  // zeroes and invalidates the windows
                break;

            default:
                eResult = RESULT_INVALID_PARAM_1;
                break;
        }
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

void Looper_Process(const GRID* const pGrid,
                    const U8 nChain,
                    FLOAT32* const apChain[],
                    const U16 nFrames)
{
    const U8  nBase   = pGrid->aPlaneBase[nChain];
    const U8  nWidth  = pGrid->aWidth[nChain];
    const U8  nLooper = LooperOfPlane(nBase);
    const U32 nLen    = aLength[nLooper].nFrames;
    const U8  eState  = aTransport[nChain].eState;
    const U32 nPos    = aTransport[nChain].nPos;

    BOOLEAN bDirty;
    U32     nNext;
    U8      p;
    U16     i;
    U16     nCount = nFrames;

    if ((nLen == 0UL) || (eState == (U8)TRANSPORT_STOP))
    {
        return;                                     // passthrough
    }

    // The store still owns the windows, either because a fetch is in flight or
    // because the chain overran. Stay dry and hold the position: a block of
    // missing loop audio is recoverable, a corrupted recording is not.
    if ((LoopMem_Ready() == FALSE) || (LoopMem_Valid(nBase) == FALSE))
    {
        return;
    }

    if (nCount > (U16)LOOP_WINDOW_FRAMES)
    {
        nCount = (U16)LOOP_WINDOW_FRAMES;           // a window is exactly one block
    }

    bDirty = ((eState == (U8)TRANSPORT_RECORD) || (eState == (U8)TRANSPORT_OVERDUB))
                 ? TRUE : FALSE;

    for (p = 0U; p < nWidth; p++)
    {
        FLOAT32* const pBuf = apChain[p];
        U8* const      pWin = LoopMem_Window((U8)(nBase + p));

        if (pWin == NULL_PTR)
        {
            continue;
        }

        // Note what is NOT here any more: no nPos, no wrap test, no multiply
        // against a 25 MiB base. The window is one block of DTCM and the walk
        // is linear, so the wrap is handled once per block by the store instead
        // of once per sample here.
        for (i = 0U; i < nCount; i++)
        {
            U8* const     pSlot = &pWin[i * LOOP_BYTES_PER_SAMPLE];
            const FLOAT32 fIn   = pBuf[i];

            switch (eState)
            {
                case (U8)TRANSPORT_RECORD:
                    Pack24(pSlot, fIn);
                    // Output stays dry while recording, so the player hears
                    // themselves and not a one-loop-old copy.
                    break;

                case (U8)TRANSPORT_OVERDUB:
                {
                    const FLOAT32 fOld = Unpack24(pSlot);

                    Pack24(pSlot, fOld + fIn);
                    pBuf[i] = fIn + fOld;
                    break;
                }

                case (U8)TRANSPORT_PLAY:
                default:
                    pBuf[i] = fIn + Unpack24(pSlot);
                    break;
            }
        }
    }

    nNext = nPos + (U32)nCount;
    if (nNext >= nLen)
    {
        nNext -= nLen;
    }

    aTransport[nChain].nPos = nNext;

    // Hand every plane back and ask for the next window. The transfers do not
    // start until AudioSys_ProcessBlock kicks them, once all chains are done.
    for (p = 0U; p < nWidth; p++)
    {
        LoopMem_Commit((U8)(nBase + p), nNext, nLen, bDirty);
    }

    // A finished first pass of RECORD becomes PLAY automatically, which is what
    // every hardware looper does. The wrap test is valid because the shortest
    // possible loop is one bar - tens of thousands of frames - and a block is 64.
    if ((eState == (U8)TRANSPORT_RECORD) && (nNext < (U32)nCount))
    {
        aTransport[nChain].eState = (U8)TRANSPORT_PLAY;
    }
}

//--------------------------------------------------------------------------------------------------

U32 Looper_RecordedFrames(const U8 nLooper)
{
    if (nLooper >= (U8)LOOPER_QTY)
    {
        return 0UL;
    }

    /* 0 means empty, which the transport turns into a refusal rather than a
       zero-length file. */
    return aLength[nLooper].nFrames;
}

//--------------------------------------------------------------------------------------------------

void Looper_GetTelemetry(PROTO_TELEMETRY* const pTelem)
{
    U8 i;

    for (i = 0U; i < LOOPER_QTY; i++)
    {
        pTelem->aLoopLen[i] = aLength[i].nFrames;
        pTelem->aLoopPos[i] = 0UL;
    }

    for (i = 0U; i < CHAIN_MAX_QTY; i++)
    {
        const U8 nLooper = LooperOfPlane(Grid_Active()->aPlaneBase[i]);

        pTelem->aTransport[i] = aTransport[i].eState;

        // Report the playhead of whichever chain in the pair is running, so the
        // GUI loop indicator has something to follow.
        if (aTransport[i].eState != (U8)TRANSPORT_STOP)
        {
            pTelem->aLoopPos[nLooper] = aTransport[i].nPos;
        }
    }
}

/****************************************** end of file *******************************************/
