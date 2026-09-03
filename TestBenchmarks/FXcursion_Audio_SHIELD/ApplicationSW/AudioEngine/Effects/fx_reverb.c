/**
 * @file      fx_reverb.c
 *
 * @details   Reverb implementation. See fx_reverb.h for the model.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - Eight lines, in-loop diffusion, room size
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_reverb.h"

#include "mem_map.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

#define REV_TWO_PI                  (6.28318531f)

/**
 * Lines in the network.
 *
 * Four was not enough, and the measurement that proved it was echo density: an
 * impulse into the four-line version produced NOTHING above -66 dB in its first
 * fifty milliseconds. Four lines of 40 to 74 ms give perhaps eighty echoes a
 * second where a real room gives thousands, and the ear hears that as thin and
 * far away rather than as a long tail.
 *
 * Eight lines double it, and the in-loop allpasses below multiply it again on
 * every pass.
 */
#define REV_LINE_QTY                (8U)

/** Input diffusion allpasses, in series before the network. */
#define REV_AP_QTY                  (4U)

/** Longest pre-delay, in frames. Integer arithmetic only - see fx_reverb.h. */
#define REV_PREDELAY_FRAMES         ((AUDIO_SAMPLE_RATE_HZ * REV_PREDELAY_MAX_MS) / 1000UL)

/*
 * Delay lengths, frames at 48 kHz.
 *
 * Every one is prime, and that is the whole trick. A common factor between two
 * lines makes their echoes coincide periodically, which the ear hears as a
 * metallic ring sitting on top of the tail. Primes never line up.
 *
 * The network lines span 16 to 60 ms - deliberately shorter than the first
 * version's 40 to 74. Short lines recirculate more often, and how often the
 * network folds back on itself is exactly what builds density.
 */
#define REV_LINE_LENS               { 761U, 1013U, 1289U, 1583U, 1867U, 2137U, 2503U, 2861U }
#define REV_LINE_TOTAL              (14014U)

/*
 * An allpass inside each feedback path.
 *
 * This is the difference between a bank of comb filters and something that
 * sounds like a room. An allpass has unity magnitude at every frequency, so it
 * costs the tail no energy and does not change the decay time by one decibel -
 * it only smears. Each pass round the loop therefore multiplies the number of
 * distinct echoes instead of merely repeating them.
 */
#define REV_LOOP_AP_LENS            { 137U, 149U, 211U, 263U, 317U, 367U, 421U, 463U }
#define REV_LOOP_AP_TOTAL           (2328U)

/** Coefficient of the in-loop allpasses. Fixed - it is a quality, not a knob. */
#define REV_LOOP_AP_GAIN            (0.55f)

/**
 * How long energy actually dwells in one of those allpasses, in units of its
 * buffer length: (1 + g^2) / (1 - g^2), which is 1.867 at g = 0.55.
 *
 * NOT 1.0. An allpass is a feedback loop, so a pulse entering it comes back out
 * as a decaying series at L, 2L, 3L and so on - it holds energy far longer than
 * its buffer is long.
 *
 * This matters because an FDN only decays at a single uniform rate if each
 * line's gain is the per-sample decay raised to that line's TRUE round trip.
 * Using the raw buffer length instead broke that condition, the slowest mode
 * won, and every tail came out about 37% longer than the knob asked for.
 */
#define REV_LOOP_AP_DWELL           (1.8674f)

#define REV_AP0_LEN                 (229U)
#define REV_AP1_LEN                 (173U)
#define REV_AP2_LEN                 (613U)
#define REV_AP3_LEN                 (449U)
#define REV_IN_AP_TOTAL             (REV_AP0_LEN + REV_AP1_LEN + REV_AP2_LEN + REV_AP3_LEN)

/* Offsets inside one bank. */
#define REV_OFF_PREDELAY            (0U)
#define REV_OFF_IN_AP               (REV_OFF_PREDELAY + REV_PREDELAY_FRAMES)
#define REV_OFF_LINES               (REV_OFF_IN_AP + REV_IN_AP_TOTAL)
#define REV_OFF_LOOP_AP             (REV_OFF_LINES + REV_LINE_TOTAL)
#define REV_BANK_USED               (REV_OFF_LOOP_AP + REV_LOOP_AP_TOTAL)

/* A mono instance puts everything in one bank, so that bank has to hold it. */
FXC_STATIC_ASSERT(REV_BANK_USED <= REVERB_FRAMES_PER_PLANE, reverb_fits_one_bank);

/** 1/sqrt(8): injecting into eight lines without eight times the energy. */
#define REV_IN_GAIN                 (0.35355339f)

/** Highest per-line feedback gain allowed, whatever decay time is asked for. */
#define REV_MAX_FEEDBACK            (0.9999f)

/** Below this the damping state is treated as silence - see fx_reverb.h. */
#define REV_DENORMAL_FLOOR          (1.0e-20f)



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

/**
 * @brief One reverb instance. Indexed by the plane BASE, because the stereo
 *        variant is a single instance spanning two planes.
 */
typedef struct stREVERB_STATE
{
    U32     nPredelayPos;
    U32     anInApPos[REV_AP_QTY];
    U32     anLinePos[REV_LINE_QTY];
    U32     anLoopApPos[REV_LINE_QTY];
    FLOAT32 afDamp[REV_LINE_QTY];

} REVERB_STATE;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Reverb delay memory, SDRAM bank 2 - on its own chip select, away from the
 * delay lines, because the two access patterns would thrash each other's open
 * rows if they shared a bank.
 *
 * 65536 frames per plane is 256 KiB, of which one instance uses about 42000.
 */
static FLOAT32 aReverbMem[AUDIO_PLANE_QTY][REVERB_FRAMES_PER_PLANE] IN_SDRAM_REVERB MEM_ALIGN(32);

static REVERB_STATE aState[AUDIO_PLANE_QTY] IN_DTCM;

static const U32 anLineLen[REV_LINE_QTY]   = REV_LINE_LENS;
static const U32 anLoopApLen[REV_LINE_QTY] = REV_LOOP_AP_LENS;

static const U32 anInApLen[REV_AP_QTY] =
{
    REV_AP0_LEN, REV_AP1_LEN, REV_AP2_LEN, REV_AP3_LEN
};



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static void ResetInstance(const U8 nPlaneBase, const U8 nWidth)
{
    U8 p;

    for (p = 0U; p < nWidth; p++)
    {
        const U8 nPlane = nPlaneBase + p;

        if (nPlane < AUDIO_PLANE_QTY)
        {
            U32 i;

            for (i = 0UL; i < (U32)REVERB_FRAMES_PER_PLANE; i++)
            {
                aReverbMem[nPlane][i] = 0.0f;
            }
        }
    }

    if (nPlaneBase < AUDIO_PLANE_QTY)
    {
        REVERB_STATE* const pSt = &aState[nPlaneBase];
        U8 i;

        pSt->nPredelayPos = 0UL;

        for (i = 0U; i < REV_AP_QTY; i++)
        {
            pSt->anInApPos[i] = 0UL;
        }

        for (i = 0U; i < REV_LINE_QTY; i++)
        {
            pSt->anLinePos[i]   = 0UL;
            pSt->anLoopApPos[i] = 0UL;
            pSt->afDamp[i]      = 0.0f;
        }
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Schroeder allpass. Smears a signal in time without colouring it.
 */
static FLOAT32 RevAllpass(FLOAT32* const pLine,
                          const U32 nLen,
                          U32* const pPos,
                          const FLOAT32 fIn,
                          const FLOAT32 fG)
{
    const FLOAT32 fDelayed = pLine[*pPos];
    const FLOAT32 fV       = fIn + (fG * fDelayed);

    pLine[*pPos] = fV;

    *pPos = *pPos + 1UL;
    if (*pPos >= nLen)
    {
        *pPos = 0UL;
    }

    return fDelayed - (fG * fV);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Normalised 8x8 Hadamard, in place.
 *
 * Three butterfly stages - 24 adds - instead of 64 multiply accumulates. The
 * 1/sqrt(8) makes it orthonormal, which is what lets the decay time be an
 * actual number: the matrix moves energy between lines without creating or
 * destroying any, so everything that sets the tail length is in the gains.
 */
static void Hadamard8(FLOAT32* const pV)
{
    FLOAT32 a[REV_LINE_QTY];
    U8      i;

    for (i = 0U; i < 8U; i += 2U)
    {
        a[i]      = pV[i] + pV[i + 1U];
        a[i + 1U] = pV[i] - pV[i + 1U];
    }

    for (i = 0U; i < 8U; i += 4U)
    {
        pV[i]      = a[i]      + a[i + 2U];
        pV[i + 1U] = a[i + 1U] + a[i + 3U];
        pV[i + 2U] = a[i]      - a[i + 2U];
        pV[i + 3U] = a[i + 1U] - a[i + 3U];
    }

    for (i = 0U; i < 4U; i++)
    {
        const FLOAT32 fLo = pV[i];
        const FLOAT32 fHi = pV[i + 4U];

        pV[i]      = (fLo + fHi) * REV_IN_GAIN;
        pV[i + 4U] = (fLo - fHi) * REV_IN_GAIN;
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief The network, for one or two planes.
 *
 * One instance either way. The stereo variant sums both planes in and takes two
 * different combinations out - see fx_reverb.h on why that is not the same as
 * running the mono version twice.
 */
static void ReverbRun(const FX_CTX* pCtx,
                      FLOAT32* const apPlane[],
                      const U16 nFrames,
                      const FLOAT32 fPredelaySec,
                      const FLOAT32 fDecaySec,
                      const FLOAT32 fSize,
                      const FLOAT32 fDampHz,
                      const FLOAT32 fDiffusion,
                      const FLOAT32 fMix,
                      const FLOAT32 fWidth)
{
    const U8            nWidth = pCtx->nWidth;
    const U8            nBase  = pCtx->nPlaneBase;
    REVERB_STATE* const pSt    = &aState[nBase];

    FLOAT32* const pBankA = aReverbMem[nBase];

    const FLOAT32 fDamp    = FxUtil_Clamp(1.0f - expf(-REV_TWO_PI * fDampHz /
                                                      (FLOAT32)AUDIO_SAMPLE_RATE_HZ),
                                          0.0f, 1.0f);
    const FLOAT32 fDryGain = 1.0f - fMix;
    const FLOAT32 fOutGain = REV_IN_GAIN;       /* 1/sqrt(8), summing eight taps */

    FLOAT32* apLine[REV_LINE_QTY];
    FLOAT32* apLoopAp[REV_LINE_QTY];
    U32      anDelay[REV_LINE_QTY];
    FLOAT32  afFeedback[REV_LINE_QTY];
    U32      nDecayFrames;
    U32      nPredelay;
    U32      nOff;
    U8       k;
    U16      i;

    /* Everything lives in the first bank; the second is left for a future
       larger tank. Offsets are fixed, so this is address arithmetic, not a
       search. */
    nOff = REV_OFF_LINES;
    for (k = 0U; k < REV_LINE_QTY; k++)
    {
        apLine[k] = &pBankA[nOff];
        nOff     += anLineLen[k];
    }

    nOff = REV_OFF_LOOP_AP;
    for (k = 0U; k < REV_LINE_QTY; k++)
    {
        apLoopAp[k] = &pBankA[nOff];
        nOff       += anLoopApLen[k];
    }

    nDecayFrames = (U32)(FxUtil_Clamp(fDecaySec, REV_DECAY_MIN_SEC, REV_DECAY_MAX_SEC) *
                         (FLOAT32)AUDIO_SAMPLE_RATE_HZ);

    if (nDecayFrames < 1UL)
    {
        nDecayFrames = 1UL;
    }

    for (k = 0U; k < REV_LINE_QTY; k++)
    {
        /* SIZE shortens the read rather than the buffer, so a smaller room is a
           shorter loop and needs a proportionally larger gain to decay over the
           same time. */
        FLOAT32 fLoop;

        anDelay[k] = (U32)((FLOAT32)anLineLen[k] * FxUtil_Clamp(fSize, REV_SIZE_MIN, 1.0f));

        if (anDelay[k] < 1UL)
        {
            anDelay[k] = 1UL;
        }

        /* The in-loop allpass is part of the round trip, and it holds energy
           for longer than it is long - see REV_LOOP_AP_DWELL. */
        fLoop = (FLOAT32)anDelay[k] +
                ((FLOAT32)anLoopApLen[k] * REV_LOOP_AP_DWELL);

        afFeedback[k] = FxUtil_Clamp(powf(10.0f, -3.0f * fLoop / (FLOAT32)nDecayFrames),
                                     0.0f, REV_MAX_FEEDBACK);
    }

    nPredelay = (U32)(FxUtil_Clamp(fPredelaySec, REV_PREDELAY_MIN_SEC, REV_PREDELAY_MAX_SEC) *
                      (FLOAT32)AUDIO_SAMPLE_RATE_HZ);

    if (nPredelay >= REV_PREDELAY_FRAMES)
    {
        nPredelay = REV_PREDELAY_FRAMES - 1UL;
    }

    for (i = 0U; i < nFrames; i++)
    {
        FLOAT32 afTap[REV_LINE_QTY];
        FLOAT32 afNode[REV_LINE_QTY];
        FLOAT32 fIn;
        FLOAT32 fX;
        U32     nRead;

        fIn = (nWidth == CHAIN_MAX_WIDTH) ? (0.5f * (apPlane[0][i] + apPlane[1][i]))
                                          : apPlane[0][i];

        /* ---- pre-delay ------------------------------------------------------ */
        pBankA[REV_OFF_PREDELAY + pSt->nPredelayPos] = fIn;

        nRead = pSt->nPredelayPos + REV_PREDELAY_FRAMES - nPredelay;
        if (nRead >= REV_PREDELAY_FRAMES)
        {
            nRead -= REV_PREDELAY_FRAMES;
        }

        fX = pBankA[REV_OFF_PREDELAY + nRead];

        pSt->nPredelayPos++;
        if (pSt->nPredelayPos >= REV_PREDELAY_FRAMES)
        {
            pSt->nPredelayPos = 0UL;
        }

        /* ---- input diffusion ------------------------------------------------ */
        nOff = REV_OFF_IN_AP;
        for (k = 0U; k < REV_AP_QTY; k++)
        {
            fX = RevAllpass(&pBankA[nOff], anInApLen[k], &pSt->anInApPos[k], fX, fDiffusion);
            nOff += anInApLen[k];
        }

        /* ---- read, damp, and diffuse inside the loop ------------------------ */
        for (k = 0U; k < REV_LINE_QTY; k++)
        {
            nRead = pSt->anLinePos[k] + anLineLen[k] - anDelay[k];
            if (nRead >= anLineLen[k])
            {
                nRead -= anLineLen[k];
            }

            afTap[k] = apLine[k][nRead];

            pSt->afDamp[k] += fDamp * (afTap[k] - pSt->afDamp[k]);

            if (fabsf(pSt->afDamp[k]) < REV_DENORMAL_FLOOR)
            {
                pSt->afDamp[k] = 0.0f;
            }

            afNode[k] = RevAllpass(apLoopAp[k], anLoopApLen[k], &pSt->anLoopApPos[k],
                                   pSt->afDamp[k], REV_LOOP_AP_GAIN);
        }

        /* ---- mix and write back --------------------------------------------- */
        Hadamard8(afNode);

        for (k = 0U; k < REV_LINE_QTY; k++)
        {
            apLine[k][pSt->anLinePos[k]] = (fX * REV_IN_GAIN) + (afNode[k] * afFeedback[k]);

            pSt->anLinePos[k]++;
            if (pSt->anLinePos[k] >= anLineLen[k])
            {
                pSt->anLinePos[k] = 0UL;
            }
        }

        /* ---- out ------------------------------------------------------------ */
        if (nWidth == CHAIN_MAX_WIDTH)
        {
            /* Alternate lines to each side. Their lengths are different and
               mutually prime, so the two sides are genuinely decorrelated
               rather than the same tail at two levels. */
            const FLOAT32 fWetL = (afTap[0] + afTap[2] + afTap[4] + afTap[6]) * 0.5f;
            const FLOAT32 fWetR = (afTap[1] + afTap[3] + afTap[5] + afTap[7]) * 0.5f;
            const FLOAT32 fMid  = 0.5f * (fWetL + fWetR);
            const FLOAT32 fSide = 0.5f * (fWetL - fWetR) * fWidth;

            apPlane[0][i] = (apPlane[0][i] * fDryGain) + ((fMid + fSide) * fMix);
            apPlane[1][i] = (apPlane[1][i] * fDryGain) + ((fMid - fSide) * fMix);
        }
        else
        {
            FLOAT32 fWet = 0.0f;

            for (k = 0U; k < REV_LINE_QTY; k++)
            {
                fWet += afTap[k];
            }

            apPlane[0][i] = (apPlane[0][i] * fDryGain) + (fWet * fOutGain * fMix);
        }
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Damping cutoff for a knob where 0 is bright and 1 is dark.
 *
 * FxParam_Exp cannot express a descending range, so the interpolation is done
 * here rather than by handing it a reversed pair it would reject.
 */
static FLOAT32 DampHz(const FX_PARAM* const pParam)
{
    const FLOAT32 fNorm = FxParam_Norm(pParam);

    return REV_DAMP_BRIGHT_HZ * powf(REV_DAMP_DARK_HZ / REV_DAMP_BRIGHT_HZ, fNorm);
}



/***************************************************************************************************
* Definitions of global (public) functions - MONO
***************************************************************************************************/

void FxReverbM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetInstance(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxReverbM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    ReverbRun(pCtx, apPlane, nFrames,
              FxParam_TimeSec(&pCtx->pParam[FX_REVERBM_P_PREDELAY], pCtx->pTempo,
                              REV_PREDELAY_MIN_SEC, REV_PREDELAY_MAX_SEC),
              FxParam_Exp(&pCtx->pParam[FX_REVERBM_P_DECAY],
                          REV_DECAY_MIN_SEC, REV_DECAY_MAX_SEC),
              FxParam_Lin(&pCtx->pParam[FX_REVERBM_P_SIZE], REV_SIZE_MIN, 1.0f),
              DampHz(&pCtx->pParam[FX_REVERBM_P_DAMPING]),
              FxParam_Lin(&pCtx->pParam[FX_REVERBM_P_DIFFUSION], 0.0f, REV_DIFFUSION_MAX),
              FxParam_Norm(&pCtx->pParam[FX_REVERBM_P_MIX]),
              1.0f);        /* one plane - width has no meaning */
}



/***************************************************************************************************
* Definitions of global (public) functions - STEREO
***************************************************************************************************/

void FxReverbS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    ResetInstance(nPlaneBase, nWidth);
}

//--------------------------------------------------------------------------------------------------

void FxReverbS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    ReverbRun(pCtx, apPlane, nFrames,
              FxParam_TimeSec(&pCtx->pParam[FX_REVERBS_P_PREDELAY], pCtx->pTempo,
                              REV_PREDELAY_MIN_SEC, REV_PREDELAY_MAX_SEC),
              FxParam_Exp(&pCtx->pParam[FX_REVERBS_P_DECAY],
                          REV_DECAY_MIN_SEC, REV_DECAY_MAX_SEC),
              FxParam_Lin(&pCtx->pParam[FX_REVERBS_P_SIZE], REV_SIZE_MIN, 1.0f),
              DampHz(&pCtx->pParam[FX_REVERBS_P_DAMPING]),
              FxParam_Lin(&pCtx->pParam[FX_REVERBS_P_DIFFUSION], 0.0f, REV_DIFFUSION_MAX),
              FxParam_Norm(&pCtx->pParam[FX_REVERBS_P_MIX]),
              FxParam_Lin(&pCtx->pParam[FX_REVERBS_P_WIDTH], 0.0f, REV_WIDTH_MAX));
}

/****************************************** end of file *******************************************/
