/**
 * @file      fx_reverb.h
 *
 * @details   Four-line feedback delay network with a Hadamard mixing matrix.
 *
 *            ------------------------------------------------------------------
 *            THE STRUCTURE
 *            ------------------------------------------------------------------
 *
 *              in -> pre-delay -> 4 diffusion allpasses -> FDN -> mix
 *
 *            The FDN is four delay lines of mutually prime length. Each is read,
 *            low-pass filtered, and the four results are mixed by a normalised
 *            Hadamard matrix before being written back with a per-line decay
 *            gain. The matrix is orthonormal, so it moves energy between the
 *            lines without creating or destroying any: everything that decides
 *            how long the tail lasts is in the gains, which makes the decay time
 *            an actual number rather than something to tune by ear.
 *
 *            The damping filter sits INSIDE the feedback path, so each pass
 *            round the loop is a little darker than the last. That is what makes
 *            a tail sound like a room rather than like a delay.
 *
 *            ------------------------------------------------------------------
 *            THE STEREO VARIANT IS A DIFFERENT ALGORITHM
 *            ------------------------------------------------------------------
 *
 *            It is NOT the mono one run twice. Two independent reverbs sound
 *            like two rooms, and on a guitar that is immediately obvious.
 *
 *            There is ONE tail. Both inputs are summed into a single FDN and two
 *            DIFFERENT combinations of its four lines are taken out - lines 0
 *            and 2 to the left, 1 and 3 to the right. Because the lines have
 *            different lengths those two outputs are decorrelated, which is
 *            what a real room does and what makes a stereo tail wide instead of
 *            merely loud.
 *
 *            ------------------------------------------------------------------
 *            DENORMALS
 *            ------------------------------------------------------------------
 *
 *            A reverb tail decays smoothly towards zero, so it spends its last
 *            second in the range where denormal floats live, and denormals can
 *            cost 10 to 100 times a normal operation.
 *
 *            AudioIO_Init sets FPSCR.FZ so the hardware flushes them. The
 *            explicit squash in the damping state costs one compare per line and
 *            makes the effect behave the same on a host build that has no such
 *            control - worth having in code that is auditioned on a PC.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_REVERB_H
#define FX_REVERB_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_common.h"



/***************************************************************************************************
* Parameter ranges
***************************************************************************************************/

/*
 * In the header so the GUI can turn a normalised 0..1 into "2.40 s". See the
 * note in fx_delay.h.
 */

/**
 * Pre-delay. FX_PF_SYNCABLE, so it also resolves against the tempo.
 *
 * The maximum is stated in whole milliseconds as well, because the buffer
 * length has to be an INTEGER constant expression - a length computed from a
 * float is not one, and the compile-time check that it fits its bank quietly
 * turns into a variable-length array instead.
 */
#define REV_PREDELAY_MIN_SEC        (0.001f)
#define REV_PREDELAY_MAX_MS         (500UL)
#define REV_PREDELAY_MAX_SEC        (0.500f)

/**
 * Room size: the fraction of each network line that is actually used.
 *
 * Independent of decay. Size sets how far apart the reflections are - a small
 * bright room against a big hall - while decay sets how long they go on for.
 * Below about a quarter the lines get short enough to colour the tail.
 */
#define REV_SIZE_MIN                (0.25f)

/** RT60, the time for the tail to fall by 60 dB. */
#define REV_DECAY_MIN_SEC           (0.20f)
#define REV_DECAY_MAX_SEC           (12.0f)

/**
 * Damping corner. The knob runs the other way - 0 is bright, 1 is dark - so
 * this is the cutoff at each end, not the value the knob maps onto.
 */
#define REV_DAMP_BRIGHT_HZ          (18000.0f)
#define REV_DAMP_DARK_HZ            (1200.0f)

/**
 * Allpass coefficient. Above about 0.75 the diffusers start to ring on their
 * own instead of smearing what goes through them.
 */
#define REV_DIFFUSION_MAX           (0.70f)

/** Stereo width. 0 collapses the tail to the centre, 1 is natural. */
#define REV_WIDTH_MAX               (2.00f)



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxReverbM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxReverbM_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxReverbS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxReverbS_Reset(const U8 nPlaneBase, const U8 nWidth);



#endif // #ifndef FX_REVERB_H

/****************************************** end of file *******************************************/
