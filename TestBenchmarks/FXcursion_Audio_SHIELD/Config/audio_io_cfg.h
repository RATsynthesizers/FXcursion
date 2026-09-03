/**
 * @file      audio_io_cfg.h
 *
 * @details   Board wiring for the audio path: which converter carries which
 *            plane, and how big the DMA buffers are.
 *
 *            ------------------------------------------------------------------
 *            THE CHANNEL MAP IS BOARD KNOWLEDGE, NOT ENGINE KNOWLEDGE
 *            ------------------------------------------------------------------
 *
 *            The engine works on PLANES and knows nothing about converters.
 *            Everything that says "codec 1 is planes 0 and 1" lives here, so
 *            re-wiring the board is a change to this file and nothing else.
 *
 *                SAI1 block A (RX)   codec 0 ADC  ->  planes 0, 1
 *                SAI1 block B (TX)   codec 0 DAC  <-  planes 0, 1
 *                SAI2 block A (RX)   codec 1 ADC  ->  planes 2, 3
 *                SAI2 block B (TX)   codec 1 DAC  <-  planes 2, 3
 *                I2S3        (TX)    codec 2 DAC  <-  headphone L, R
 *
 *            ------------------------------------------------------------------
 *            WHY THE HEADPHONE BUFFER IS BIGGER THAN THE OTHERS
 *            ------------------------------------------------------------------
 *
 *            The four main streams share SAI1's frame sync, so their DMA
 *            pointers move together and one buffer half is always safe to touch
 *            while the other is in flight.
 *
 *            I2S3 does not. It has its own WS and its own DMA. It cannot DRIFT
 *            - it divides the same 24.576 MHz crystal - but its frame phase
 *            relative to SAI1 is arbitrary and fixed at start-up. A two-half
 *            buffer would therefore be either always safe or always marginal,
 *            depending on a phase nobody controls.
 *
 *            So the headphone buffer is HP_ELASTIC_BLOCKS blocks deep and the
 *            audio ISR writes two slots ahead of whichever slot the DMA is
 *            reading. That gives a block of margin on both sides for any phase,
 *            at the cost of one extra block of monitor latency.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef AUDIO_IO_CFG_H
#define AUDIO_IO_CFG_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"

#include "audio_cfg.h"
#include "fx_defs.h"



/***************************************************************************************************
* Channel map
***************************************************************************************************/

/** Slots carried by one SAI block. A WM8731 is a stereo converter. */
#define AIO_SLOTS_PER_SAI               (2U)

/** First engine plane carried by SAI1. */
#define AIO_SAI1_PLANE_BASE             (0U)

/** First engine plane carried by SAI2. */
#define AIO_SAI2_PLANE_BASE             (2U)

/** Slots on the headphone converter. */
#define AIO_HP_SLOTS                    (HP_BUS_WIDTH)



/***************************************************************************************************
* Headphone bus defaults
***************************************************************************************************/

/**
 * Which planes are summed into the left and right headphone channels.
 *
 * Even planes left, odd planes right. In the two-stereo topology that is the
 * natural image - each stereo chain keeps its sides. In four-mono it spreads
 * the four channels two per side rather than collapsing them to one point,
 * which is the more useful default for monitoring.
 *
 * These are the CHANNEL half of selective tap and they work today; see
 * HpBus_SetSourceMask. The tap POINT - hearing a chain before its effects
 * rather than after - is the half that still needs an engine hook.
 */
#define HP_LEFT_PLANE_MASK              (0x5U)      /* planes 0 and 2 */
#define HP_RIGHT_PLANE_MASK             (0xAU)      /* planes 1 and 3 */

/**
 * Default headphone master gain.
 *
 * Two planes are summed per side, so unity would clip whenever both are hot.
 * One half is the gain at which a sum of two full-scale channels is exactly
 * full scale - a pure sum that cannot clip, with no hidden normalisation.
 * The player raises it from the GUI if they want it louder.
 */
#define HP_MASTER_DEFAULT               (0.5f)

/** Bounds accepted by HpBus_SetMaster. */
#define HP_MASTER_MAX                   (2.0f)



/***************************************************************************************************
* DMA buffer geometry
***************************************************************************************************/

/** 32-bit words in one block of one stereo stream. */
#define AIO_BLOCK_WORDS                 (AUDIO_BLOCK_FRAMES * AIO_SLOTS_PER_SAI)

/** Halves in a main stream buffer. Classic ping-pong. */
#define AIO_MAIN_SLOTS                  (2U)

/** Words in one main stream DMA buffer. 64 x 2 x 2 = 256 words = 1 KiB. */
#define AIO_MAIN_WORDS                  (AIO_BLOCK_WORDS * AIO_MAIN_SLOTS)

/** Slots in the headphone DMA buffer. Two per elastic block. */
#define AIO_HP_BUF_SLOTS                (HP_ELASTIC_BLOCKS * 2U)

/** Words in the headphone DMA buffer. 64 x 2 x 4 = 512 words = 2 KiB. */
#define AIO_HP_WORDS                    (AIO_BLOCK_WORDS * AIO_HP_BUF_SLOTS)

/** How far ahead of the DMA read slot the ISR writes. Half the ring. */
#define AIO_HP_WRITE_LEAD               (AIO_HP_BUF_SLOTS / 2U)



/***************************************************************************************************
* Configuration sanity checks
***************************************************************************************************/

#if ((AIO_SAI1_PLANE_BASE + AIO_SLOTS_PER_SAI) != AIO_SAI2_PLANE_BASE)
#error "The two SAI blocks must cover consecutive plane ranges"
#endif

#if ((AIO_SAI2_PLANE_BASE + AIO_SLOTS_PER_SAI) != AUDIO_CH_QTY)
#error "The channel map does not cover every plane exactly once"
#endif

#if (AIO_HP_BUF_SLOTS < 4U)
#error "The headphone ring needs at least four slots to lead the DMA safely"
#endif

/* HAL_SAI_Receive_DMA and HAL_I2S_Transmit_DMA take the item count as U16. */
#if (AIO_HP_WORDS > 65535U)
#error "DMA item count does not fit the HAL's U16 size parameter"
#endif



#endif // #ifndef AUDIO_IO_CFG_H

/****************************************** end of file *******************************************/
