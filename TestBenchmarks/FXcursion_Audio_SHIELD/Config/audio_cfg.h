/**
 * @file      audio_cfg.h
 *
 * @details   Audio-controller-only configuration.
 *
 *            Dimensions the interface controller also needs - channel count,
 *            grid size, parameter count, sample rate, loop length - live in
 *            InterComProtocol/fx_defs.h so there is exactly one definition of each. This
 *            file holds what only the audio side cares about: block size,
 *            effect memory limits and link tuning.
 *
 *            There is no dynamic allocation anywhere in this project. Changing
 *            a value here changes a static array somewhere and nothing else.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef AUDIO_CFG_H
#define AUDIO_CFG_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "fx_defs.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

// ------------------------------------------------------------------------------------------------
// Block processing
// ------------------------------------------------------------------------------------------------

// Frames processed per DSP block.
//
// Buffering latency is 2 * AUDIO_BLOCK_FRAMES:
//     32 frames -> 0.67 ms      64 frames -> 1.33 ms      128 frames -> 2.67 ms
// Add codec group delay on top. 64 is the recommended starting point; measure on
// hardware with the DWT load meter before changing it.
//
// Must be a power of two. Valid values: [16 ; 256].
#define AUDIO_BLOCK_FRAMES              (64U)

// Full-scale magnitude of one hardware sample, as a power of two.
// A power of two keeps the scale factor exact in FLOAT32 and makes -1.0f map
// exactly to negative full scale. Do NOT use 8388607.
#define AUDIO_FULLSCALE                 (8388608.0f)    /* 2^23, 24-bit */

// The same fact as integers. One definition, because three modules clamp to it
// - the output stage, the looper's packer and the headphone bus - and they must
// agree or a sample that is legal in one is wrapped in another.
#define AUDIO_SAMPLE_MAX                (8388607L)      /*  2^23 - 1 */
#define AUDIO_SAMPLE_MIN                (-8388608L)     /* -2^23     */

// ------------------------------------------------------------------------------------------------
// Headphone monitor bus
// ------------------------------------------------------------------------------------------------

// Codec 3 is an output-only monitor: a stereo sum of the main chains, on its own
// frame sync. Frequency-locked to the same crystal, so it cannot drift; only the
// frame phase differs, which this many blocks of elastic buffer absorbs.
#define HP_BUS_WIDTH                    (2U)
#define HP_ELASTIC_BLOCKS               (2U)

// TODO: selective tap. Today the send is taken post-everything - the same signal
// that leaves the physical outputs. The field for a per-chain tap point is
// reserved in PROTO_CFG; when it is implemented, this is where its default goes.

// ------------------------------------------------------------------------------------------------
// Effect limits
// ------------------------------------------------------------------------------------------------

// Longest delay line, seconds. Reserved statically per plane in SDRAM bank 1.
#define DELAY_MAX_SEC                   (4U)

// Longest modulation delay (chorus / flanger / vibrato), milliseconds.
#define MODDELAY_MAX_MS                 (30U)

// Reverb delay memory per plane, frames.
//
// A Dattorro/FDN tank with a usable pre-delay needs roughly 700 ms of total line
// storage per plane - about 134 KiB. 65536 frames is 256 KiB, near twice that,
// because bank 2 has 16 MiB and there is no reason to be stingy with the one
// effect that most rewards it.
#define REVERB_FRAMES_PER_PLANE         (65536UL)

// ------------------------------------------------------------------------------------------------
// Looper storage
// ------------------------------------------------------------------------------------------------

// Loop audio is stored packed 24-bit in the SDRAM banks - see loop_mem.h.
//
// FLOAT32 would cost a third more for no audible gain: a loop is a recording of
// already-processed audio, and 24 bits is the converter's own resolution.
#define LOOP_BYTES_PER_SAMPLE           (3U)

// ------------------------------------------------------------------------------------------------
// SDRAM
// ------------------------------------------------------------------------------------------------

// Refresh rate counter for the W9812G6xH driver, at SDCLK = D1HCLK/2 = 120 MHz.
//
//     COUNT = (64 ms / 4096 rows) * 120 MHz - 20 = 1855
//
// The driver's own default is the interface board's more conservative value.
#define SDRAM_RFR_COUNT                 (1855U)

// ------------------------------------------------------------------------------------------------
// Control link and telemetry
// ------------------------------------------------------------------------------------------------

// Telemetry frame period, milliseconds. Matched to GUI refresh, not to audio.
#define TELEMETRY_PERIOD_MS             (40U)

// Telemetry frames between diagnostic frames. Nothing in PROTO_DIAG changes
// fast, and at 115200 baud the link is worth spending carefully: telemetry
// alone is already about 12% of it.
#define TELEMETRY_DIAG_EVERY            (8U)

// Control link transmit ring, bytes. Must hold the largest frame twice over, so
// an ACK can be queued while a telemetry frame is still going out.
#define CTRL_TX_RING_BYTES              (512U)

// Control link receive ring buffer, bytes. Must be a power of two.
#define CTRL_RX_RING_BYTES              (256U)

// Mixer gain smoothing time constant, milliseconds.
// Without this every gain change zipper-clicks. Do not set to zero.
#define MIX_GAIN_SMOOTH_MS              (20U)

// Peak meter decay, dB per second, reported through telemetry.
#define METER_DECAY_DB_PER_SEC          (20.0f)



/***************************************************************************************************
* Derived constants - do not edit
***************************************************************************************************/

#define AUDIO_BLOCK_PERIOD_US           ((AUDIO_BLOCK_FRAMES * 1000000UL) / AUDIO_SAMPLE_RATE_HZ)

/* No casts in these: they are evaluated by the preprocessor in the #if checks
 * below, where a cast is not a valid token. AUDIO_SAMPLE_RATE_HZ carries the UL
 * suffix, so every product is already unsigned long. */
#define DELAY_MAX_FRAMES                (DELAY_MAX_SEC * AUDIO_SAMPLE_RATE_HZ)
#define MODDELAY_MAX_FRAMES             ((MODDELAY_MAX_MS * AUDIO_SAMPLE_RATE_HZ) / 1000UL)
#define LOOP_MAX_FRAMES                 (LOOP_MAX_SEC * AUDIO_SAMPLE_RATE_HZ)
#define LOOP_MAX_BYTES                  (LOOP_MAX_FRAMES * LOOP_BYTES_PER_SAMPLE)



/***************************************************************************************************
* Configuration sanity checks
***************************************************************************************************/

#if ((AUDIO_BLOCK_FRAMES & (AUDIO_BLOCK_FRAMES - 1U)) != 0U)
#error "AUDIO_BLOCK_FRAMES must be a power of two"
#endif

#if (AUDIO_BLOCK_FRAMES < 16U)
#error "AUDIO_BLOCK_FRAMES below 16 makes per-block overhead dominate"
#endif

#if (AUDIO_PLANE_QTY != AUDIO_CH_QTY)
#error "Plane count must equal physical channel count - see the width invariant in fx_defs.h"
#endif

#if ((CHAIN_MAX_QTY * CHAIN_MAX_WIDTH) < AUDIO_CH_QTY)
#error "Chains cannot cover all physical channels"
#endif

#if ((AUDIO_PLANE_QTY % LOOPER_QTY) != 0U)
#error "Planes must divide evenly between loopers"
#endif

#if ((CTRL_RX_RING_BYTES & (CTRL_RX_RING_BYTES - 1U)) != 0U)
#error "CTRL_RX_RING_BYTES must be a power of two"
#endif

#if (MODDELAY_MAX_FRAMES >= DELAY_MAX_FRAMES)
#error "Modulation delay must be shorter than the main delay line"
#endif

#if (HP_BUS_WIDTH != 2U)
#error "The headphone monitor is a stereo bus"
#endif



#endif // #ifndef AUDIO_CFG_H

/****************************************** end of file *******************************************/
