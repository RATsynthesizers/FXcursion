/**
 * @file      fx_tremolo.h
 *
 * @details   Tremolo, in two distinct types: mono-only and stereo-only.
 *
 *            They are separate effects, not one effect with a width flag - see
 *            the note at the top of fx_defs.h. Both variants live in fx_tremolo.c
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

#ifndef FX_TREMOLO_H
#define FX_TREMOLO_H



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
/* Bounds set by the note divisions, not by taste - see fx_modulation.h. */
#define TREM_RATE_MIN_HZ            (0.02f)
#define TREM_RATE_MAX_HZ            (55.0f)

/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxTremoloM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxTremoloM_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxTremoloS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxTremoloS_Reset(const U8 nPlaneBase, const U8 nWidth);



#endif // #ifndef FX_TREMOLO_H

/****************************************** end of file *******************************************/
