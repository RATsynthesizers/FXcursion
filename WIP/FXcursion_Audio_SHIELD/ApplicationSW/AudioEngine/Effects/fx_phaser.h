/**
 * @file      fx_phaser.h
 *
 * @details   Cascaded first-order allpasses with an LFO-swept corner.
 *
 *            An allpass passes every frequency at full level but delays each by
 *            a different amount. Sum that with the dry signal and the places
 *            where the two are half a cycle apart cancel - a comb of notches.
 *            Sweep the allpass corner and the notches sweep with it, which is
 *            the sound.
 *
 *            It is NOT a flanger. A flanger's notches are evenly spaced because
 *            they come from a fixed delay; a phaser's are not, because they come
 *            from phase, and that irregular spacing is what makes it sound
 *            liquid rather than metallic.
 *
 *            STAGES is FX_PF_STEPPED and quantises to 2, 4, 6 or 8. Each PAIR of
 *            stages adds one notch, so the number is audible in steps and a
 *            continuous knob would be a lie.
 *
 *            No delay line at all - a handful of floats per plane.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_PHASER_H
#define FX_PHASER_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_common.h"



/***************************************************************************************************
* Parameter ranges
***************************************************************************************************/

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
#define PHASER_RATE_MIN_HZ          (0.02f)
#define PHASER_RATE_MAX_HZ          (55.0f)

/**
 * Where the sweep lives, as a CORNER FREQUENCY.
 *
 * Sweeping the raw coefficient instead put the effect in the wrong place: at
 * half depth it only reached 0.58, which is a corner of about 4 kHz, so the
 * whole effect happened between 4 and 9 kHz. Up there a phaser is thin and
 * reads as just another comb - which is why it was hard to tell from the
 * flanger.
 *
 * A phaser belongs in the mids. Depth widens the sweep upwards from the bottom,
 * so turning it down parks the notches low rather than leaving them stranded in
 * the treble.
 */
#define PHASER_FC_MIN_HZ            (200.0f)
#define PHASER_FC_MAX_HZ            (3000.0f)

/**
 * Per-stage multiplier on that corner.
 *
 * A cascade of IDENTICAL allpasses puts its notches in a fixed ratio, which is
 * a comb by another name. Staggering the stages geometrically is what gives a
 * phaser its irregular, liquid spacing - and it is the single biggest reason
 * the first version sounded like a flanger.
 */
#define PHASER_STAGE_SPREAD         { 1.00f, 1.33f, 1.78f, 2.37f, \
                                      3.16f, 4.21f, 5.62f, 7.49f }

/** Regeneration around the cascade. Sharpens the notches. */
#define PHASER_FEEDBACK_MAX         (0.85f)

/** Stages actually available, always even. */
#define PHASER_STAGE_QTY            (8U)
#define PHASER_STAGE_STEPS          (4U)        /* 2, 4, 6, 8 */



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxPhaserM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxPhaserM_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxPhaserS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxPhaserS_Reset(const U8 nPlaneBase, const U8 nWidth);



#endif // #ifndef FX_PHASER_H

/****************************************** end of file *******************************************/
