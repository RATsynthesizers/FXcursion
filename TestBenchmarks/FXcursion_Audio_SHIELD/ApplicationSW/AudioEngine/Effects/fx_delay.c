/**
 * @file      fx_delay.c
 *
 * @details   Delay, mono and stereo. The memory hog of the machine, and the one
 *            effect that exercises every part of the design.
 *
 *            MONO    Time, Feedback, Tone, Mix.
 *            STEREO  the same, plus Ping-pong and Spread.
 *
 *            Ping-pong is the reason a stereo delay is a different EFFECT and not
 *            a wider instance of the mono one: it routes each plane's feedback
 *            into the other plane's line. There is no sensible mono value for it,
 *            and a "width" flag on one shared type would mean carrying a
 *            parameter that does nothing half the time.
 *
 *            ------------------------------------------------------------------
 *            TWO THINGS THAT ARE EASY TO GET WRONG
 *            ------------------------------------------------------------------
 *
 *            1. DELAY PRECISION. The tap position is kept as a U32 integer part
 *               plus a FLOAT32 fraction, never as a single float. A FLOAT32
 *               holding 192000 frames resolves to about 1/64 of a sample, so a
 *               single-float tap makes a long delay sound gritty when its time is
 *               swept. Keeping the integer part separate makes the fraction exact
 *               at any length.
 *
 *            2. MOVING THE TAP. When the delay time changes, the tap is walked
 *               to its new position sample by sample, at a bounded rate. That
 *               produces the tape-style pitch bend everyone expects from a delay
 *               and, more importantly, never clicks.
 *
 *               The alternative - snapping the tap and crossfading two readers -
 *               gives a clean, pitch-stable change. If you want that behaviour
 *               later, it is a second read pointer and a 10 ms ramp; the state
 *               struct below already has room.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - 2.0.0 - mono and stereo split, ping-pong added
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_delay.h"

#include "mem_map.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/


/** Feedback ceiling. Above this the line grows without bound. */

/** Damping filter corner, at tone = 0.0 and tone = 1.0. */

/** How fast the tap may walk, in samples per sample. Bounds the pitch bend. */
#define DELAY_TAP_SLEW_MAX          (0.25f)

/** Time constant of the delay-time smoother. */
#define DELAY_TIME_SMOOTH_MS        (120.0f)

/** Spread at 1.0 puts the right tap at this multiple of the left one. */

#define FX_TWO_PI                   (6.28318531f)



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

typedef struct stDELAY_STATE
{
    U32     nWritePos;          /**< 0..DELAY_MAX_FRAMES-1                       */

    U32     nTapInt;            /**< current tap, integer frames                 */
    FLOAT32 fTapFrac;           /**< current tap, fractional frames, 0..1        */

    FLOAT32 fSmoothedSec;       /**< delay time after the one-pole smoother      */
    FLOAT32 fDamp;              /**< one-pole state in the feedback path         */

} DELAY_STATE;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * The delay lines. SDRAM bank 1, shared by both variants.
 *
 * Sharing costs nothing: a mono delay and a stereo delay can never be live on
 * the same planes, because a chain is either mono or stereo. A stereo instance
 * at plane base p uses lines p and p+1 - exactly what two mono instances on
 * those planes would have used.
 *
 * One stream per plane, 512 B per block, about 1.5 MB/s in total against roughly
 * 150 MB/s available - which is why these can live in SDRAM while the short
 * modulation lines cannot.
 */
static FLOAT32 aLine[AUDIO_PLANE_QTY][DELAY_MAX_FRAMES] IN_SDRAM_DELAY MEM_ALIGN(32);

static DELAY_STATE aState[AUDIO_PLANE_QTY] IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static void ResetPlanes(const U8 nPlaneBase, const U8 nWidth)
{
    U8 p;

    // Called from the super-loop only. Clearing 750 KiB per plane takes several
    // milliseconds and must never happen inside the audio ISR - see grid.c.
    for (p = 0U; p < nWidth; p++)
    {
        const U8 nPlane = nPlaneBase + p;
        U32      i;

        if (nPlane >= AUDIO_PLANE_QTY)
        {
            break;
        }

        for (i = 0UL; i < DELAY_MAX_FRAMES; i++)
        {
            aLine[nPlane][i] = 0.0f;
        }

        aState[nPlane].nWritePos    = 0UL;
        aState[nPlane].nTapInt      = (U32)(DELAY_TIME_MIN_SEC * (FLOAT32)AUDIO_SAMPLE_RATE_HZ);
        aState[nPlane].fTapFrac     = 0.0f;
        aState[nPlane].fSmoothedSec = 0.0f;     // 0 forces a snap on the first block
        aState[nPlane].fDamp        = 0.0f;
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Advance one plane's tap toward its target and clamp it into the line.
 *
 * @return per-sample step to walk, already slew limited
 */
static FLOAT32 PrepareTap(DELAY_STATE* const pSt,
                          const FLOAT32 fTargetSec,
                          const FLOAT32 fSmooth,
                          const FLOAT32 fRatio,
                          const U16 nFrames)
{
    FLOAT32 fTargetFrames;
    S32     nDeltaInt;
    FLOAT32 fStep;

    // Snap on the very first block after a reset, so the delay does not sweep in
    // from zero when it is added to a chain.
    if (pSt->fSmoothedSec <= 0.0f)
    {
        pSt->fSmoothedSec = fTargetSec;
        pSt->nTapInt      = (U32)((fTargetSec * fRatio) * (FLOAT32)AUDIO_SAMPLE_RATE_HZ);
        pSt->fTapFrac     = 0.0f;
    }
    else
    {
        pSt->fSmoothedSec += fSmooth * (fTargetSec - pSt->fSmoothedSec);
    }

    fTargetFrames = (pSt->fSmoothedSec * fRatio) * (FLOAT32)AUDIO_SAMPLE_RATE_HZ;

    if (fTargetFrames > (FLOAT32)(DELAY_MAX_FRAMES - 2UL))
    {
        fTargetFrames = (FLOAT32)(DELAY_MAX_FRAMES - 2UL);
    }

    // The integer difference is computed in integers so that no precision is lost
    // at long delays.
    nDeltaInt = (S32)((U32)fTargetFrames) - (S32)pSt->nTapInt;
    fStep     = (((FLOAT32)nDeltaInt) + ((fTargetFrames - (FLOAT32)((U32)fTargetFrames))
                                         - pSt->fTapFrac)) / (FLOAT32)nFrames;

    return FxUtil_Clamp(fStep, -DELAY_TAP_SLEW_MAX, DELAY_TAP_SLEW_MAX);
}

//--------------------------------------------------------------------------------------------------

/** Walk the tap by one sample and keep it inside the line. */
static void StepTap(DELAY_STATE* const pSt, const FLOAT32 fStep)
{
    pSt->fTapFrac += fStep;

    while (pSt->fTapFrac >= 1.0f)
    {
        pSt->fTapFrac -= 1.0f;
        pSt->nTapInt++;
    }
    while (pSt->fTapFrac < 0.0f)
    {
        pSt->fTapFrac += 1.0f;
        if (pSt->nTapInt > 0UL)
        {
            pSt->nTapInt--;
        }
    }

    // Never read what we are about to write, and never read past the end.
    if (pSt->nTapInt < 1UL)
    {
        pSt->nTapInt = 1UL;
    }
    else if (pSt->nTapInt > (DELAY_MAX_FRAMES - 2UL))
    {
        pSt->nTapInt = DELAY_MAX_FRAMES - 2UL;
    }
    else
    {
        do_nothing();
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief The shared engine. Mono calls it with nWidth 1 and no crossfeed.
 *
 * @param fCross   0 = each plane feeds itself, 1 = full ping-pong
 * @param fRatioR  right tap as a multiple of the left one (Spread)
 */
static void DelayRun(const FX_CTX* pCtx,
                     FLOAT32* const apPlane[],
                     const U16 nFrames,
                     const FLOAT32 fTargetSec,
                     const FLOAT32 fFeedback,
                     const FLOAT32 fToneHz,
                     const FLOAT32 fMix,
                     const FLOAT32 fCross,
                     const FLOAT32 fRatioR)
{
    const U8      nWidth     = pCtx->nWidth;
    const FLOAT32 fSmooth    = FxUtil_SmoothCoeff(DELAY_TIME_SMOOTH_MS, nFrames);
    const FLOAT32 fDampCoeff = FxUtil_Clamp(1.0f - expf(-FX_TWO_PI * fToneHz /
                                                        (FLOAT32)AUDIO_SAMPLE_RATE_HZ),
                                            0.0f, 1.0f);
    const FLOAT32 fDryGain   = 1.0f - fMix;
    const FLOAT32 fDirect    = 1.0f - fCross;

    DELAY_STATE* apSt[CHAIN_MAX_WIDTH];
    FLOAT32*     apLine[CHAIN_MAX_WIDTH];
    FLOAT32      aStep[CHAIN_MAX_WIDTH];
    U8           p;
    U16          i;

    for (p = 0U; p < nWidth; p++)
    {
        const U8 nPlane = pCtx->nPlaneBase + p;

        apSt[p]   = &aState[nPlane];
        apLine[p] = aLine[nPlane];
        aStep[p]  = PrepareTap(apSt[p], fTargetSec, fSmooth,
                               (p == 0U) ? 1.0f : fRatioR, nFrames);
    }

    for (i = 0U; i < nFrames; i++)
    {
        FLOAT32 aWet[CHAIN_MAX_WIDTH];

        // Read every plane BEFORE writing any of them: with ping-pong on, each
        // line's input depends on the other line's output, so the two must be
        // sampled from the same instant.
        for (p = 0U; p < nWidth; p++)
        {
            StepTap(apSt[p], aStep[p]);

            aWet[p] = FxUtil_DelayRead(apLine[p], DELAY_MAX_FRAMES,
                                       apSt[p]->nWritePos,
                                       apSt[p]->nTapInt, apSt[p]->fTapFrac);

            // Damping in the feedback path: each repeat is a little darker,
            // which is what makes a digital delay sound musical rather than
            // metallic.
            apSt[p]->fDamp += fDampCoeff * (aWet[p] - apSt[p]->fDamp);
        }

        for (p = 0U; p < nWidth; p++)
        {
            const U8      nOther = (nWidth == 2U) ? (U8)(1U - p) : p;
            const FLOAT32 fRegen = (apSt[p]->fDamp * fDirect)
                                 + (apSt[nOther]->fDamp * fCross);

            // Soft clip the REGENERATION only, never the incoming signal.
            // Saturating (fIn + feedback) together would colour the dry signal
            // on its way into the line even at zero feedback - the cubic curve
            // is not transparent, it merely has unity slope at the origin.
            apLine[p][apSt[p]->nWritePos] = apPlane[p][i]
                                          + FxUtil_SoftClip(fRegen * fFeedback);

            apSt[p]->nWritePos++;
            if (apSt[p]->nWritePos >= DELAY_MAX_FRAMES)
            {
                apSt[p]->nWritePos = 0UL;
            }

            apPlane[p][i] = (apPlane[p][i] * fDryGain) + (aWet[p] * fMix);
        }
    }
}



/***************************************************************************************************
* Definitions of global (public) functions - MONO
***************************************************************************************************/

void FxDelayM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxDelayM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    DelayRun(pCtx, apPlane, nFrames,
             FxParam_TimeSec(&pCtx->pParam[FX_DELAYM_P_TIME], pCtx->pTempo,
                             DELAY_TIME_MIN_SEC, DELAY_TIME_MAX_SEC),
             FxParam_Lin(&pCtx->pParam[FX_DELAYM_P_FEEDBACK], 0.0f, DELAY_FEEDBACK_MAX),
             FxParam_Exp(&pCtx->pParam[FX_DELAYM_P_TONE],
                         DELAY_TONE_MIN_HZ, DELAY_TONE_MAX_HZ),
             FxParam_Norm(&pCtx->pParam[FX_DELAYM_P_MIX]),
             0.0f,          /* no crossfeed - there is nothing to cross to */
             1.0f);         /* no spread                                   */
}



/***************************************************************************************************
* Definitions of global (public) functions - STEREO
***************************************************************************************************/

void FxDelayS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxDelayS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    const FLOAT32 fSpread = FxParam_Norm(&pCtx->pParam[FX_DELAYS_P_SPREAD]);

    DelayRun(pCtx, apPlane, nFrames,
             FxParam_TimeSec(&pCtx->pParam[FX_DELAYS_P_TIME], pCtx->pTempo,
                             DELAY_TIME_MIN_SEC, DELAY_TIME_MAX_SEC),
             FxParam_Lin(&pCtx->pParam[FX_DELAYS_P_FEEDBACK], 0.0f, DELAY_FEEDBACK_MAX),
             FxParam_Exp(&pCtx->pParam[FX_DELAYS_P_TONE],
                         DELAY_TONE_MIN_HZ, DELAY_TONE_MAX_HZ),
             FxParam_Norm(&pCtx->pParam[FX_DELAYS_P_MIX]),
             FxParam_Norm(&pCtx->pParam[FX_DELAYS_P_PINGPONG]),
             1.0f + (fSpread * (DELAY_SPREAD_MAX_RATIO - 1.0f)));
}

/****************************************** end of file *******************************************/
