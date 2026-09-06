/**
 * @file      fx_compressor.h
 *
 * @details   Feed-forward peak compressor with a log-domain gain computer.
 *
 *            ------------------------------------------------------------------
 *            WHY THE GAIN IS COMPUTED IN dB
 *            ------------------------------------------------------------------
 *
 *            A ratio is a statement about decibels: 4:1 means four dB in, one dB
 *            out. Expressed linearly that is a power law, so the gain computer
 *            works in dB and converts back once per sample.
 *
 *            The conversion is skipped entirely while the signal sits below the
 *            knee, which on guitar material is most of the time - see the
 *            fKneeLowLin short circuit in the implementation.
 *
 *            ------------------------------------------------------------------
 *            LINK, AND WHY ITS DEFAULT MATTERS
 *            ------------------------------------------------------------------
 *
 *            At LINK 1 a single detector fed by max(|L|, |R|) drives one gain
 *            applied to both planes. At LINK 0 the two planes have independent
 *            detectors.
 *
 *            Anything in between makes the stereo image shift left and right in
 *            time with whichever side is louder - the classic dual-mono
 *            compressor fault. It is offered because it is occasionally useful,
 *            but 1.0 is the setting that keeps the image still.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_COMPRESSOR_H
#define FX_COMPRESSOR_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_common.h"



/***************************************************************************************************
* Parameter ranges
***************************************************************************************************/

/*
 * In the header, not the .c, because the GUI has to turn a normalised 0..1 back
 * into "-18.0 dB" and cannot invent the range. See the note in fx_delay.h.
 */

/** Threshold, dB below full scale. */
#define COMP_THRESH_MIN_DB          (-60.0f)
#define COMP_THRESH_MAX_DB          (0.0f)

/** Ratio, n:1. 1.0 is no compression at all, which is a useful bypass. */
#define COMP_RATIO_MIN              (1.0f)
#define COMP_RATIO_MAX              (20.0f)

/** Attack and release, milliseconds. Exponential - linear feels wrong at both ends. */
#define COMP_ATTACK_MIN_MS          (0.1f)
#define COMP_ATTACK_MAX_MS          (100.0f)
#define COMP_RELEASE_MIN_MS         (10.0f)
#define COMP_RELEASE_MAX_MS         (1000.0f)

/** Make-up gain, dB. */
#define COMP_MAKEUP_MAX_DB          (24.0f)

/**
 * Soft knee width, dB, centred on the threshold.
 *
 * Fixed rather than exposed: eight knobs are already spoken for, and a knee is
 * the parameter players are least likely to reach for.
 */
#define COMP_KNEE_DB                (6.0f)



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxCompressorM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxCompressorM_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxCompressorS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxCompressorS_Reset(const U8 nPlaneBase, const U8 nWidth);

/**
 * @brief Gain reduction of the last block, dB (negative), for a GUI meter.
 *
 * @param nPlane  audio plane
 */
extern FLOAT32 FxCompressor_GainReductionDb(const U8 nPlane);



#endif // #ifndef FX_COMPRESSOR_H

/****************************************** end of file *******************************************/
