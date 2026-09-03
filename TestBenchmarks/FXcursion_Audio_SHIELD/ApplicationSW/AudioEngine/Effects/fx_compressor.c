/**
 * @file      fx_compressor.c
 *
 * @details   Compressor implementation. See fx_compressor.h for the model.
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

#include "fx_compressor.h"

#include "mem_map.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/** dB per octave of amplitude. 20 * log10(2). */
#define COMP_DB_PER_OCT             (6.02059991f)
#define COMP_OCT_PER_DB             (0.16609640f)

/** Floor for the envelope, so the logarithm always has something to work on. */
#define COMP_ENV_FLOOR              (1.0e-9f)

/** Envelope below this cannot be reduced, so the gain computer is skipped. */
#define COMP_HALF_KNEE_DB           (COMP_KNEE_DB * 0.5f)



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

/**
 * @brief Per-plane detector state.
 */
typedef struct stCOMP_STATE
{
    FLOAT32 fEnv;               /**< peak envelope, linear                       */
    FLOAT32 fLastGrDb;          /**< most recent gain reduction, for the meter   */

} COMP_STATE;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * No delay memory at all - a compressor is a few floats per plane. DTCM,
 * because they are touched twice per sample and nothing else reads them.
 */
static COMP_STATE aState[AUDIO_PLANE_QTY] IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static void ResetPlanes(const U8 nPlaneBase, const U8 nWidth)
{
    U8 p;

    for (p = 0U; p < nWidth; p++)
    {
        const U8 nPlane = nPlaneBase + p;

        if (nPlane < AUDIO_PLANE_QTY)
        {
            aState[nPlane].fEnv      = 0.0f;
            aState[nPlane].fLastGrDb = 0.0f;
        }
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief One-pole coefficient for a given time constant.
 *
 * A zero or negative time means "instant", which is what a zero-length attack
 * should do rather than divide by zero.
 */
static FLOAT32 OnePoleCoeff(const FLOAT32 fMs)
{
    FLOAT32 fCoeff = 1.0f;

    if (fMs > 0.0f)
    {
        const FLOAT32 fSamples = fMs * 0.001f * (FLOAT32)AUDIO_SAMPLE_RATE_HZ;

        if (fSamples > 1.0f)
        {
            fCoeff = 1.0f - expf(-1.0f / fSamples);
        }
    }

    return FxUtil_Clamp(fCoeff, 0.0f, 1.0f);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Gain reduction in dB for an envelope already known to be above the knee.
 *
 * @param fEnv        linear peak envelope
 * @param fThreshDb   threshold
 * @param fSlope      (1/ratio) - 1, so always <= 0
 *
 * @return dB to ADD, so zero or negative
 */
static FLOAT32 GainReductionDb(const FLOAT32 fEnv, const FLOAT32 fThreshDb, const FLOAT32 fSlope)
{
    const FLOAT32 fEnvDb = COMP_DB_PER_OCT * log2f(fEnv + COMP_ENV_FLOOR);
    const FLOAT32 fOver  = fEnvDb - fThreshDb;
    FLOAT32       fGrDb;

    if (fOver >= COMP_HALF_KNEE_DB)
    {
        fGrDb = fSlope * fOver;
    }
    else
    {
        // Inside the knee. The quadratic is the standard one: it meets zero and
        // the straight line at the knee edges with matching slope, so the
        // transition into compression has no audible corner.
        const FLOAT32 fKneeIn = fOver + COMP_HALF_KNEE_DB;

        fGrDb = fSlope * (fKneeIn * fKneeIn) / (2.0f * COMP_KNEE_DB);
    }

    return fGrDb;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief The compressor proper, for one or two planes.
 *
 * @param fLink  0 = independent detectors, 1 = one detector on max(|L|, |R|).
 *               Ignored when nWidth is 1.
 */
static void CompressorRun(const FX_CTX* pCtx,
                          FLOAT32* const apPlane[],
                          const U16 nFrames,
                          const FLOAT32 fThreshDb,
                          const FLOAT32 fRatio,
                          const FLOAT32 fAttackMs,
                          const FLOAT32 fReleaseMs,
                          const FLOAT32 fMakeupDb,
                          const FLOAT32 fLink)
{
    const U8      nWidth   = pCtx->nWidth;
    const FLOAT32 fAtt     = OnePoleCoeff(fAttackMs);
    const FLOAT32 fRel     = OnePoleCoeff(fReleaseMs);
    const FLOAT32 fSlope   = (1.0f / FxUtil_Clamp(fRatio, 1.0f, COMP_RATIO_MAX)) - 1.0f;
    const FLOAT32 fMakeup  = exp2f(fMakeupDb * COMP_OCT_PER_DB);

    /*
     * The linear envelope below which no reduction is possible. Comparing
     * against this costs one branch and saves a logarithm and an exponential on
     * every sample that is not being compressed - which, on guitar, is most of
     * them.
     */
    const FLOAT32 fKneeLowLin = exp2f((fThreshDb - COMP_HALF_KNEE_DB) * COMP_OCT_PER_DB);

    COMP_STATE* apSt[CHAIN_MAX_WIDTH];
    FLOAT32     aWorstGr[CHAIN_MAX_WIDTH];
    U8          p;
    U16         i;

    for (p = 0U; p < nWidth; p++)
    {
        apSt[p]     = &aState[pCtx->nPlaneBase + p];
        aWorstGr[p] = 0.0f;
    }

    for (i = 0U; i < nFrames; i++)
    {
        FLOAT32 fLinked = 0.0f;

        if (nWidth == CHAIN_MAX_WIDTH)
        {
            const FLOAT32 fA = fabsf(apPlane[0][i]);
            const FLOAT32 fB = fabsf(apPlane[1][i]);

            fLinked = (fA > fB) ? fA : fB;
        }

        for (p = 0U; p < nWidth; p++)
        {
            const FLOAT32 fAbs = fabsf(apPlane[p][i]);
            FLOAT32       fLevel;
            FLOAT32       fEnv;
            FLOAT32       fGain;

            /* Blend between this plane's own level and the pair's loudest. The
               blend is linear and the linked value is never smaller, so LINK
               only ever adds reduction. */
            fLevel = (nWidth == CHAIN_MAX_WIDTH) ? (fAbs + (fLink * (fLinked - fAbs)))
                                                 : fAbs;

            /* Separate attack and release. Which one applies is decided by the
               instantaneous level against the envelope, not by the gain, so a
               transient starts the attack on the sample it arrives. */
            fEnv = apSt[p]->fEnv;
            fEnv += ((fLevel > fEnv) ? fAtt : fRel) * (fLevel - fEnv);
            apSt[p]->fEnv = fEnv;

            if (fEnv <= fKneeLowLin)
            {
                fGain = fMakeup;                    // no logarithms at all
            }
            else
            {
                const FLOAT32 fGrDb = GainReductionDb(fEnv, fThreshDb, fSlope);

                if (fGrDb < aWorstGr[p])
                {
                    aWorstGr[p] = fGrDb;
                }

                fGain = fMakeup * exp2f(fGrDb * COMP_OCT_PER_DB);
            }

            apPlane[p][i] = apPlane[p][i] * fGain;
        }
    }

    for (p = 0U; p < nWidth; p++)
    {
        apSt[p]->fLastGrDb = aWorstGr[p];
    }
}



/***************************************************************************************************
* Definitions of global (public) functions - MONO
***************************************************************************************************/

void FxCompressorM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxCompressorM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    CompressorRun(pCtx, apPlane, nFrames,
                  FxParam_Lin(&pCtx->pParam[FX_COMPM_P_THRESHOLD],
                              COMP_THRESH_MIN_DB, COMP_THRESH_MAX_DB),
                  FxParam_Exp(&pCtx->pParam[FX_COMPM_P_RATIO],
                              COMP_RATIO_MIN, COMP_RATIO_MAX),
                  FxParam_Exp(&pCtx->pParam[FX_COMPM_P_ATTACK],
                              COMP_ATTACK_MIN_MS, COMP_ATTACK_MAX_MS),
                  FxParam_Exp(&pCtx->pParam[FX_COMPM_P_RELEASE],
                              COMP_RELEASE_MIN_MS, COMP_RELEASE_MAX_MS),
                  FxParam_Lin(&pCtx->pParam[FX_COMPM_P_MAKEUP],
                              0.0f, COMP_MAKEUP_MAX_DB),
                  0.0f);        /* one plane - nothing to link to */
}



/***************************************************************************************************
* Definitions of global (public) functions - STEREO
***************************************************************************************************/

void FxCompressorS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxCompressorS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    CompressorRun(pCtx, apPlane, nFrames,
                  FxParam_Lin(&pCtx->pParam[FX_COMPS_P_THRESHOLD],
                              COMP_THRESH_MIN_DB, COMP_THRESH_MAX_DB),
                  FxParam_Exp(&pCtx->pParam[FX_COMPS_P_RATIO],
                              COMP_RATIO_MIN, COMP_RATIO_MAX),
                  FxParam_Exp(&pCtx->pParam[FX_COMPS_P_ATTACK],
                              COMP_ATTACK_MIN_MS, COMP_ATTACK_MAX_MS),
                  FxParam_Exp(&pCtx->pParam[FX_COMPS_P_RELEASE],
                              COMP_RELEASE_MIN_MS, COMP_RELEASE_MAX_MS),
                  FxParam_Lin(&pCtx->pParam[FX_COMPS_P_MAKEUP],
                              0.0f, COMP_MAKEUP_MAX_DB),
                  FxParam_Norm(&pCtx->pParam[FX_COMPS_P_LINK]));
}



/***************************************************************************************************
* Definitions of global (public) functions - COMMON
***************************************************************************************************/

FLOAT32 FxCompressor_GainReductionDb(const U8 nPlane)
{
    return (nPlane < AUDIO_PLANE_QTY) ? aState[nPlane].fLastGrDb : 0.0f;
}

/****************************************** end of file *******************************************/
