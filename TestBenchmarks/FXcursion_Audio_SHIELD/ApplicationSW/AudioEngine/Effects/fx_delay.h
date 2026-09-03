/**
 * @file      fx_delay.h
 *
 * @details   Delay, in two distinct types: mono-only and stereo-only.
 *
 *            They are separate effects, not one effect with a width flag - see
 *            the note at the top of fx_defs.h. Both variants live in fx_delay.c
 *            and share their core and their static memory, because a mono and a
 *            stereo instance can never occupy the same planes.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - 2.0.0 - mono and stereo split
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_DELAY_H
#define FX_DELAY_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_common.h"



/***************************************************************************************************
* Parameter ranges
***************************************************************************************************/

/*
 * These live in the HEADER, not in the .c, because they are part of the
 * effect's contract with whatever is drawing the knobs.
 *
 * A parameter is stored normalised, 0..1. That is the right thing on the wire
 * and the wrong thing on a screen: "Time 0.700" tells a player nothing. Turning
 * it back into 470 ms needs the range and the mapping, and if the GUI keeps its
 * own copy of those numbers they drift the first time an effect is retuned.
 *
 * So there is one definition, here, and both the pedal's display and the VST
 * bench read it.
 */
#define DELAY_TIME_MIN_SEC          (0.020f)
#define DELAY_TIME_MAX_SEC          ((FLOAT32)DELAY_MAX_SEC)
#define DELAY_FEEDBACK_MAX          (0.98f)
#define DELAY_TONE_MIN_HZ           (600.0f)
#define DELAY_TONE_MAX_HZ           (12000.0f)
#define DELAY_SPREAD_MAX_RATIO      (2.0f)

/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxDelayM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxDelayM_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxDelayS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxDelayS_Reset(const U8 nPlaneBase, const U8 nWidth);



#endif // #ifndef FX_DELAY_H

/****************************************** end of file *******************************************/
