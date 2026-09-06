/**
 * @file      fx_modulation.c
 *
 * @details   Chorus, flanger and vibrato. See fx_modulation.h for the model.
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

#include "fx_modulation.h"

#include "mem_map.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

#define MOD_TWO_PI                  (6.28318531f)

/** Frames per millisecond at the engine's rate. */
#define MOD_FRAMES_PER_MS           ((FLOAT32)AUDIO_SAMPLE_RATE_HZ * 0.001f)



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

/**
 * @brief Per-plane write position, plus the LFO phase of the INSTANCE.
 *
 * The phase is only meaningful in the base plane's entry: a stereo instance has
 * one LFO and derives the second plane's position from it by SPREAD, which is
 * what keeps the two sides locked together instead of drifting.
 */
typedef struct stMOD_STATE
{
    U32     nWritePos;
    FLOAT32 fPhase;

} MOD_STATE;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Modulation delay lines, internal SRAM. Shared by each effect's two variants,
 * because a mono and a stereo instance can never be live on the same planes.
 *
 * These are why mem_map.h has two memory tiers. A fully loaded machine has many
 * short lines being read and written every block; in SDRAM they would replace
 * the entire D-cache several times per block with zero reuse. In AXI SRAM they
 * cost nothing.
 */
static FLOAT32 aChorusLine[AUDIO_PLANE_QTY][MODDELAY_MAX_FRAMES]  IN_SRAM_FAST MEM_ALIGN(32);
static FLOAT32 aFlangerLine[AUDIO_PLANE_QTY][MODDELAY_MAX_FRAMES] IN_SRAM_FAST MEM_ALIGN(32);
static FLOAT32 aVibratoLine[AUDIO_PLANE_QTY][MODDELAY_MAX_FRAMES] IN_SRAM_FAST MEM_ALIGN(32);

static MOD_STATE aChorusState[AUDIO_PLANE_QTY]  IN_DTCM;

/** Per-voice base-delay multipliers - see CHORUS_VOICE_QTY in the header. */
static const FLOAT32 afChorusVoice[CHORUS_VOICE_QTY] = CHORUS_VOICE_SPREAD;
static MOD_STATE aFlangerState[AUDIO_PLANE_QTY] IN_DTCM;
static MOD_STATE aVibratoState[AUDIO_PLANE_QTY] IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static void ResetBank(FLOAT32 (*const pBank)[MODDELAY_MAX_FRAMES],
                      MOD_STATE* const pState,
                      const U8 nPlaneBase,
                      const U8 nWidth)
{
    U8 p;

    for (p = 0U; p < nWidth; p++)
    {
        const U8 nPlane = nPlaneBase + p;

        if (nPlane < AUDIO_PLANE_QTY)
        {
            U32 i;

            for (i = 0UL; i < (U32)MODDELAY_MAX_FRAMES; i++)
            {
                pBank[nPlane][i] = 0.0f;
            }

            pState[nPlane].nWritePos = 0UL;
            pState[nPlane].fPhase    = 0.0f;
        }
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief The whole family, in one loop.
 *
 * @param nVoices     taps summed per plane. THREE for chorus, one for the
 *                    others - a single voice mixed with the dry is a comb
 *                    filter, which is a flanger however long the delay.
 * @param fBaseMs     tap position at the bottom of the sweep
 * @param fSweepMs    how far the LFO moves it
 * @param fFeedback   0 for chorus and vibrato, the comb's regeneration for flanger
 * @param fMix        1.0 for vibrato - see fx_modulation.h on why it has no dry
 * @param fSpread     LFO phase offset between the planes, in half cycles
 */
static void ModRun(const FX_CTX* pCtx,
                   FLOAT32* const apPlane[],
                   const U16 nFrames,
                   FLOAT32 (*const pBank)[MODDELAY_MAX_FRAMES],
                   MOD_STATE* const pState,
                   const U8 nVoices,
                   const FLOAT32 fRateHz,
                   const FLOAT32 fBaseMs,
                   const FLOAT32 fSweepMs,
                   const FLOAT32 fFeedback,
                   const FLOAT32 fMix,
                   const FLOAT32 fSpread)
{
    const U8      nWidth   = pCtx->nWidth;
    const U8      nBase    = pCtx->nPlaneBase;
    const FLOAT32 fInc     = fRateHz / (FLOAT32)AUDIO_SAMPLE_RATE_HZ;
    const FLOAT32 fDryGain = 1.0f - fMix;

    FLOAT32 fPhase = pState[nBase].fPhase;
    U8      p;
    U16     i;

    for (i = 0U; i < nFrames; i++)
    {
        for (p = 0U; p < nWidth; p++)
        {
            MOD_STATE* const pSt = &pState[nBase + p];
            FLOAT32*   const pLn = pBank[nBase + p];

            const FLOAT32 fIn = apPlane[p][i];
            FLOAT32       fWetSum = 0.0f;
            FLOAT32       fWetFb  = 0.0f;
            U8            v;

            for (v = 0U; v < nVoices; v++)
            {
                /* Half a cycle at full spread puts the two PLANES in antiphase;
                   the voices are spread evenly around the whole cycle so their
                   combs never coincide. */
                const FLOAT32 fPh   = fPhase + ((FLOAT32)p * fSpread * 0.5f)
                                             + ((FLOAT32)v / (FLOAT32)nVoices);
                const FLOAT32 fLfo  = 0.5f + (0.5f * sinf(MOD_TWO_PI * fPh));
                const FLOAT32 fBase = (nVoices > 1U) ? (fBaseMs * afChorusVoice[v]) : fBaseMs;
                const FLOAT32 fTapF = (fBase + (fSweepMs * fLfo)) * MOD_FRAMES_PER_MS;

                FLOAT32 fWet;
                U32     nTapInt = (U32)fTapF;

                /* FxUtil_DelayRead needs room for the interpolation on both sides. */
                if (nTapInt < 1UL)
                {
                    nTapInt = 1UL;
                }
                else if (nTapInt > ((U32)MODDELAY_MAX_FRAMES - 2UL))
                {
                    nTapInt = (U32)MODDELAY_MAX_FRAMES - 2UL;
                }
                else
                {
                    do_nothing();
                }

                fWet = FxUtil_DelayRead(pLn, (U32)MODDELAY_MAX_FRAMES, pSt->nWritePos,
                                        nTapInt, fTapF - (FLOAT32)((U32)fTapF));

                fWetSum += fWet;

                if (v == 0U)
                {
                    fWetFb = fWet;      /* only the flanger feeds back, and it has one voice */
                }
            }

            if (nVoices > 1U)
            {
                fWetSum *= CHORUS_VOICE_NORM;
            }

            /* Soft clip the regeneration only, never the signal on its way in -
               the same rule as the delay, and for the same reason. */
            pLn[pSt->nWritePos] = fIn + FxUtil_SoftClip(fWetFb * fFeedback);

            pSt->nWritePos++;
            if (pSt->nWritePos >= (U32)MODDELAY_MAX_FRAMES)
            {
                pSt->nWritePos = 0UL;
            }

            apPlane[p][i] = (fIn * fDryGain) + (fWetSum * fMix);
        }

        fPhase += fInc;
        if (fPhase >= 1.0f)
        {
            fPhase -= 1.0f;
        }
    }

    pState[nBase].fPhase = fPhase;
}



/***************************************************************************************************
* Definitions of global (public) functions - CHORUS
***************************************************************************************************/

void FxChorusM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetBank(aChorusLine, aChorusState, nPlaneBase, nWidth);
}

void FxChorusM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    ModRun(pCtx, apPlane, nFrames, aChorusLine, aChorusState,
           (U8)CHORUS_VOICE_QTY,
           FxParam_RateHz(&pCtx->pParam[FX_CHORUSM_P_RATE], pCtx->pTempo,
                          MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ),
           FxParam_Lin(&pCtx->pParam[FX_CHORUSM_P_DELAY],
                       CHORUS_DELAY_MIN_MS, CHORUS_DELAY_MAX_MS),
           FxParam_Norm(&pCtx->pParam[FX_CHORUSM_P_DEPTH]) * CHORUS_SWEEP_MS,
           0.0f,
           FxParam_Norm(&pCtx->pParam[FX_CHORUSM_P_MIX]),
           0.0f);
}

void FxChorusS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetBank(aChorusLine, aChorusState, nPlaneBase, nWidth);
}

void FxChorusS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    ModRun(pCtx, apPlane, nFrames, aChorusLine, aChorusState,
           (U8)CHORUS_VOICE_QTY,
           FxParam_RateHz(&pCtx->pParam[FX_CHORUSS_P_RATE], pCtx->pTempo,
                          MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ),
           FxParam_Lin(&pCtx->pParam[FX_CHORUSS_P_DELAY],
                       CHORUS_DELAY_MIN_MS, CHORUS_DELAY_MAX_MS),
           FxParam_Norm(&pCtx->pParam[FX_CHORUSS_P_DEPTH]) * CHORUS_SWEEP_MS,
           0.0f,
           FxParam_Norm(&pCtx->pParam[FX_CHORUSS_P_MIX]),
           FxParam_Norm(&pCtx->pParam[FX_CHORUSS_P_SPREAD]));
}



/***************************************************************************************************
* Definitions of global (public) functions - FLANGER
***************************************************************************************************/

void FxFlangerM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetBank(aFlangerLine, aFlangerState, nPlaneBase, nWidth);
}

void FxFlangerM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    ModRun(pCtx, apPlane, nFrames, aFlangerLine, aFlangerState,
           1U,
           FxParam_RateHz(&pCtx->pParam[FX_FLANGERM_P_RATE], pCtx->pTempo,
                          MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ),
           FLANGER_DELAY_MIN_MS,
           FxParam_Norm(&pCtx->pParam[FX_FLANGERM_P_DEPTH]) * FLANGER_SWEEP_MS,
           FxParam_Lin(&pCtx->pParam[FX_FLANGERM_P_FEEDBACK], 0.0f, FLANGER_FEEDBACK_MAX),
           FxParam_Norm(&pCtx->pParam[FX_FLANGERM_P_MIX]),
           0.0f);
}

void FxFlangerS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetBank(aFlangerLine, aFlangerState, nPlaneBase, nWidth);
}

void FxFlangerS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    ModRun(pCtx, apPlane, nFrames, aFlangerLine, aFlangerState,
           1U,
           FxParam_RateHz(&pCtx->pParam[FX_FLANGERS_P_RATE], pCtx->pTempo,
                          MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ),
           FLANGER_DELAY_MIN_MS,
           FxParam_Norm(&pCtx->pParam[FX_FLANGERS_P_DEPTH]) * FLANGER_SWEEP_MS,
           FxParam_Lin(&pCtx->pParam[FX_FLANGERS_P_FEEDBACK], 0.0f, FLANGER_FEEDBACK_MAX),
           FxParam_Norm(&pCtx->pParam[FX_FLANGERS_P_MIX]),
           FxParam_Norm(&pCtx->pParam[FX_FLANGERS_P_SPREAD]));
}



/***************************************************************************************************
* Definitions of global (public) functions - VIBRATO
***************************************************************************************************/

void FxVibratoM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetBank(aVibratoLine, aVibratoState, nPlaneBase, nWidth);
}

void FxVibratoM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    const FLOAT32 fDepth = FxParam_Norm(&pCtx->pParam[FX_VIBRATOM_P_DEPTH]);

    ModRun(pCtx, apPlane, nFrames, aVibratoLine, aVibratoState,
           1U,
           FxParam_RateHz(&pCtx->pParam[FX_VIBRATOM_P_RATE], pCtx->pTempo,
                          MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ),
           VIBRATO_DELAY_MID_MS - (fDepth * VIBRATO_SWEEP_MS * 0.5f),
           fDepth * VIBRATO_SWEEP_MS,
           0.0f,
           1.0f,            /* no dry path - vibrato is pitch, not thickening */
           0.0f);
}

void FxVibratoS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetBank(aVibratoLine, aVibratoState, nPlaneBase, nWidth);
}

void FxVibratoS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    const FLOAT32 fDepth = FxParam_Norm(&pCtx->pParam[FX_VIBRATOS_P_DEPTH]);

    ModRun(pCtx, apPlane, nFrames, aVibratoLine, aVibratoState,
           1U,
           FxParam_RateHz(&pCtx->pParam[FX_VIBRATOS_P_RATE], pCtx->pTempo,
                          MOD_RATE_MIN_HZ, MOD_RATE_MAX_HZ),
           VIBRATO_DELAY_MID_MS - (fDepth * VIBRATO_SWEEP_MS * 0.5f),
           fDepth * VIBRATO_SWEEP_MS,
           0.0f,
           1.0f,
           FxParam_Norm(&pCtx->pParam[FX_VIBRATOS_P_SPREAD]));
}

/****************************************** end of file *******************************************/
