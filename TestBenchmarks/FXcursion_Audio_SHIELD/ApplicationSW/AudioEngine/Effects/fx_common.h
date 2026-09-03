/**
 * @file      fx_common.h
 *
 * @details   The contract every effect implements, plus the small helpers they
 *            all need.
 *
 *            ------------------------------------------------------------------
 *            THE PROCESSING CONTRACT - read this before writing an effect
 *            ------------------------------------------------------------------
 *
 *            void Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], U16 nFrames)
 *
 *            * PROCESSING IS IN PLACE. apPlane[p] holds nFrames input samples on
 *              entry and must hold nFrames output samples on exit. You may use
 *              it as scratch. You must NOT read any buffer you do not own, and
 *              you must NOT reach through a "previous module" pointer - there
 *              are no module pointers in this design.
 *
 *            * BUFFERS ARE PLANAR. apPlane[0] is the left/mono plane, apPlane[1]
 *              the right plane of a stereo chain. Each plane is a contiguous
 *              array of nFrames FLOAT32, nominal range -1.0f .. +1.0f. Planar,
 *              not interleaved, so mono effects are just "run the loop twice"
 *              and CMSIS-DSP block functions are usable directly.
 *
 *            * AN EFFECT IS MONO-ONLY OR STEREO-ONLY. There are no
 *              width-agnostic effects. g_aFxDesc[type].nWidth is 1 or 2 and the
 *              grid guarantees pCtx->nWidth equals it, so your process function
 *              does not branch on width at all - a mono effect just uses
 *              apPlane[0], a stereo effect uses apPlane[0] and apPlane[1].
 *
 *              A conceptual effect that exists in both forms is TWO entries in
 *              the registry, written in one .c file sharing a core and sharing
 *              their static memory. Sharing memory is safe because a chain is
 *              either mono or stereo, so the two variants can never be live on
 *              the same planes: a stereo instance at plane base p uses exactly
 *              the entries two mono instances on planes p and p+1 would have.
 *
 *              The stereo variant must differ by at least one PARAMETER, not
 *              just by running twice - Ping-pong on a delay, Link on a
 *              compressor, Phase on a tremolo, Width on a reverb, Spread on the
 *              modulation effects. If a stereo variant would be identical to the
 *              mono one run twice, it should not exist.
 *
 *            * THERE IS NO INSTANCE OBJECT. An effect "instance" is identified
 *              entirely by (chain, plane, FX_TYPE):
 *                  - its state lives in a static array in its own .c file,
 *                    indexed by plane;
 *                  - its parameters live in the global parameter array, indexed
 *                    by (chain, FX_TYPE).
 *              Nothing is allocated, nothing is constructed, and there is no
 *              pointer that can dangle. Adding or removing an effect from a
 *              chain changes one byte in a table.
 *
 *            * NO HAL. Nothing under Modules/ may include a HAL header, so the
 *              whole DSP layer builds and runs on a PC. See Test/.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_COMMON_H
#define FX_COMMON_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"
#include "fx_defs.h"



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief Everything an effect is told about the context it is running in.
 */
typedef struct stFX_CTX
{
    U8              nChain;         /**< 0..CHAIN_MAX_QTY-1, selects parameters  */
    U8              nPlaneBase;     /**< 0..AUDIO_PLANE_QTY-1, selects state     */
    U8              nWidth;         /**< 1 or 2                                  */
    U8              nReserved;

    const FX_PARAM* pParam;         /**< FX_PARAM_QTY entries for this instance  */
    const TEMPO*    pTempo;         /**< global tempo, for synced parameters     */

} FX_CTX;

/**
 * @brief Process one block, in place. See the contract at the top of this file.
 */
typedef void (*FX_PROCESS_FN)(const FX_CTX* pCtx,
                              FLOAT32* const apPlane[],
                              const U16 nFrames);

/**
 * @brief Clear the state of the given planes.
 *
 * Called when an effect is added to a chain, so it never starts with stale
 * audio from a previous configuration. Must be safe to call at any time and
 * must not depend on parameters.
 */
typedef void (*FX_RESET_FN)(const U8 nPlaneBase, const U8 nWidth);

/**
 * @brief Registry entry for one effect type. One per FX_TYPE, in flash.
 */
typedef struct stFX_ENTRY
{
    FX_PROCESS_FN pfProcess;
    FX_RESET_FN   pfReset;

} FX_ENTRY;



/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/**
 * @brief The effect registry. Indexed by FX_TYPE. Lives in flash.
 *
 * This is the entire dispatch mechanism: no vtables, no per-instance function
 * pointers. Adding an effect means writing its .c file and adding one row here.
 */
extern const FX_ENTRY g_aFxEntry[FX_TYPE_QTY];



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

// ------------------------------------------------------------------------------------------------
// Parameter mapping
//
// Parameters arrive normalised 0..1. The mapping to seconds / Hz / dB belongs to
// the effect that owns the parameter, never to the GUI - so that adding an
// effect never requires a change on the interface side.
// ------------------------------------------------------------------------------------------------

/** Raw normalised value, clamped to 0..1. */
extern FLOAT32 FxParam_Norm(const FX_PARAM* const pParam);

/** Linear map of the normalised value onto [fMin ; fMax]. */
extern FLOAT32 FxParam_Lin(const FX_PARAM* const pParam,
                           const FLOAT32 fMin,
                           const FLOAT32 fMax);

/** Exponential map onto [fMin ; fMax]. Use for frequencies and times. fMin > 0. */
extern FLOAT32 FxParam_Exp(const FX_PARAM* const pParam,
                           const FLOAT32 fMin,
                           const FLOAT32 fMax);

/**
 * @brief Map a time-like parameter, honouring tempo sync.
 *
 * Free mode:  exponential map onto [fMinSec ; fMaxSec].
 * Sync mode:  the note division resolved against the current tempo, then
 *             clamped to [fMinSec ; fMaxSec].
 */
extern FLOAT32 FxParam_TimeSec(const FX_PARAM* const pParam,
                               const TEMPO* const pTempo,
                               const FLOAT32 fMinSec,
                               const FLOAT32 fMaxSec);

/**
 * @brief Map a rate-like parameter, honouring tempo sync.
 *
 * Sync mode returns the reciprocal of the resolved note duration, so a 1/4
 * division at 120 BPM gives 2.0 Hz.
 */
extern FLOAT32 FxParam_RateHz(const FX_PARAM* const pParam,
                              const TEMPO* const pTempo,
                              const FLOAT32 fMinHz,
                              const FLOAT32 fMaxHz);

// ------------------------------------------------------------------------------------------------
// Tempo
// ------------------------------------------------------------------------------------------------

/** Duration of one quarter note, seconds. */
extern FLOAT32 Tempo_QuarterSec(const TEMPO* const pTempo);

/**
 * @brief Duration of one bar, seconds.
 *
 * Bar = nBeatsPerBar * (4 / nBeatUnit) quarter notes.
 * 4/4 at 120 BPM -> 2.00 s.  6/8 at 120 BPM -> 1.50 s.
 */
extern FLOAT32 Tempo_BarSec(const TEMPO* const pTempo);

// ------------------------------------------------------------------------------------------------
// Small DSP helpers
// ------------------------------------------------------------------------------------------------

/** Clamp to [fMin ; fMax]. */
extern FLOAT32 FxUtil_Clamp(const FLOAT32 fValue, const FLOAT32 fMin, const FLOAT32 fMax);

/**
 * @brief One-pole smoothing coefficient for a given time constant.
 *
 * Apply as:  fState += fCoeff * (fTarget - fState);  once per block.
 * Every user-facing gain must go through this or it will zipper-click.
 */
extern FLOAT32 FxUtil_SmoothCoeff(const FLOAT32 fTimeConstMs, const U16 nFrames);

/** Cubic soft clip, unity slope at zero, hard limited to +/-1. */
extern FLOAT32 FxUtil_SoftClip(const FLOAT32 fValue);

/**
 * @brief Constant-power pan weights.
 *
 * fPan -1.0 = hard left, 0.0 = centre, +1.0 = hard right.
 * Centre gives 0.707 on both sides, so panning does not change perceived level.
 */
extern void FxUtil_PanGains(const FLOAT32 fPan, FLOAT32* const pLeft, FLOAT32* const pRight);

/**
 * @brief Read a delay line at a fractional offset, linear interpolation.
 *
 * The delay is passed SPLIT into an integer and a fractional part on purpose.
 * A FLOAT32 has a 24-bit mantissa, so a delay of 192000 frames (4 s at 48 kHz)
 * would only resolve to about 1/64 of a frame, and a long delay whose time is
 * being swept would sound gritty and quantised. Keeping the integer part in a
 * U32 makes the fractional part exact at any delay length.
 *
 * @param pLine     circular buffer base
 * @param nLineLen  buffer length in frames, must be >= 2
 * @param nWritePos current write index, 0..nLineLen-1
 * @param nIntDelay integer part of the delay in frames, 1..nLineLen-2
 * @param fFrac     fractional part, 0.0f .. 1.0f
 */
extern FLOAT32 FxUtil_DelayRead(const FLOAT32* const pLine,
                                const U32 nLineLen,
                                const U32 nWritePos,
                                const U32 nIntDelay,
                                const FLOAT32 fFrac);



#endif // #ifndef FX_COMMON_H

/****************************************** end of file *******************************************/
