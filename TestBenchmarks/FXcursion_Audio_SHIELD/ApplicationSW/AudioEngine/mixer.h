/**
 * @file      mixer.h
 *
 * @details   The routing matrix. Occupies one whole column of the grid.
 *
 *            Every chain can feed every chain: an nChainQty x nChainQty matrix
 *            of cells, each with a gain and a pan. Input configuration always
 *            matches output configuration, so destination chain i has the same
 *            width as source chain i.
 *
 *            ------------------------------------------------------------------
 *            WIDTH TRANSITIONS
 *            ------------------------------------------------------------------
 *
 *            Sources and destinations can still differ in width when a topology
 *            mixes stereo and mono chains, e.g. TOPO_ST1_2MONO:
 *
 *              2 -> 1   out = (L + R) * gain * 0.707     (-3 dB, preserves power)
 *              1 -> 2   outL = in * gain * cos(pan)      (constant-power pan)
 *                       outR = in * gain * sin(pan)
 *              n -> n   per-plane multiply; pan is ignored
 *
 *            ------------------------------------------------------------------
 *            AUTO GAIN
 *            ------------------------------------------------------------------
 *
 *            When bAutoGain is set, a bus carrying N active sources is scaled by
 *            1/sqrt(N).
 *
 *            NOT 1/N. Guitar channels are uncorrelated, so their sum grows as
 *            sqrt(N) in power; 1/N over-attenuates audibly, and two sources come
 *            out noticeably quieter than one. The old C++ mixer used 1/N, and it
 *            also counted sources that were connected but silent, which made the
 *            level of one bus depend on what was patched somewhere else.
 *
 *            Recommendation, which the switch exists to let you test: use auto
 *            gain to pick the DEFAULT gain of a newly created connection, and
 *            leave it off in normal operation. A bus whose level changes because
 *            the user touched a different bus is baffling from the front panel,
 *            and with FLOAT32 internally there is no clipping to protect against
 *            until the output conversion. Meters plus a soft clip at the output
 *            are the honest protection.
 *
 *            ------------------------------------------------------------------
 *            SMOOTHING
 *            ------------------------------------------------------------------
 *
 *            Every gain is ramped toward its target once per block with a
 *            one-pole filter. Without this, every gain change zipper-clicks.
 *            This is the single most commonly omitted detail in a homebrew
 *            mixer.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef MIXER_H
#define MIXER_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"
#include "grid.h"
#include "fx_protocol.h"



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Unity on the diagonal, silence elsewhere, ramps settled.
 *
 * MUST be called before any audio runs - the matrix lives in DTCM, which the
 * startup code neither copies nor zeroes.
 */
extern STD_RESULT Mixer_Init(void);

/**
 * @brief Take new gain and pan targets from a configuration.
 *
 * Call from the super-loop. Only the TARGETS change here; the audio path ramps
 * toward them, so this is safe to call at any time without a lock.
 */
extern void Mixer_Apply(const PROTO_CFG* const pCfg, const GRID* const pGrid);

/**
 * @brief Sum every chain into every chain, in place.
 *
 * Call from the audio ISR only, once per block, when the column loop reaches the
 * mixer column.
 *
 * @param pGrid    running graph
 * @param apPlane  AUDIO_PLANE_QTY plane pointers
 * @param nFrames  frames in this block
 */
extern void Mixer_Process(const GRID* const pGrid,
                          FLOAT32* const apPlane[],
                          const U16 nFrames);



#endif // #ifndef MIXER_H

/****************************************** end of file *******************************************/
