/**
 * @file      fx_tremolo.c
 *
 * @details   Amplitude modulation with a tempo-syncable LFO, mono and stereo.
 *
 *            MONO    Rate, Depth, Shape.
 *            STEREO  the same, plus Phase - the offset between the two planes'
 *                    LFOs. At 0 the pair pulses together; at 180 degrees the
 *                    same effect IS an auto-panner. That is the clearest example
 *                    in the pool of a parameter that has no mono meaning at all.
 *
 *            Also the worked example of a SYNCED parameter: the rate comes from
 *            FxParam_RateHz, which returns either an exponentially mapped free
 *            rate or the reciprocal of a note division resolved against the
 *            current tempo. The effect does not care which, and neither does
 *            the GUI.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - 2.0.0 - mono and stereo split, Phase added
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_tremolo.h"

#include "mem_map.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/


#define FX_TWO_PI                   (6.28318531f)



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/** LFO phase, 0..1, per plane. Shared by both variants - see fx_amp.c. */
static FLOAT32 aPhase[AUDIO_PLANE_QTY] IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static void ResetPlanes(const U8 nPlaneBase, const U8 nWidth)
{
    U8 p;

    for (p = 0U; p < nWidth; p++)
    {
        if ((nPlaneBase + p) < AUDIO_PLANE_QTY)
        {
            aPhase[nPlaneBase + p] = 0.0f;
        }
    }
}

//--------------------------------------------------------------------------------------------------

/** Modulate one plane. fPhaseOffset shifts its LFO relative to the chain's. */
static void ModulatePlane(FLOAT32* const pBuf,
                          const U16 nFrames,
                          const U8 nPlane,
                          const FLOAT32 fInc,
                          const FLOAT32 fDepth,
                          const FLOAT32 fShape,
                          const FLOAT32 fPhaseOffset)
{
    FLOAT32 fPhase = aPhase[nPlane];
    U16     i;

    for (i = 0U; i < nFrames; i++)
    {
        FLOAT32 fLfoPhase = fPhase + fPhaseOffset;

        if (fLfoPhase >= 1.0f)
        {
            fLfoPhase -= 1.0f;
        }

        {
            // Shape morphs sine (0.0) toward square (1.0).
            const FLOAT32 fSine   = sinf(fLfoPhase * FX_TWO_PI);
            const FLOAT32 fSquare = (fSine >= 0.0f) ? 1.0f : -1.0f;
            const FLOAT32 fLfo    = fSine + (fShape * (fSquare - fSine));

            // Map -1..+1 onto a gain of (1 - depth) .. 1.
            pBuf[i] *= 1.0f - (fDepth * 0.5f * (1.0f - fLfo));
        }

        fPhase += fInc;
        if (fPhase >= 1.0f)
        {
            fPhase -= 1.0f;
        }
    }

    aPhase[nPlane] = fPhase;
}



/***************************************************************************************************
* Definitions of global (public) functions - MONO
***************************************************************************************************/

void FxTremoloM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxTremoloM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    const FLOAT32 fRateHz = FxParam_RateHz(&pCtx->pParam[FX_TREMOLOM_P_RATE],
                                           pCtx->pTempo,
                                           TREM_RATE_MIN_HZ, TREM_RATE_MAX_HZ);

    ModulatePlane(apPlane[0], nFrames, pCtx->nPlaneBase,
                  fRateHz / (FLOAT32)AUDIO_SAMPLE_RATE_HZ,
                  FxParam_Norm(&pCtx->pParam[FX_TREMOLOM_P_DEPTH]),
                  FxParam_Norm(&pCtx->pParam[FX_TREMOLOM_P_SHAPE]),
                  0.0f);
}



/***************************************************************************************************
* Definitions of global (public) functions - STEREO
***************************************************************************************************/

void FxTremoloS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxTremoloS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    const FLOAT32 fRateHz = FxParam_RateHz(&pCtx->pParam[FX_TREMOLOS_P_RATE],
                                           pCtx->pTempo,
                                           TREM_RATE_MIN_HZ, TREM_RATE_MAX_HZ);

    const FLOAT32 fInc    = fRateHz / (FLOAT32)AUDIO_SAMPLE_RATE_HZ;
    const FLOAT32 fDepth  = FxParam_Norm(&pCtx->pParam[FX_TREMOLOS_P_DEPTH]);
    const FLOAT32 fShape  = FxParam_Norm(&pCtx->pParam[FX_TREMOLOS_P_SHAPE]);

    // Phase 0..1 maps to 0..180 degrees of offset. At 1.0 the right plane is in
    // antiphase with the left and the effect becomes an auto-panner.
    const FLOAT32 fOffset = FxParam_Norm(&pCtx->pParam[FX_TREMOLOS_P_PHASE]) * 0.5f;

    ModulatePlane(apPlane[0], nFrames, pCtx->nPlaneBase,      fInc, fDepth, fShape, 0.0f);
    ModulatePlane(apPlane[1], nFrames, pCtx->nPlaneBase + 1U, fInc, fDepth, fShape, fOffset);
}

/****************************************** end of file *******************************************/
