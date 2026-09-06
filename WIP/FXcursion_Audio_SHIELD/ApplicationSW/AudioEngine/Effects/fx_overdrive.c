/**
 * @file      fx_overdrive.c
 *
 * @details   Asymmetric soft-clipping overdrive, mono and stereo.
 *
 *            MONO    Drive, Bias, Level, Mix.
 *            STEREO  the same, plus Spread - the two planes are driven slightly
 *                    differently, which is how a stereo drive gets width instead
 *                    of just being the same distortion twice.
 *
 *            This is a port of the old C++ Overdrive::process, with three real
 *            bugs fixed. They are worth naming because they are the kind that
 *            survive a language change untouched:
 *
 *            1. DOUBLE PROMOTION.  The original computed
 *                   prevModule->output[lr] * (drive->val + 0.1) * 10.0
 *               where 0.1 and 10.0 are DOUBLE literals, so the whole expression
 *               was evaluated in double precision on every sample. The FPU
 *               supports it, but it is several times slower than single and
 *               blocks vectorisation. Every literal here carries an 'f'.
 *               Build with -Wdouble-promotion and this class of bug cannot
 *               come back.
 *
 *            2. std::abs ON A FLOAT.  The original called std::abs with only
 *               <math.h> and <limits> included. It happened to work because
 *               libstdc++'s <math.h> pulls in <cmath>. The C equivalent, abs()
 *               from <stdlib.h>, takes an int and would silently truncate the
 *               sample to zero. Use fabsf.
 *
 *            3. A LOCAL ARRAY NAMED abs.  The original declared
 *               "float abs[STEREO]" in the same function that called std::abs.
 *               Legal in C++ because the call was qualified; a hard conflict in C.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - 2.0.0 - mono and stereo split, Spread added
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_overdrive.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/




/** At Spread 1.0 the two planes differ by this much drive, either side of centre. */



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/** Waveshape one plane. Memoryless, so there is no state to index. */
static void ShapePlane(FLOAT32* const pBuf,
                       const U16 nFrames,
                       const FLOAT32 fDrive,
                       const FLOAT32 fBias,
                       const FLOAT32 fLevel,
                       const FLOAT32 fMix)
{
    const FLOAT32 fDry = 1.0f - fMix;
    U16           i;

    for (i = 0U; i < nFrames; i++)
    {
        const FLOAT32 fIn  = pBuf[i];
        FLOAT32       fVal = fIn * fDrive;

        // fabsf, not abs. See the header comment.
        if (fabsf(fVal) > fBias)
        {
            // Hard region: clamp to the bias level, keeping the sign.
            fVal = (fVal > 0.0f) ? fBias : -fBias;
        }
        else
        {
            // Soft region: cubic transfer curve, same shape as the original.
            fVal = (fVal * (3.0f - ((fVal * fVal) * 2.0f))) * (1.0f / 3.0f);
        }

        fVal *= fLevel;
        fVal  = FxUtil_Clamp(fVal, -1.0f, 1.0f);

        pBuf[i] = (fVal * fMix) + (fIn * fDry);
    }
}



/***************************************************************************************************
* Definitions of global (public) functions - MONO
***************************************************************************************************/

void FxOverdriveM_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    // Memoryless waveshaper: nothing to clear.
    (void)nPlaneBase;
    (void)nWidth;
}

//--------------------------------------------------------------------------------------------------

void FxOverdriveM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    ShapePlane(apPlane[0], nFrames,
               FxParam_Exp(&pCtx->pParam[FX_ODM_P_DRIVE], OD_DRIVE_MIN, OD_DRIVE_MAX),
               FxParam_Lin(&pCtx->pParam[FX_ODM_P_BIAS],  OD_BIAS_MIN,  OD_BIAS_MAX),
               FxParam_Lin(&pCtx->pParam[FX_ODM_P_LEVEL], 0.0f,         OD_LEVEL_MAX),
               FxParam_Norm(&pCtx->pParam[FX_ODM_P_MIX]));
}



/***************************************************************************************************
* Definitions of global (public) functions - STEREO
***************************************************************************************************/

void FxOverdriveS_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    (void)nPlaneBase;
    (void)nWidth;
}

//--------------------------------------------------------------------------------------------------

void FxOverdriveS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    const FLOAT32 fDrive  = FxParam_Exp(&pCtx->pParam[FX_ODS_P_DRIVE],
                                        OD_DRIVE_MIN, OD_DRIVE_MAX);
    const FLOAT32 fBias   = FxParam_Lin(&pCtx->pParam[FX_ODS_P_BIAS],
                                        OD_BIAS_MIN, OD_BIAS_MAX);
    const FLOAT32 fLevel  = FxParam_Lin(&pCtx->pParam[FX_ODS_P_LEVEL], 0.0f, OD_LEVEL_MAX);
    const FLOAT32 fMix    = FxParam_Norm(&pCtx->pParam[FX_ODS_P_MIX]);
    const FLOAT32 fSpread = FxParam_Norm(&pCtx->pParam[FX_ODS_P_SPREAD]) * OD_SPREAD_MAX;

    // Left a little cleaner, right a little hotter. At Spread 0 the two planes
    // are identical and this is a dual-mono drive.
    ShapePlane(apPlane[0], nFrames, fDrive * (1.0f - fSpread), fBias, fLevel, fMix);
    ShapePlane(apPlane[1], nFrames, fDrive * (1.0f + fSpread), fBias, fLevel, fMix);
}

/****************************************** end of file *******************************************/
