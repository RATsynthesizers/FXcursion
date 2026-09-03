/**
 * @file      fx_distortion.c
 *
 * @details   Distortion implementation. See fx_distortion.h for the model.
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

#include "fx_distortion.h"

#include "mem_map.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

#define DIST_TWO_PI                 (6.28318531f)

/** Oversampling factor. */
#define DIST_OS                     (2U)

/** Sample rate the clipper actually runs at. */
#define DIST_OS_RATE                ((FLOAT32)AUDIO_SAMPLE_RATE_HZ * (FLOAT32)DIST_OS)

/**
 * The two Q values that make a pair of cascaded biquads a fourth-order
 * Butterworth. Not arbitrary: any other pair gives ripple or a soft knee.
 */
#define DIST_Q_A                    (0.54119610f)
#define DIST_Q_B                    (1.30656296f)



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

typedef struct stBIQUAD
{
    FLOAT32 fX1;
    FLOAT32 fX2;
    FLOAT32 fY1;
    FLOAT32 fY2;

} BIQUAD;

typedef struct stBIQUAD_COEF
{
    FLOAT32 fB0;
    FLOAT32 fB1;
    FLOAT32 fB2;
    FLOAT32 fA1;
    FLOAT32 fA2;

} BIQUAD_COEF;

typedef struct stDIST_STATE
{
    BIQUAD  atUp[2];            /**< band limits the zero-stuffed signal         */
    BIQUAD  atDown[2];          /**< band limits before throwing samples away    */
    FLOAT32 fTone;              /**< one-pole after the clipper                  */

} DIST_STATE;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

static DIST_STATE aState[AUDIO_PLANE_QTY] IN_DTCM;



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
            U8 k;

            for (k = 0U; k < 2U; k++)
            {
                aState[nPlane].atUp[k].fX1   = 0.0f;
                aState[nPlane].atUp[k].fX2   = 0.0f;
                aState[nPlane].atUp[k].fY1   = 0.0f;
                aState[nPlane].atUp[k].fY2   = 0.0f;
                aState[nPlane].atDown[k].fX1 = 0.0f;
                aState[nPlane].atDown[k].fX2 = 0.0f;
                aState[nPlane].atDown[k].fY1 = 0.0f;
                aState[nPlane].atDown[k].fY2 = 0.0f;
            }

            aState[nPlane].fTone = 0.0f;
        }
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Robert Bristow-Johnson lowpass, normalised.
 */
static void MakeLowpass(BIQUAD_COEF* const pC,
                        const FLOAT32 fCutoffHz,
                        const FLOAT32 fSampleHz,
                        const FLOAT32 fQ)
{
    const FLOAT32 fW0    = DIST_TWO_PI * FxUtil_Clamp(fCutoffHz, 20.0f, fSampleHz * 0.45f) /
                           fSampleHz;
    const FLOAT32 fCos   = cosf(fW0);
    const FLOAT32 fAlpha = sinf(fW0) / (2.0f * fQ);

    const FLOAT32 fA0 = 1.0f + fAlpha;
    const FLOAT32 fB  = (1.0f - fCos) * 0.5f;

    pC->fB0 = fB / fA0;
    pC->fB1 = (1.0f - fCos) / fA0;
    pC->fB2 = pC->fB0;
    pC->fA1 = (-2.0f * fCos) / fA0;
    pC->fA2 = (1.0f - fAlpha) / fA0;
}

//--------------------------------------------------------------------------------------------------

static FLOAT32 BiquadRun(BIQUAD* const pS, const BIQUAD_COEF* const pC, const FLOAT32 fIn)
{
    const FLOAT32 fOut = (pC->fB0 * fIn) + (pC->fB1 * pS->fX1) + (pC->fB2 * pS->fX2)
                       - (pC->fA1 * pS->fY1) - (pC->fA2 * pS->fY2);

    pS->fX2 = pS->fX1;
    pS->fX1 = fIn;
    pS->fY2 = pS->fY1;
    pS->fY1 = fOut;

    return fOut;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief The clipper: flat topped, but reaching that top with a defined slope.
 *
 * NOT a literal corner, and the reason is measured rather than aesthetic.
 *
 * A true hard clip generates harmonics that fall off only as 1/n, so there is
 * always significant energy above ANY sample rate you choose. Driving a 7 kHz
 * tone through one at 96 kHz put its thirteenth harmonic at 91 kHz, which folds
 * to 5 kHz, and its fifteenth at 105 kHz, which folds to 9 kHz - both inside
 * the audible band, where no anti-aliasing filter can reach them. Measured at
 * -22 and -25 dB below the note. Plainly audible, and no amount of extra
 * oversampling removes them: doubling the rate again just moves which harmonic
 * lands where.
 *
 * This curve is continuous in its first derivative, so its harmonics fall off
 * as 1/n^2 instead. Combined with running at twice the rate, that is what puts
 * the folded content below hearing.
 *
 * It is still much harder than the overdrive: the drive range reaches a hundred
 * rather than thirty, so the signal spends nearly all of its time on the flat
 * top rather than on the curve.
 */
static FLOAT32 HardClip(const FLOAT32 fIn)
{
    const FLOAT32 fA = fabsf(fIn);
    FLOAT32       fY;

    if (fA >= 1.0f)
    {
        fY = 1.0f;
    }
    else
    {
        /* 1.5a - 0.5a^3: reaches exactly 1 at a = 1, and its slope reaches
           exactly 0 there, so the join with the flat top has no corner. */
        fY = (1.5f * fA) - (0.5f * fA * fA * fA);
    }

    return (fIn < 0.0f) ? -fY : fY;
}

//--------------------------------------------------------------------------------------------------

static void DistortionRun(const FX_CTX* pCtx,
                          FLOAT32* const apPlane[],
                          const U16 nFrames,
                          const FLOAT32 fDrive,
                          const FLOAT32 fToneHz,
                          const FLOAT32 fLevel,
                          const FLOAT32 fMix,
                          const FLOAT32 fSpread)
{
    const U8      nWidth   = pCtx->nWidth;
    const U8      nBase    = pCtx->nPlaneBase;
    const FLOAT32 fDryGain = 1.0f - fMix;

    /* One pole, at the ORIGINAL rate - it runs after decimation, where a real
       pedal's tone control sits. */
    const FLOAT32 fToneCoef = FxUtil_Clamp(1.0f - expf(-DIST_TWO_PI * fToneHz /
                                                       (FLOAT32)AUDIO_SAMPLE_RATE_HZ),
                                           0.0f, 1.0f);

    BIQUAD_COEF tCoefA;
    BIQUAD_COEF tCoefB;
    U8          p;
    U16         i;
    U8          n;

    MakeLowpass(&tCoefA, DIST_AA_CUTOFF_HZ, DIST_OS_RATE, DIST_Q_A);
    MakeLowpass(&tCoefB, DIST_AA_CUTOFF_HZ, DIST_OS_RATE, DIST_Q_B);

    for (p = 0U; p < nWidth; p++)
    {
        DIST_STATE* const pSt = &aState[nBase + p];

        /* The two sides clip a little differently, so the result has width
           instead of being the same mono distortion in two places. */
        const FLOAT32 fSideDrive = fDrive * (1.0f + (((p == 0U) ? -0.5f : 0.5f) * fSpread));

        for (i = 0U; i < nFrames; i++)
        {
            const FLOAT32 fIn = apPlane[p][i];
            FLOAT32       fKeep = 0.0f;

            for (n = 0U; n < DIST_OS; n++)
            {
                /* Zero stuffing halves the level, so the kept sample carries
                   the whole input and its partner carries nothing. The factor
                   of two puts the level back. */
                FLOAT32 fX = (n == 0U) ? (fIn * (FLOAT32)DIST_OS) : 0.0f;

                fX = BiquadRun(&pSt->atUp[0], &tCoefA, fX);
                fX = BiquadRun(&pSt->atUp[1], &tCoefB, fX);

                fX = HardClip(fX * fSideDrive);

                fX = BiquadRun(&pSt->atDown[0], &tCoefA, fX);
                fX = BiquadRun(&pSt->atDown[1], &tCoefB, fX);

                /* Decimate: keep one of every two, now that everything above
                   the original Nyquist has been filtered away. */
                if (n == 0U)
                {
                    fKeep = fX;
                }
            }

            pSt->fTone += fToneCoef * (fKeep - pSt->fTone);

            apPlane[p][i] = (fIn * fDryGain) + (pSt->fTone * fLevel * fMix);
        }
    }
}



/***************************************************************************************************
* Definitions of global (public) functions - MONO
***************************************************************************************************/

void FxDistortionM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxDistortionM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    DistortionRun(pCtx, apPlane, nFrames,
                  FxParam_Exp(&pCtx->pParam[FX_DISTM_P_DRIVE], DIST_DRIVE_MIN, DIST_DRIVE_MAX),
                  FxParam_Exp(&pCtx->pParam[FX_DISTM_P_TONE], DIST_TONE_MIN_HZ, DIST_TONE_MAX_HZ),
                  FxParam_Lin(&pCtx->pParam[FX_DISTM_P_LEVEL], 0.0f, DIST_LEVEL_MAX),
                  FxParam_Norm(&pCtx->pParam[FX_DISTM_P_MIX]),
                  0.0f);
}



/***************************************************************************************************
* Definitions of global (public) functions - STEREO
***************************************************************************************************/

void FxDistortionS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetPlanes(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxDistortionS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    DistortionRun(pCtx, apPlane, nFrames,
                  FxParam_Exp(&pCtx->pParam[FX_DISTS_P_DRIVE], DIST_DRIVE_MIN, DIST_DRIVE_MAX),
                  FxParam_Exp(&pCtx->pParam[FX_DISTS_P_TONE], DIST_TONE_MIN_HZ, DIST_TONE_MAX_HZ),
                  FxParam_Lin(&pCtx->pParam[FX_DISTS_P_LEVEL], 0.0f, DIST_LEVEL_MAX),
                  FxParam_Norm(&pCtx->pParam[FX_DISTS_P_MIX]),
                  FxParam_Norm(&pCtx->pParam[FX_DISTS_P_SPREAD]) * DIST_SPREAD_MAX);
}

/****************************************** end of file *******************************************/
