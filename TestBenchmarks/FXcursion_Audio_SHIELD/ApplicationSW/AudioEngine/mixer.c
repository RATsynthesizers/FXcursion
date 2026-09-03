/**
 * @file      mixer.c
 *
 * @details   Routing matrix implementation.
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

#include "mixer.h"

#include "mem_map.h"
#include "Effects/fx_common.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/** Wire gain 0..65535 maps onto 0.0 .. MIX_GAIN_MAX linear. 32768 is unity. */
#define MIX_GAIN_MAX                (2.0f)

/** -3 dB. Power-preserving stereo-to-mono fold. */
#define MIX_FOLD_GAIN               (0.70710678f)



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/* Targets, written by the super-loop. */
static FLOAT32 aTargetGain[CHAIN_MAX_QTY][CHAIN_MAX_QTY] IN_DTCM;
static FLOAT32 aTargetPan[CHAIN_MAX_QTY][CHAIN_MAX_QTY]  IN_DTCM;

/* Ramped values, owned by the audio path. */
static FLOAT32 aCurGain[CHAIN_MAX_QTY][CHAIN_MAX_QTY]    IN_DTCM;

/* Source snapshot: the mixer reads all chains and writes all chains, so it needs
 * a copy of the inputs. 4 planes x 64 frames x 4 B = 1 KiB in DTCM. */
static FLOAT32 aSrc[AUDIO_PLANE_QTY][AUDIO_BLOCK_FRAMES] IN_DTCM;



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT Mixer_Init(void)
{
    U8 i;
    U8 j;

    for (i = 0U; i < CHAIN_MAX_QTY; i++)
    {
        for (j = 0U; j < CHAIN_MAX_QTY; j++)
        {
            const FLOAT32 fUnity = (i == j) ? 1.0f : 0.0f;

            aTargetGain[i][j] = fUnity;
            aTargetPan[i][j]  = 0.0f;
            aCurGain[i][j]    = fUnity;     // start settled, do not ramp up at boot
        }
    }

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

void Mixer_Apply(const PROTO_CFG* const pCfg, const GRID* const pGrid)
{
    U8 nDst;
    U8 nSrc;

    (void)pGrid;

    for (nDst = 0U; nDst < CHAIN_MAX_QTY; nDst++)
    {
        for (nSrc = 0U; nSrc < CHAIN_MAX_QTY; nSrc++)
        {
            aTargetGain[nDst][nSrc] = ((FLOAT32)pCfg->aMixGain[nDst][nSrc] / 65535.0f)
                                    * MIX_GAIN_MAX;

            aTargetPan[nDst][nSrc]  = (FLOAT32)pCfg->aMixPan[nDst][nSrc] / 127.0f;
        }
    }
}

//--------------------------------------------------------------------------------------------------

void Mixer_Process(const GRID* const pGrid, FLOAT32* const apPlane[], const U16 nFrames)
{
    const U8      nChainQty = pGrid->nChainQty;
    const FLOAT32 fSmooth   = FxUtil_SmoothCoeff((FLOAT32)MIX_GAIN_SMOOTH_MS, nFrames);
    U8            nPlane;
    U8            nDst;
    U8            nSrc;
    U16           i;

    // Snapshot the inputs, then clear the outputs. Both are needed because every
    // chain is simultaneously a source and a destination.
    for (nPlane = 0U; nPlane < AUDIO_PLANE_QTY; nPlane++)
    {
        (void)memcpy(aSrc[nPlane], apPlane[nPlane], (size_t)nFrames * sizeof(FLOAT32));

        for (i = 0U; i < nFrames; i++)
        {
            apPlane[nPlane][i] = 0.0f;
        }
    }

    for (nDst = 0U; nDst < nChainQty; nDst++)
    {
        const U8 nDstBase  = pGrid->aPlaneBase[nDst];
        const U8 nDstWidth = pGrid->aWidth[nDst];

        FLOAT32  fBusGain  = 1.0f;
        U8       nActive   = 0U;

        if (pGrid->bAutoGain != (U8)FALSE)
        {
            for (nSrc = 0U; nSrc < nChainQty; nSrc++)
            {
                if (aTargetGain[nDst][nSrc] > 0.0f)
                {
                    nActive++;
                }
            }

            // 1/sqrt(N), not 1/N - see the note in mixer.h.
            if (nActive > 1U)
            {
                fBusGain = 1.0f / sqrtf((FLOAT32)nActive);
            }
        }

        for (nSrc = 0U; nSrc < nChainQty; nSrc++)
        {
            const U8 nSrcBase  = pGrid->aPlaneBase[nSrc];
            const U8 nSrcWidth = pGrid->aWidth[nSrc];
            FLOAT32  fGain;

            // Ramp toward the target once per block. Never apply a raw target.
            aCurGain[nDst][nSrc] += fSmooth * (aTargetGain[nDst][nSrc] - aCurGain[nDst][nSrc]);
            fGain = aCurGain[nDst][nSrc] * fBusGain;

            if (fGain <= 0.000001f)
            {
                continue;
            }

            if (nSrcWidth == nDstWidth)
            {
                U8 p;

                for (p = 0U; p < nDstWidth; p++)
                {
                    const FLOAT32* const pIn  = aSrc[nSrcBase + p];
                    FLOAT32* const       pOut = apPlane[nDstBase + p];

                    for (i = 0U; i < nFrames; i++)
                    {
                        pOut[i] += pIn[i] * fGain;
                    }
                }
            }
            else if ((nSrcWidth == 2U) && (nDstWidth == 1U))
            {
                const FLOAT32* const pL   = aSrc[nSrcBase];
                const FLOAT32* const pR   = aSrc[nSrcBase + 1U];
                FLOAT32* const       pOut = apPlane[nDstBase];
                const FLOAT32        fG   = fGain * MIX_FOLD_GAIN;

                for (i = 0U; i < nFrames; i++)
                {
                    pOut[i] += (pL[i] + pR[i]) * fG;
                }
            }
            else /* (nSrcWidth == 1U) && (nDstWidth == 2U) */
            {
                const FLOAT32* const pIn   = aSrc[nSrcBase];
                FLOAT32* const       pOutL = apPlane[nDstBase];
                FLOAT32* const       pOutR = apPlane[nDstBase + 1U];
                FLOAT32              fPanL;
                FLOAT32              fPanR;

                FxUtil_PanGains(aTargetPan[nDst][nSrc], &fPanL, &fPanR);

                fPanL *= fGain;
                fPanR *= fGain;

                for (i = 0U; i < nFrames; i++)
                {
                    pOutL[i] += pIn[i] * fPanL;
                    pOutR[i] += pIn[i] * fPanR;
                }
            }
        }
    }
}

/****************************************** end of file *******************************************/
