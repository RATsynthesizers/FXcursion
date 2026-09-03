/**
 * @file      hp_bus.c
 *
 * @details   Headphone monitor bus. See hp_bus.h for the model.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "hp_bus.h"

#include "mem_map.h"
#include "Effects/fx_common.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/** Every plane that exists, as a mask. */
#define HP_PLANE_MASK_ALL               ((U8)((1U << AUDIO_PLANE_QTY) - 1U))

/** The bits the headphone converter shifts out. Same 24-bit word as the SAI. */
#define HP_MASK_24                      (0x00FFFFFFUL)

/* HpBus_Process writes slot 0 and slot 1 by name. */
FXC_STATIC_ASSERT(AIO_HP_SLOTS == 2U, hp_bus_is_stereo);



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

static FLOAT32 fMasterTarget IN_DTCM;
static FLOAT32 fMasterCur    IN_DTCM;

static U8 nLeftSources  IN_DTCM;
static U8 nRightSources IN_DTCM;

/* Reserved: per-chain tap point. Stored and reported, not yet acted on. */
static U8 aTapPoint[CHAIN_MAX_QTY] IN_DTCM;

static U32 nClips IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Sum the planes named by a mask, for one frame.
 *
 * A plain sum. The master gain is what keeps it in range - see hp_bus.h.
 */
static S32 SumMask(const S32* const pFrame, const U8 nMask)
{
    S32 nAcc = 0;
    U8  p;

    for (p = 0U; p < AUDIO_PLANE_QTY; p++)
    {
        if ((nMask & (U8)(1U << p)) != 0U)
        {
            nAcc += pFrame[p];
        }
    }

    return nAcc;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Apply the master gain and clamp to the converter's range.
 */
static S32 ToHp(const S32 nAcc, const FLOAT32 fGain, U32* const pClips)
{
    const FLOAT32 fScaled = (FLOAT32)nAcc * fGain;
    S32           nOut;

    // Strictly greater, not greater-or-equal. Both bounds are representable
    // samples, and the default master gain is chosen so that a sum of two
    // full-scale planes lands exactly ON the bound - counting that as a clip
    // would make the meter cry wolf on the one case the default exists to make
    // safe.
    if (fScaled > (FLOAT32)AUDIO_SAMPLE_MAX)
    {
        nOut = AUDIO_SAMPLE_MAX;
        (*pClips)++;
    }
    else if (fScaled < (FLOAT32)AUDIO_SAMPLE_MIN)
    {
        nOut = AUDIO_SAMPLE_MIN;
        (*pClips)++;
    }
    else
    {
        nOut = (S32)fScaled;
    }

    return (S32)((U32)nOut & HP_MASK_24);
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT HpBus_Init(void)
{
    U8 i;

    fMasterTarget = HP_MASTER_DEFAULT;

    // Start AT the target rather than ramping up from silence: the first block
    // after start-up should be a correct monitor mix, not a fade-in.
    fMasterCur    = HP_MASTER_DEFAULT;

    nLeftSources  = (U8)HP_LEFT_PLANE_MASK;
    nRightSources = (U8)HP_RIGHT_PLANE_MASK;

    for (i = 0U; i < CHAIN_MAX_QTY; i++)
    {
        aTapPoint[i] = (U8)HP_TAP_POST_EVERYTHING;
    }

    nClips = 0UL;

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

void HpBus_SetMaster(const FLOAT32 fGain)
{
    // Clamped rather than refused. This is a volume control; a value out of
    // range means the GUI sent nonsense, and going silent would be worse than
    // going to the nearest legal setting.
    fMasterTarget = FxUtil_Clamp(fGain, 0.0f, HP_MASTER_MAX);
}

//--------------------------------------------------------------------------------------------------

FLOAT32 HpBus_Master(void)
{
    return fMasterTarget;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT HpBus_SetSourceMask(const U8 nLeftMask, const U8 nRightMask)
{
    STD_RESULT eResult = RESULT_OK;

    if (((nLeftMask | nRightMask) & (U8)(~HP_PLANE_MASK_ALL)) != 0U)
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else
    {
        nLeftSources  = nLeftMask;
        nRightSources = nRightMask;
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT HpBus_SetTapPoint(const U8 nChain, const U8 eTapPoint)
{
    STD_RESULT eResult;

    if (nChain >= CHAIN_MAX_QTY)
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else if (eTapPoint > (U8)HP_TAP_POST_FX)
    {
        eResult = RESULT_INVALID_PARAM_1;
    }
    else
    {
        // Stored whatever it is, so a preset written by a newer GUI survives a
        // round trip through this firmware unchanged.
        aTapPoint[nChain] = eTapPoint;

        // ...but only one of them actually does anything yet.
        eResult = (eTapPoint == (U8)HP_TAP_POST_EVERYTHING) ? RESULT_OK : RESULT_NOT_OK;
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

U8 HpBus_TapPoint(const U8 nChain)
{
    return (nChain < CHAIN_MAX_QTY) ? aTapPoint[nChain] : (U8)HP_TAP_POST_EVERYTHING;
}

//--------------------------------------------------------------------------------------------------

void HpBus_Process(const S32* const pBlock, S32* const pHp, const U16 nFrames)
{
    // Per block, matching Mixer_Process. A block is 1333 us, so a 20 ms
    // constant still takes about fifteen of them to settle - short enough to
    // feel immediate, long enough not to click.
    const FLOAT32 fSmooth = FxUtil_SmoothCoeff((FLOAT32)MIX_GAIN_SMOOTH_MS, nFrames);
    FLOAT32       fGain;
    U16           i;

    fMasterCur += fSmooth * (fMasterTarget - fMasterCur);
    fGain       = fMasterCur;

    for (i = 0U; i < nFrames; i++)
    {
        const S32* const pFrame = &pBlock[(U32)i * AUDIO_CH_QTY];
        const U32        nOut   = (U32)i * AIO_HP_SLOTS;

        pHp[nOut + 0U] = ToHp(SumMask(pFrame, nLeftSources),  fGain, &nClips);
        pHp[nOut + 1U] = ToHp(SumMask(pFrame, nRightSources), fGain, &nClips);
    }
}

//--------------------------------------------------------------------------------------------------

U32 HpBus_ClipCount(void)
{
    return nClips;
}

/****************************************** end of file *******************************************/
