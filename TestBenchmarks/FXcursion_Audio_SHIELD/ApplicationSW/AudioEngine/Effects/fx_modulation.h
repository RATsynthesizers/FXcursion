/**
 * @file      fx_modulation.h
 *
 * @details   Chorus, flanger and vibrato.
 *
 *            ------------------------------------------------------------------
 *            WHY THESE THREE SHARE A FILE
 *            ------------------------------------------------------------------
 *
 *            They are one structure with three voicings:
 *
 *                in -> short delay line, tap position swept by an LFO -> mix
 *
 *            What separates them is the delay range, whether the output is fed
 *            back, and how much dry survives:
 *
 *              Chorus    5 to 25 ms, no feedback, blended with the dry. Long
 *                        enough that the pitch wobble reads as a second player
 *                        rather than as detuning.
 *
 *              Flanger   under 8 ms, WITH feedback. Short enough that the dry
 *                        and the delayed copy comb-filter each other, and the
 *                        feedback is what turns that comb into the jet whoosh.
 *
 *              Vibrato   no dry path at all. Vibrato is pitch modulation by
 *                        definition, and any dry signal left in turns it back
 *                        into a chorus.
 *
 *            Written once, three times over, they would drift apart the first
 *            time the LFO or the fractional tap was touched.
 *
 *            ------------------------------------------------------------------
 *            SPREAD
 *            ------------------------------------------------------------------
 *
 *            The stereo variants add SPREAD: the LFO phase offset between the
 *            two planes. At 0 both sides sweep together, which is a dual-mono
 *            effect in two speakers. At 1 they sweep in antiphase, and that
 *            difference between the sides is the whole reason a stereo chorus
 *            sounds wide instead of merely doubled.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_MODULATION_H
#define FX_MODULATION_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_common.h"



/***************************************************************************************************
* Parameter ranges
***************************************************************************************************/

/*
 * In the header so the GUI can turn a normalised 0..1 into "1.20 Hz". See the
 * note in fx_delay.h.
 */

/**
 * LFO rate. FX_PF_SYNCABLE, and the bounds are set BY that.
 *
 * The free range follows the tempo - it spans exactly what 1/1 and 1/32 give at
 * the current BPM - so these hard limits exist only to stop the clamp biting
 * inside the legal tempo range. They are not musical choices:
 *
 *     1/1  at  20 BPM  =  12 s     ->  0.083 Hz
 *     1/32 at 400 BPM  =  18.75 ms ->  53.3 Hz
 *
 * Set narrower than that and the top of the knob can never reach the fastest
 * division: at 8 Hz the free maximum came out as a 1/16 note while sync went to
 * 1/32, which is precisely the mismatch these bounds are meant to prevent.
 */
#define MOD_RATE_MIN_HZ             (0.02f)
#define MOD_RATE_MAX_HZ             (55.0f)

/**
 * Chorus: the DELAY knob sets the centre, depth sweeps around it.
 *
 * The two that decide whether the tap can run off the end of the line are given
 * as whole milliseconds first and converted, because the check below is a
 * preprocessor one and the preprocessor cannot evaluate a float.
 */
#define CHORUS_DELAY_MIN_MS         (8.0f)
#define CHORUS_DELAY_MAX_MS_WHOLE   (16U)
#define CHORUS_SWEEP_MS_WHOLE       (4U)
#define CHORUS_DELAY_MAX_MS         ((FLOAT32)CHORUS_DELAY_MAX_MS_WHOLE)
#define CHORUS_SWEEP_MS             ((FLOAT32)CHORUS_SWEEP_MS_WHOLE)

/**
 * Voices, and this is what makes a chorus a chorus.
 *
 * ONE delayed copy mixed with the dry is a comb filter - which is a flanger,
 * however long the delay. Measured side by side the two were the same effect at
 * different notch spacings, and they sounded it.
 *
 * Three copies at evenly spread LFO phases have combs that do not line up, so
 * each one's notches sit on the others' peaks and the comb largely fills in.
 * What is left is the pitch shimmer of three slightly detuned copies, which is
 * the sound, and it is nothing like a flanger.
 */
#define CHORUS_VOICE_QTY            (3U)

/**
 * Per-voice multiplier on the base delay, so the three are not merely phase
 * shifted but genuinely different lengths.
 */
#define CHORUS_VOICE_SPREAD         { 1.00f, 1.18f, 1.37f }

/* Three decorrelated voices sum as the root of three, not as three. */
#define CHORUS_VOICE_NORM           (0.57735027f)

/* The longest voice multiplier, as a percentage, for the check below. */
#define CHORUS_VOICE_SPREAD_PCT     (137U)

/** Flanger: short enough for the comb to be the point. */
#define FLANGER_DELAY_MIN_MS        (0.6f)
#define FLANGER_SWEEP_MS            (7.0f)
#define FLANGER_FEEDBACK_MAX        (0.90f)

/** Vibrato: pitch only. */
#define VIBRATO_DELAY_MID_MS        (3.0f)
#define VIBRATO_SWEEP_MS            (2.4f)

/* Every tap has to stay inside the shared modulation line, and the LONGEST
 * voice is the one that decides that. */
#if ((((CHORUS_DELAY_MAX_MS_WHOLE * CHORUS_VOICE_SPREAD_PCT) / 100U) + \
      CHORUS_SWEEP_MS_WHOLE) >= MODDELAY_MAX_MS)
#error "Chorus can sweep past the end of the modulation delay line"
#endif



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxChorusM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxChorusM_Reset(const U8 nPlaneBase, const U8 nWidth);
extern void FxChorusS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxChorusS_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxFlangerM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxFlangerM_Reset(const U8 nPlaneBase, const U8 nWidth);
extern void FxFlangerS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxFlangerS_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxVibratoM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxVibratoM_Reset(const U8 nPlaneBase, const U8 nWidth);
extern void FxVibratoS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxVibratoS_Reset(const U8 nPlaneBase, const U8 nWidth);



#endif // #ifndef FX_MODULATION_H

/****************************************** end of file *******************************************/
