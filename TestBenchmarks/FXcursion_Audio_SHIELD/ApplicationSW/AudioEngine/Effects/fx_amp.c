/**
 * @file      fx_amp.c
 *
 * @details   Amp, mono and stereo. The simplest pair in the pool, and the one to
 *            read first to see how a mono/stereo split is written.
 *
 *            MONO    Gain.
 *            STEREO  Gain, Pan, Width.
 *
 *            Pan and Width have no meaning on one plane. In the previous design
 *            the mono Amp still carried a Pan parameter that silently did
 *            nothing - which is exactly the reason the pool is split by width
 *            rather than sharing one type with a runtime flag.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - 2.0.0 - mono and stereo split
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_amp.h"

#include "mem_map.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/** 0.0 = silence, 0.5 = unity, 1.0 = +6 dB. */

/** Width: 0 = collapsed to mono, 0.5 = untouched, 1.0 = double the side signal. */

#define AMP_SMOOTH_MS               (15.0f)



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Smoothed gain per plane, shared by both variants.
 *
 * Sharing is safe and costs nothing: a mono Amp and a stereo Amp can never be
 * live on the same plane, because a chain is either mono or stereo. The stereo
 * variant simply uses two adjacent entries. Every effect in this project shares
 * its static memory between variants for the same reason.
 */
static FLOAT32 aCurGain[AUDIO_PLANE_QTY] IN_DTCM;



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
            aCurGain[nPlaneBase + p] = -1.0f;       // negative means "snap"
        }
    }
}

//--------------------------------------------------------------------------------------------------

/** Ramp one plane's gain toward its target and return the settled value. */
static FLOAT32 SmoothGain(const U8 nPlane, const FLOAT32 fTarget, const FLOAT32 fSmooth)
{
    if (aCurGain[nPlane] < 0.0f)
    {
        aCurGain[nPlane] = fTarget;                 // first block after a reset
    }
    else
    {
        aCurGain[nPlane] += fSmooth * (fTarget - aCurGain[nPlane]);
    }

    return aCurGain[nPlane];
}



/***************************************************************************************************
* Definitions of global (public) functions - MONO
***************************************************************************************************/

void FxAmpM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxAmpM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    const FLOAT32 fTarget = FxParam_Lin(&pCtx->pParam[FX_AMPM_P_GAIN], 0.0f, AMP_GAIN_MAX);
    const FLOAT32 fSmooth = FxUtil_SmoothCoeff(AMP_SMOOTH_MS, nFrames);
    const FLOAT32 fGain   = SmoothGain(pCtx->nPlaneBase, fTarget, fSmooth);

    FLOAT32* const pBuf = apPlane[0];
    U16            i;

    for (i = 0U; i < nFrames; i++)
    {
        pBuf[i] *= fGain;
    }
}



/***************************************************************************************************
* Definitions of global (public) functions - STEREO
***************************************************************************************************/

void FxAmpS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxAmpS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    const FLOAT32 fTarget = FxParam_Lin(&pCtx->pParam[FX_AMPS_P_GAIN], 0.0f, AMP_GAIN_MAX);
    const FLOAT32 fPan    = (FxParam_Norm(&pCtx->pParam[FX_AMPS_P_PAN]) * 2.0f) - 1.0f;
    const FLOAT32 fWidth  = FxParam_Lin(&pCtx->pParam[FX_AMPS_P_WIDTH], 0.0f, AMP_WIDTH_MAX);
    const FLOAT32 fSmooth = FxUtil_SmoothCoeff(AMP_SMOOTH_MS, nFrames);

    FLOAT32* const pL = apPlane[0];
    FLOAT32* const pR = apPlane[1];

    FLOAT32 fPanL;
    FLOAT32 fPanR;
    FLOAT32 fGainL;
    FLOAT32 fGainR;
    U16     i;

    FxUtil_PanGains(fPan, &fPanL, &fPanR);

    // Normalise so that centre pan is unity rather than 0.707: this is a channel
    // fader, not a contribution to a mix bus.
    fGainL = SmoothGain(pCtx->nPlaneBase,      fTarget, fSmooth) * fPanL * 1.41421356f;
    fGainR = SmoothGain(pCtx->nPlaneBase + 1U, fTarget, fSmooth) * fPanR * 1.41421356f;

    for (i = 0U; i < nFrames; i++)
    {
        // Mid/side width, then pan and gain. Width 0 collapses the pair to mono,
        // which is a genuinely useful thing to be able to do to a stereo chain
        // and is simply not expressible on a mono one.
        const FLOAT32 fMid  = (pL[i] + pR[i]) * 0.5f;
        const FLOAT32 fSide = ((pL[i] - pR[i]) * 0.5f) * fWidth;

        pL[i] = (fMid + fSide) * fGainL;
        pR[i] = (fMid - fSide) * fGainR;
    }
}

/****************************************** end of file *******************************************/
