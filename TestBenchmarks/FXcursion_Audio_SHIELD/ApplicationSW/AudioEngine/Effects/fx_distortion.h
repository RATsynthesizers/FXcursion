/**
 * @file      fx_distortion.h
 *
 * @details   Hard clipping with a tone stack, at twice the sample rate.
 *
 *            ------------------------------------------------------------------
 *            WHY IT OVERSAMPLES AND THE OVERDRIVE DOES NOT
 *            ------------------------------------------------------------------
 *
 *            A hard clipper generates harmonics without limit. At 48 kHz every
 *            one of them above 24 kHz folds back down the spectrum and lands
 *            somewhere unrelated to the note being played - which is why cheap
 *            digital distortion sounds gritty and slightly out of tune on high
 *            notes rather than merely aggressive.
 *
 *            So the clipper runs at 96 kHz: the signal is upsampled, clipped,
 *            band limited and decimated again. Everything that folds now folds
 *            from above 48 kHz, where there is far less of it, and the filter
 *            removes most of that before it can.
 *
 *            The overdrive does not need this. Its curve is smooth, so it
 *            produces a handful of low harmonics that mostly stay in band. Hard
 *            clipping is a corner, and a corner has infinite bandwidth.
 *
 *            Four biquads per sample is the price. There is headroom for it.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_DISTORTION_H
#define FX_DISTORTION_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_common.h"



/***************************************************************************************************
* Parameter ranges
***************************************************************************************************/

/** Gain into the clipper. Exponential - the useful range spans two decades. */
#define DIST_DRIVE_MIN              (1.0f)
#define DIST_DRIVE_MAX              (100.0f)

/** Tone: a one-pole lowpass after the clipper, where a real pedal has one. */
#define DIST_TONE_MIN_HZ            (500.0f)
#define DIST_TONE_MAX_HZ            (12000.0f)

/** Output trim. */
#define DIST_LEVEL_MAX              (2.0f)

/** Stereo only: how differently the two sides clip. */
#define DIST_SPREAD_MAX             (0.40f)

/** Anti-aliasing corner, in the oversampled domain. */
#define DIST_AA_CUTOFF_HZ           (19000.0f)



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxDistortionM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxDistortionM_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxDistortionS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxDistortionS_Reset(const U8 nPlaneBase, const U8 nWidth);



#endif // #ifndef FX_DISTORTION_H

/****************************************** end of file *******************************************/
