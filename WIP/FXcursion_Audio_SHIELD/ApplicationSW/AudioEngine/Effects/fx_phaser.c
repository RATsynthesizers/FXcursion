/**
 * @file      fx_phaser.c
 *
 * @details   Phaser implementation. See fx_phaser.h for the model.
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

#include "fx_phaser.h"

#include "mem_map.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

#define PHASER_TWO_PI               (6.28318531f)
#define PHASER_PI                   (3.14159265f)



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

/**
 * @brief One plane's cascade, plus the instance's LFO phase in the base plane.
 */
typedef struct stPHASER_STATE
{
    FLOAT32 afZ[PHASER_STAGE_QTY];      /**< one state per allpass stage         */
    FLOAT32 fFeedback;                  /**< last output, for regeneration       */
    FLOAT32 fPhase;                     /**< base plane only                     */

} PHASER_STATE;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

static PHASER_STATE aState[AUDIO_PLANE_QTY] IN_DTCM;

/** Per-stage corner multipliers - see PHASER_STAGE_SPREAD in the header. */
static const FLOAT32 afStageSpread[PHASER_STAGE_QTY] = PHASER_STAGE_SPREAD;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief First-order allpass coefficient for a given corner frequency.
 *
 * a = (1 - tan(pi*f/fs)) / (1 + tan(pi*f/fs)). Evaluated twice per stage per
 * block - for the two ends of the sweep - and interpolated in between, rather
 * than a tangent per sample.
 */
static FLOAT32 CoeffForHz(const FLOAT32 fHz)
{
    const FLOAT32 fClamped = FxUtil_Clamp(fHz, 20.0f,
                                          (FLOAT32)AUDIO_SAMPLE_RATE_HZ * 0.45f);
    const FLOAT32 fT       = tanf(PHASER_PI * fClamped / (FLOAT32)AUDIO_SAMPLE_RATE_HZ);

    return (1.0f - fT) / (1.0f + fT);
}

//--------------------------------------------------------------------------------------------------

static void ResetPlanes(const U8 nPlaneBase, const U8 nWidth)
{
    U8 p;

    for (p = 0U; p < nWidth; p++)
    {
        const U8 nPlane = nPlaneBase + p;

        if (nPlane < AUDIO_PLANE_QTY)
        {
            U8 k;

            for (k = 0U; k < PHASER_STAGE_QTY; k++)
            {
                aState[nPlane].afZ[k] = 0.0f;
            }

            aState[nPlane].fFeedback = 0.0f;
            aState[nPlane].fPhase    = 0.0f;
        }
    }
}

//--------------------------------------------------------------------------------------------------

static void PhaserRun(const FX_CTX* pCtx,
                      FLOAT32* const apPlane[],
                      const U16 nFrames,
                      const FLOAT32 fRateHz,
                      const FLOAT32 fDepth,
                      const FLOAT32 fFeedbackAmt,
                      const U8 nStages,
                      const FLOAT32 fMix,
                      const FLOAT32 fSpread)
{
    const U8      nWidth   = pCtx->nWidth;
    const U8      nBase    = pCtx->nPlaneBase;
    const FLOAT32 fInc     = fRateHz / (FLOAT32)AUDIO_SAMPLE_RATE_HZ;
    const FLOAT32 fDryGain = 1.0f - fMix;

    /*
     * The two ends of the sweep, per stage.
     *
     * Depth widens the sweep UPWARDS from the bottom, so a small depth parks the
     * notches low rather than stranding them in the treble - which is what the
     * previous version did, and why it sounded like another comb filter.
     *
     * Each stage's corner is multiplied by its own factor, so the notches are
     * spaced irregularly. A cascade of identical allpasses gives evenly related
     * notches, which is a flanger by another name.
     */
    const FLOAT32 fTopHz = PHASER_FC_MIN_HZ *
                           powf(PHASER_FC_MAX_HZ / PHASER_FC_MIN_HZ, fDepth);

    FLOAT32 afALow[PHASER_STAGE_QTY];
    FLOAT32 afAHigh[PHASER_STAGE_QTY];
    FLOAT32 fPhase = aState[nBase].fPhase;
    U8      p;
    U8      k;
    U16     i;

    for (k = 0U; k < nStages; k++)
    {
        afALow[k]  = CoeffForHz(PHASER_FC_MIN_HZ * afStageSpread[k]);
        afAHigh[k] = CoeffForHz(fTopHz * afStageSpread[k]);
    }

    for (i = 0U; i < nFrames; i++)
    {
        for (p = 0U; p < nWidth; p++)
        {
            PHASER_STATE* const pSt = &aState[nBase + p];

            const FLOAT32 fPh  = fPhase + ((FLOAT32)p * fSpread * 0.5f);
            const FLOAT32 fLfo = 0.5f + (0.5f * sinf(PHASER_TWO_PI * fPh));

            const FLOAT32 fIn = apPlane[p][i];
            FLOAT32       fX  = fIn + FxUtil_SoftClip(pSt->fFeedback * fFeedbackAmt);

            for (k = 0U; k < nStages; k++)
            {
                /* Each stage sweeps between its OWN two corners. */
                const FLOAT32 fCoeff = afALow[k] + ((afAHigh[k] - afALow[k]) * fLfo);

                /* First-order allpass, one state per stage:
                       y = -a*x + z
                       z =  x + a*y
                   Unity magnitude at every frequency - it only moves phase. */
                const FLOAT32 fY = (-fCoeff * fX) + pSt->afZ[k];

                pSt->afZ[k] = fX + (fCoeff * fY);
                fX          = fY;
            }

            pSt->fFeedback = fX;

            /* The notches come from the SUM of dry and phase-shifted: where the
               two are half a cycle apart they cancel. */
            apPlane[p][i] = (fIn * fDryGain) + (fX * fMix);
        }

        fPhase += fInc;
        if (fPhase >= 1.0f)
        {
            fPhase -= 1.0f;
        }
    }

    aState[nBase].fPhase = fPhase;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Quantise the STAGES knob to 2, 4, 6 or 8.
 *
 * FX_PF_STEPPED says the effect reads this as a small integer, and it means it:
 * each pair of stages adds one notch, so anything between two settings would
 * sound like one of them anyway.
 */
static U8 StageCount(const FX_PARAM* const pParam)
{
    const FLOAT32 fNorm = FxParam_Norm(pParam);
    U32           nStep = (U32)((fNorm * (FLOAT32)PHASER_STAGE_STEPS) + 0.0001f);

    if (nStep >= PHASER_STAGE_STEPS)
    {
        nStep = PHASER_STAGE_STEPS - 1UL;
    }

    return (U8)((nStep + 1UL) * 2UL);
}



/***************************************************************************************************
* Definitions of global (public) functions - MONO
***************************************************************************************************/

void FxPhaserM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxPhaserM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    PhaserRun(pCtx, apPlane, nFrames,
              FxParam_RateHz(&pCtx->pParam[FX_PHASERM_P_RATE], pCtx->pTempo,
                             PHASER_RATE_MIN_HZ, PHASER_RATE_MAX_HZ),
              FxParam_Norm(&pCtx->pParam[FX_PHASERM_P_DEPTH]),
              FxParam_Lin(&pCtx->pParam[FX_PHASERM_P_FEEDBACK], 0.0f, PHASER_FEEDBACK_MAX),
              StageCount(&pCtx->pParam[FX_PHASERM_P_STAGES]),
              FxParam_Norm(&pCtx->pParam[FX_PHASERM_P_MIX]),
              0.0f);
}



/***************************************************************************************************
* Definitions of global (public) functions - STEREO
***************************************************************************************************/

void FxPhaserS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxPhaserS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    PhaserRun(pCtx, apPlane, nFrames,
              FxParam_RateHz(&pCtx->pParam[FX_PHASERS_P_RATE], pCtx->pTempo,
                             PHASER_RATE_MIN_HZ, PHASER_RATE_MAX_HZ),
              FxParam_Norm(&pCtx->pParam[FX_PHASERS_P_DEPTH]),
              FxParam_Lin(&pCtx->pParam[FX_PHASERS_P_FEEDBACK], 0.0f, PHASER_FEEDBACK_MAX),
              StageCount(&pCtx->pParam[FX_PHASERS_P_STAGES]),
              FxParam_Norm(&pCtx->pParam[FX_PHASERS_P_MIX]),
              FxParam_Norm(&pCtx->pParam[FX_PHASERS_P_SPREAD]));
}

/****************************************** end of file *******************************************/
