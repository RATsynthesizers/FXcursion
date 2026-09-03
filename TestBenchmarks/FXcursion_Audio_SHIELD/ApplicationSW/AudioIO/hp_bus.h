/**
 * @file      hp_bus.h
 *
 * @details   The headphone monitor bus: a third converter fed a sum of the
 *            main outputs.
 *
 *            ------------------------------------------------------------------
 *            WHAT IT IS
 *            ------------------------------------------------------------------
 *
 *            Codec 2 is output only and carries a stereo monitor mix. Today the
 *            source is POST EVERYTHING - literally the same samples that leave
 *            the physical outputs, taken after the grid has run.
 *
 *            The sum is exactly a sum. No auto gain, no normalisation, no
 *            limiter. What keeps it from clipping is the master gain, which
 *            defaults to one half: two planes are summed per side, so a half
 *            makes a sum of two full-scale channels land exactly at full scale.
 *            That is arithmetic, not a fudge factor, and the player can raise
 *            it if they want more.
 *
 *            ------------------------------------------------------------------
 *            SELECTIVE TAP - HALF RESERVED, HALF ALREADY HERE
 *            ------------------------------------------------------------------
 *
 *            "Selective tap" is two separate questions:
 *
 *              WHICH channels do I hear?   HpBus_SetSourceMask. Works now. The
 *                                          masks are per side, so a chain can be
 *                                          soloed, muted, or moved to one ear.
 *
 *              WHERE in the chain?         HpBus_SetTapPoint. RESERVED. Taking
 *                                          a chain pre-effects means the engine
 *                                          has to hand out an intermediate
 *                                          snapshot of a plane, which it does
 *                                          not do today - Grid_Process works in
 *                                          place and there is nothing to read
 *                                          halfway through.
 *
 *                                          Implementing it means one extra
 *                                          plane-sized buffer per tapped chain
 *                                          and a copy at the tap column. The
 *                                          storage and the API are here now so
 *                                          presets saved before it lands stay
 *                                          valid after.
 *
 *            ------------------------------------------------------------------
 *            LATENCY
 *            ------------------------------------------------------------------
 *
 *            One block more than the main outputs, because the headphone DMA
 *            has its own frame phase and the ISR writes ahead of it rather than
 *            into it. 1333 us on a monitor path that nothing is synchronised
 *            to. See audio_io_cfg.h.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef HP_BUS_H
#define HP_BUS_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"

#include "audio_io_cfg.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/** The only tap point that exists today: after the whole grid. */
#define HP_TAP_POST_EVERYTHING          (0U)

/** Reserved. Before the chain's FX block. Needs an engine hook - see above. */
#define HP_TAP_PRE_FX                   (1U)

/** Reserved. After the FX block but before the mixer. */
#define HP_TAP_POST_FX                  (2U)



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Reset the bus to its defaults: even planes left, odd right, master at
 *        HP_MASTER_DEFAULT, every chain tapped post-everything.
 */
extern STD_RESULT HpBus_Init(void);

/**
 * @brief Set the monitor master gain. Smoothed, so the knob does not click.
 *
 * @param fGain  0.0 to HP_MASTER_MAX; clamped, never refused
 */
extern void HpBus_SetMaster(const FLOAT32 fGain);

/** The master gain currently being approached. */
extern FLOAT32 HpBus_Master(void);

/**
 * @brief Choose which planes feed each side.
 *
 * @param nLeftMask   bit p set means plane p sums into the left channel
 * @param nRightMask  the same for the right channel
 *
 * @return RESULT_INVALID_PARAM_0 if a mask names a plane that does not exist
 */
extern STD_RESULT HpBus_SetSourceMask(const U8 nLeftMask, const U8 nRightMask);

/**
 * @brief RESERVED. Where in a chain the monitor is taken from.
 *
 * Accepts HP_TAP_POST_EVERYTHING and refuses the rest until the engine can
 * produce an intermediate snapshot. The value is stored either way so the GUI
 * and the telemetry can already round-trip it.
 *
 * @return RESULT_NOT_OK for a tap point that is reserved but not yet implemented
 */
extern STD_RESULT HpBus_SetTapPoint(const U8 nChain, const U8 eTapPoint);

/** What HpBus_SetTapPoint last stored for a chain. */
extern U8 HpBus_TapPoint(const U8 nChain);

/**
 * @brief Sum one processed block into the headphone converter's buffer.
 *
 * @param pBlock   engine output, nFrames x AUDIO_CH_QTY, 24-bit range
 * @param pHp      destination, nFrames x AIO_HP_SLOTS hardware words
 * @param nFrames  frames to produce
 */
extern void HpBus_Process(const S32* const pBlock, S32* const pHp, const U16 nFrames);

/** Samples the monitor sum had to clamp. Non-zero means turn the master down. */
extern U32 HpBus_ClipCount(void);



#endif // #ifndef HP_BUS_H

/****************************************** end of file *******************************************/
