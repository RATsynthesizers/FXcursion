/**
 * @file      fx_template.c
 *
 * @details   COPY THIS FILE TO START A NEW EFFECT.
 *
 *            It is a working, compilable effect - a one-pole tone control
 *            followed by a gain - written to show every part of the contract in
 *            one place. It is deliberately NOT in the registry, so it never
 *            runs; it exists to be copied.
 *
 *            ==================================================================
 *            CHECKLIST FOR ADDING AN EFFECT
 *            ==================================================================
 *
 *            1. Copy fx_template.c and fx_template.h, rename FxTemplate_* to
 *               FxMyEffect_*.
 *
 *            2. DECIDE THE TWO VARIANTS FIRST. Every effect is mono-only or
 *               stereo-only, and a conceptual effect that exists in both is two
 *               entries. Before writing any DSP, answer: what does the stereo
 *               variant have that the mono one cannot? If the honest answer is
 *               "nothing, it just runs twice", then only write the mono one.
 *
 *            3. In InterComProtocol/fx_defs.h:
 *                 - append FX_MYEFFECT_M and FX_MYEFFECT_S to FX_TYPE, in that
 *                   order, mono at an EVEN id (FX_VARIANT_FOR_WIDTH depends on
 *                   it), BEFORE FX_TYPE_QTY and WITHOUT renumbering anything
 *                   (the ids are in the wire protocol and in saved presets);
 *                 - bump FX_TYPE_QTY by two;
 *                 - add both parameter index enums.
 *
 *            4. In InterComProtocol/fx_defs.c: add both parameter descriptor arrays and
 *               both rows in g_aFxDesc, with nWidth 1 and 2. Give the two rows
 *               the SAME display name - the GUI shows one entry and resolves the
 *               variant from the chain's width. Mark time-like and rate-like
 *               parameters FX_PF_SYNCABLE so the GUI offers a division picker.
 *
 *            5. COPY BOTH fx_defs FILES TO THE INTERFACE PROJECT. They are
 *               duplicated on purpose and must stay byte-identical.
 *
 *            6. In Modules/params.c: add two rows to aDefaultNorm.
 *
 *            7. In Modules/Effects/fx_registry.c: add two rows.
 *
 *            8. If the effect needs a delay line, declare it here as ONE static
 *               array indexed by plane, using a section macro from mem_map.h,
 *               and let both variants share it - see fx_delay.c. Add its size to
 *               the budget in mem_map.h so the static assertion keeps covering
 *               the real footprint.
 *
 *            9. Add a test to Test/.
 *
 *            ==================================================================
 *            THE FIVE RULES
 *            ==================================================================
 *
 *            1. PROCESS IN PLACE. apPlane[p] holds the input on entry and must
 *               hold the output on exit.
 *
 *            2. STATE IS INDEXED BY PLANE, PARAMETERS BY CHAIN. There is no
 *               instance object and nothing is allocated. pCtx tells you which
 *               of each to use.
 *
 *            3. KNOW YOUR WIDTH AT COMPILE TIME. A mono effect touches apPlane[0]
 *               and nothing else; a stereo effect touches apPlane[0] and
 *               apPlane[1]. The grid guarantees the chain matches, so there is
 *               no runtime width branch to write.
 *
 *            4. RESOLVE PARAMETERS ONCE PER BLOCK, not per sample. The compiler
 *               can then keep them in registers across the whole loop, which is
 *               most of the reason block processing is fast.
 *
 *            5. SMOOTH ANYTHING THE USER CAN TURN that multiplies the signal.
 *               An un-smoothed gain change is an audible click.
 *
 *            And one prohibition: NO HAL, NO malloc, NO printf, NO floating
 *            point literals without an 'f'. Build with -Wdouble-promotion.
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

#include "fx_template.h"

#include "mem_map.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/* Parameter indices. When this becomes a real effect these move to fx_defs.h so
 * the interface controller can see them too. */
#define TEMPLATE_P_TONE             (0U)
#define TEMPLATE_P_GAIN             (1U)

#define TEMPLATE_TONE_MIN_HZ        (200.0f)
#define TEMPLATE_TONE_MAX_HZ        (16000.0f)

#define TEMPLATE_GAIN_MAX           (2.0f)

#define TEMPLATE_SMOOTH_MS          (15.0f)

#define FX_TWO_PI                   (6.28318531f)



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

/**
 * @brief Everything this effect remembers about ONE PLANE.
 *
 * One entry per plane, never per instance. A mono effect running on a stereo
 * chain simply gets two entries and runs its algorithm twice.
 */
typedef struct stTEMPLATE_STATE
{
    FLOAT32 fLowpass;           /**< one-pole filter memory                      */
    FLOAT32 fCurGain;           /**< smoothed gain; negative means "snap"        */

} TEMPLATE_STATE;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Static, indexed by plane. This is the whole storage model.
 *
 * Small hot state goes in DTCM. If this effect needed a delay line, it would be
 * declared here too - IN_SRAM_FAST for anything up to a few tens of
 * milliseconds, IN_SDRAM_DELAY for anything longer. See mem_map.h for why the two
 * tiers exist.
 */
static TEMPLATE_STATE aState[AUDIO_PLANE_QTY] IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Helpers are static and take plain arguments. Keep them out of the
 *        per-sample loop unless they will inline.
 */
static FLOAT32 OnePoleCoeff(const FLOAT32 fCutoffHz)
{
    return FxUtil_Clamp(1.0f - expf(-FX_TWO_PI * fCutoffHz / (FLOAT32)AUDIO_SAMPLE_RATE_HZ),
                        0.0f, 1.0f);
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

/**
 * @brief Clear the state of the given planes.
 *
 * Called from the SUPER-LOOP when this effect is added to a chain, never from
 * the audio ISR - so it is allowed to be slow. FxDelay_Reset clears 750 KiB per
 * plane here, which takes milliseconds and would be catastrophic in the ISR.
 *
 * Must not depend on parameters: it can be called before they are meaningful.
 */
void FxTemplate_Reset(const U8 nPlaneBase, const U8 nWidth)
{
    U8 p;

    for (p = 0U; p < nWidth; p++)
    {
        const U8 nPlane = nPlaneBase + p;

        if (nPlane < AUDIO_PLANE_QTY)
        {
            aState[nPlane].fLowpass = 0.0f;
            aState[nPlane].fCurGain = -1.0f;        // snap on the first block
        }
    }
}

//--------------------------------------------------------------------------------------------------

void FxTemplate_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames)
{
    /* ---- 1. Resolve parameters ONCE, before the sample loop ------------------------------- */

    const FLOAT32 fToneHz  = FxParam_Exp(&pCtx->pParam[TEMPLATE_P_TONE],
                                         TEMPLATE_TONE_MIN_HZ, TEMPLATE_TONE_MAX_HZ);

    const FLOAT32 fGain    = FxParam_Lin(&pCtx->pParam[TEMPLATE_P_GAIN],
                                         0.0f, TEMPLATE_GAIN_MAX);

    const FLOAT32 fCoeff   = OnePoleCoeff(fToneHz);
    const FLOAT32 fSmooth  = FxUtil_SmoothCoeff(TEMPLATE_SMOOTH_MS, nFrames);

    U8 p;

    /* ---- 2. Loop over the planes this chain actually has ---------------------------------- */

    for (p = 0U; p < pCtx->nWidth; p++)
    {
        const U8              nPlane = pCtx->nPlaneBase + p;
        TEMPLATE_STATE* const pSt    = &aState[nPlane];
        FLOAT32* const        pBuf   = apPlane[p];

        FLOAT32 fLp;
        FLOAT32 fCurGain;
        U16     i;

        if (nPlane >= AUDIO_PLANE_QTY)
        {
            break;                                  // cannot happen; cheap insurance
        }

        /* ---- 3. Smooth anything the user can turn ----------------------------------------- */

        if (pSt->fCurGain < 0.0f)
        {
            pSt->fCurGain = fGain;
        }
        else
        {
            pSt->fCurGain += fSmooth * (fGain - pSt->fCurGain);
        }

        /* ---- 4. Pull state into locals so it stays in registers ---------------------------- */

        fLp      = pSt->fLowpass;
        fCurGain = pSt->fCurGain;

        for (i = 0U; i < nFrames; i++)
        {
            fLp += fCoeff * (pBuf[i] - fLp);        // one-pole lowpass
            pBuf[i] = fLp * fCurGain;               // IN PLACE - see rule 1
        }

        /* ---- 5. Write state back ---------------------------------------------------------- */

        pSt->fLowpass = fLp;
    }
}

/****************************************** end of file *******************************************/
