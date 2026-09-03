/**
 * @file      fx_common.c
 *
 * @details   Parameter mapping, tempo resolution and the small DSP helpers
 *            shared by every effect.
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

#include "fx_common.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

#define FX_PI                       (3.14159265358979f)

/** Below this the tempo is treated as invalid and sync falls back to free mode. */
#define TEMPO_BPM_MIN               (20.0f)
#define TEMPO_BPM_MAX               (400.0f)



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

FLOAT32 FxParam_Norm(const FX_PARAM* const pParam)
{
    return FxUtil_Clamp(pParam->fValue, 0.0f, 1.0f);
}

//--------------------------------------------------------------------------------------------------

FLOAT32 FxParam_Lin(const FX_PARAM* const pParam, const FLOAT32 fMin, const FLOAT32 fMax)
{
    return fMin + (FxParam_Norm(pParam) * (fMax - fMin));
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Exponential map of an already-normalised value.
 */
static FLOAT32 ExpMap(const FLOAT32 fNorm, const FLOAT32 fMin, const FLOAT32 fMax)
{
    FLOAT32 fResult;

    if ((fMin <= 0.0f) || (fMax <= fMin))
    {
        // Caller error, or a range that collapsed after clamping: fall back to
        // a linear map rather than producing NaN.
        fResult = fMin + (fNorm * (fMax - fMin));
    }
    else
    {
        fResult = fMin * powf(fMax / fMin, fNorm);
    }

    return fResult;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief The shortest and longest periods the note divisions can express at the
 *        current tempo.
 *
 * This is what makes a syncable parameter's FREE range agree with its SYNCED
 * range. Without it the two modes cover different spans - a tremolo knob at
 * zero gave 0.10 Hz free but 0.5 Hz on 1/1 - and flipping the sync switch
 * jumped the value.
 *
 * The cost is deliberate and worth stating: the free range now MOVES WITH THE
 * TEMPO. A free-running LFO is fastest at fast tempos, and a free delay cannot
 * reach four seconds unless the tempo is slow enough for a whole note to be
 * that long. The caller's own limits still apply on top, so nothing can ask for
 * more delay line than exists.
 */
static void TempoSpanSec(const TEMPO* const pTempo,
                         FLOAT32* const pShortest,
                         FLOAT32* const pLongest)
{
    const FLOAT32 fQuarter = Tempo_QuarterSec(pTempo);

    *pShortest = g_aDivQuarters[DIV_1_32] * fQuarter;
    *pLongest  = g_aDivQuarters[DIV_1_1]  * fQuarter;
}

//--------------------------------------------------------------------------------------------------

FLOAT32 FxParam_Exp(const FX_PARAM* const pParam, const FLOAT32 fMin, const FLOAT32 fMax)
{
    return ExpMap(FxParam_Norm(pParam), fMin, fMax);
}

//--------------------------------------------------------------------------------------------------

FLOAT32 Tempo_QuarterSec(const TEMPO* const pTempo)
{
    const FLOAT32 fBpm = FxUtil_Clamp(pTempo->fBpm, TEMPO_BPM_MIN, TEMPO_BPM_MAX);

    return 60.0f / fBpm;
}

//--------------------------------------------------------------------------------------------------

FLOAT32 Tempo_BarSec(const TEMPO* const pTempo)
{
    FLOAT32 fQuartersPerBar;

    // Bar length in quarter notes = beats * (4 / beatUnit).
    // 4/4 -> 4 * 1.0 = 4 quarters.  6/8 -> 6 * 0.5 = 3 quarters.
    if ((pTempo->nBeatsPerBar == 0U) || (pTempo->nBeatUnit == 0U))
    {
        fQuartersPerBar = 4.0f;             // degrade to 4/4 rather than divide by zero
    }
    else
    {
        fQuartersPerBar = ((FLOAT32)pTempo->nBeatsPerBar * 4.0f) / (FLOAT32)pTempo->nBeatUnit;
    }

    return fQuartersPerBar * Tempo_QuarterSec(pTempo);
}

//--------------------------------------------------------------------------------------------------

FLOAT32 FxParam_TimeSec(const FX_PARAM* const pParam,
                        const TEMPO* const pTempo,
                        const FLOAT32 fMinSec,
                        const FLOAT32 fMaxSec)
{
    FLOAT32 fSec;

    if ((pParam->bSync != FALSE) && (pParam->eDivision < (U8)DIV_QTY) && (pTempo != NULL_PTR))
    {
        fSec = g_aDivQuarters[pParam->eDivision] * Tempo_QuarterSec(pTempo);
    }
    else if (pTempo != NULL_PTR)
    {
        // Free, but spanning exactly what the divisions span at this tempo, so
        // the ends of the knob ARE 1/32 and 1/1 and the sync switch does not
        // jump the value. See TempoSpanSec.
        FLOAT32 fShortest;
        FLOAT32 fLongest;

        TempoSpanSec(pTempo, &fShortest, &fLongest);

        fSec = ExpMap(FxParam_Norm(pParam),
                      FxUtil_Clamp(fShortest, fMinSec, fMaxSec),
                      FxUtil_Clamp(fLongest,  fMinSec, fMaxSec));
    }
    else
    {
        // No tempo to follow - the effect's own range is all there is.
        fSec = FxParam_Exp(pParam, fMinSec, fMaxSec);
    }

    // A synced division can easily fall outside what the effect can store, e.g.
    // a whole note at 40 BPM is 6 s but the delay line is 4 s. Clamp rather than
    // read outside the buffer.
    return FxUtil_Clamp(fSec, fMinSec, fMaxSec);
}

//--------------------------------------------------------------------------------------------------

FLOAT32 FxParam_RateHz(const FX_PARAM* const pParam,
                       const TEMPO* const pTempo,
                       const FLOAT32 fMinHz,
                       const FLOAT32 fMaxHz)
{
    FLOAT32 fHz;

    if ((pParam->bSync != FALSE) && (pParam->eDivision < (U8)DIV_QTY) && (pTempo != NULL_PTR))
    {
        const FLOAT32 fPeriodSec = g_aDivQuarters[pParam->eDivision] * Tempo_QuarterSec(pTempo);

        fHz = (fPeriodSec > 0.0f) ? (1.0f / fPeriodSec) : fMinHz;
    }
    else if (pTempo != NULL_PTR)
    {
        // Same span as the divisions cover, so free and synced agree at the
        // ends of the knob. A long division is a SLOW rate, so the longest
        // period gives the low end here.
        FLOAT32 fShortest;
        FLOAT32 fLongest;

        TempoSpanSec(pTempo, &fShortest, &fLongest);

        if ((fShortest > 0.0f) && (fLongest > 0.0f))
        {
            fHz = ExpMap(FxParam_Norm(pParam),
                         FxUtil_Clamp(1.0f / fLongest,  fMinHz, fMaxHz),
                         FxUtil_Clamp(1.0f / fShortest, fMinHz, fMaxHz));
        }
        else
        {
            fHz = FxParam_Exp(pParam, fMinHz, fMaxHz);
        }
    }
    else
    {
        fHz = FxParam_Exp(pParam, fMinHz, fMaxHz);
    }

    return FxUtil_Clamp(fHz, fMinHz, fMaxHz);
}

//--------------------------------------------------------------------------------------------------

FLOAT32 FxUtil_Clamp(const FLOAT32 fValue, const FLOAT32 fMin, const FLOAT32 fMax)
{
    FLOAT32 fResult = fValue;

    if (fResult > fMax)
    {
        fResult = fMax;
    }
    else if (fResult < fMin)
    {
        fResult = fMin;
    }
    else
    {
        do_nothing();
    }

    return fResult;
}

//--------------------------------------------------------------------------------------------------

FLOAT32 FxUtil_SmoothCoeff(const FLOAT32 fTimeConstMs, const U16 nFrames)
{
    FLOAT32 fCoeff;

    if (fTimeConstMs <= 0.0f)
    {
        fCoeff = 1.0f;                      // no smoothing
    }
    else
    {
        const FLOAT32 fBlockSec = (FLOAT32)nFrames / (FLOAT32)AUDIO_SAMPLE_RATE_HZ;
        const FLOAT32 fTauSec   = fTimeConstMs * 0.001f;

        fCoeff = 1.0f - expf(-fBlockSec / fTauSec);
    }

    return FxUtil_Clamp(fCoeff, 0.0f, 1.0f);
}

//--------------------------------------------------------------------------------------------------

FLOAT32 FxUtil_SoftClip(const FLOAT32 fValue)
{
    // Cubic soft clip. Input is pre-scaled so that the small-signal gain is
    // exactly 1.0 and saturation begins at |input| = 1.5, which leaves usable
    // headroom above nominal full scale.
    const FLOAT32 t = fValue * (1.0f / 1.5f);
    FLOAT32       fResult;

    if (t >= 1.0f)
    {
        fResult = 1.0f;
    }
    else if (t <= -1.0f)
    {
        fResult = -1.0f;
    }
    else
    {
        fResult = 1.5f * (t - ((t * t * t) * (1.0f / 3.0f)));
    }

    return fResult;
}

//--------------------------------------------------------------------------------------------------

void FxUtil_PanGains(const FLOAT32 fPan, FLOAT32* const pLeft, FLOAT32* const pRight)
{
    // Constant power: centre gives 0.707 on both sides, so sweeping the pan does
    // not change perceived loudness.
    const FLOAT32 fAngle = (FxUtil_Clamp(fPan, -1.0f, 1.0f) + 1.0f) * (FX_PI * 0.25f);

    *pLeft  = cosf(fAngle);
    *pRight = sinf(fAngle);
}

//--------------------------------------------------------------------------------------------------

FLOAT32 FxUtil_DelayRead(const FLOAT32* const pLine,
                         const U32 nLineLen,
                         const U32 nWritePos,
                         const U32 nIntDelay,
                         const FLOAT32 fFrac)
{
    U32     nRead0;
    U32     nRead1;
    FLOAT32 fFraction;

    // Integer arithmetic on the index, so precision does not depend on the
    // delay length. See the note in fx_common.h.
    nRead0 = (nWritePos + nLineLen) - (nIntDelay % nLineLen);
    if (nRead0 >= nLineLen)
    {
        nRead0 -= nLineLen;
    }

    nRead1 = (nRead0 == 0U) ? (nLineLen - 1U) : (nRead0 - 1U);

    fFraction = FxUtil_Clamp(fFrac, 0.0f, 1.0f);

    // nRead1 is one sample FURTHER back in time than nRead0, so a growing
    // fraction moves the tap away from the write head.
    return pLine[nRead0] + (fFraction * (pLine[nRead1] - pLine[nRead0]));
}

/****************************************** end of file *******************************************/
