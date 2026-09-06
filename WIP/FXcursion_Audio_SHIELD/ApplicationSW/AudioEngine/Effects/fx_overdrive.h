/**
 * @file      fx_overdrive.h
 *
 * @details   Overdrive, in two distinct types: mono-only and stereo-only.
 *
 *            They are separate effects, not one effect with a width flag - see
 *            the note at the top of fx_defs.h. Both variants live in fx_overdrive.c
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

#ifndef FX_OVERDRIVE_H
#define FX_OVERDRIVE_H



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
#define OD_DRIVE_MIN                (1.0f)
#define OD_DRIVE_MAX                (30.0f)
#define OD_BIAS_MIN                 (0.30f)
#define OD_BIAS_MAX                 (1.00f)
#define OD_LEVEL_MAX                (2.0f)
#define OD_SPREAD_MAX               (0.35f)

/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

extern void FxOverdriveM_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxOverdriveM_Reset(const U8 nPlaneBase, const U8 nWidth);

extern void FxOverdriveS_Process(const FX_CTX* pCtx, FLOAT32* const apPlane[], const U16 nFrames);
extern void FxOverdriveS_Reset(const U8 nPlaneBase, const U8 nWidth);



#endif // #ifndef FX_OVERDRIVE_H

/****************************************** end of file *******************************************/
