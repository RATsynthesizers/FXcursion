/**
 * @file      looper.h
 *
 * @details   Two loopers, four transports.
 *
 *            ------------------------------------------------------------------
 *            THE MODEL
 *            ------------------------------------------------------------------
 *
 *            LOOPER IDENTITY IS DETERMINED BY PLANE, NOT BY CHAIN:
 *
 *                planes 0..1  ->  looper 0
 *                planes 2..3  ->  looper 1
 *
 *            in every topology. So:
 *
 *              2 stereo chains : stereo1 is looper 0 (both its planes),
 *                                stereo2 is looper 1
 *              4 mono chains   : mono1 and mono2 share looper 0's LENGTH but
 *                                record into their own plane buffers; mono3 and
 *                                mono4 share looper 1's
 *
 *            LENGTH is per looper pair. TRANSPORT is per chain, and the two
 *            chains of a pair run independently - mono1 can be playing while
 *            mono2 is still armed.
 *
 *            Because length is shared, changing a pair's bar count while either
 *            of its transports is non-idle is REFUSED rather than applied. The
 *            alternative - truncating audio that is currently playing on the
 *            other chain - is worse.
 *
 *            ------------------------------------------------------------------
 *            STORAGE
 *            ------------------------------------------------------------------
 *
 *            Four plane regions of LOOP_MAX_SEC, packed 24-bit, in the two
 *            SDRAM banks - 11 MiB of each, holding a stereo pair twice so undo
 *            has a snapshot. There is no QSPI PSRAM on this board any more.
 *
 *            THIS MODULE NEVER TOUCHES THAT MEMORY DIRECTLY. loop_mem stages
 *            one block per plane in DTCM and copies it in and out. Everything
 *            below operates on that
 *            window; see loop_store.h for why, and for the one-block latency it
 *            costs on a PLAY or OVERDUB start.
 *
 *            Loop length is TEMPO-DERIVED: bars * Params_BarFrames(). The
 *            maximum usable bar count therefore depends on both BPM and time
 *            signature, and the GUI needs the same arithmetic to bound its
 *            picker.
 *
 *            ------------------------------------------------------------------
 *            TEMPO RE-LOCK - deliberate placeholder
 *            ------------------------------------------------------------------
 *
 *            Each loop records the BPM it was captured at, in fRecordedBpm.
 *            Playback rate is currently hardcoded to 1.0. When re-lock is
 *            implemented, the ratio is fRecordedBpm / current BPM and playback
 *            becomes a fractional read - about twenty lines.
 *
 *            The field exists NOW, and is written NOW, specifically so that
 *            loops and presets saved before re-lock ships stay valid after it
 *            does. That is the only expensive part of adding it later.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef LOOPER_H
#define LOOPER_H



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
 * @brief Clear all transports and lengths. Does NOT clear the recorded audio.
 *
 * MUST be called before any audio runs, and after LoopMem_Init. Loop length
 * starts at 0, which means "empty", so whatever the SDRAM powered up holding is
 * never played.
 */
extern STD_RESULT Looper_Init(void);

/**
 * @brief Take new bar counts from a configuration. Call from the super-loop.
 *
 * A pair whose transports are not both stopped keeps its current length.
 */
extern void Looper_Apply(const PROTO_CFG* const pCfg, const GRID* const pGrid);

/**
 * @brief Stop every transport because the topology is about to change.
 *
 * MUST be called from Grid_Apply whenever the topology changes, BEFORE the new
 * grid is published.
 *
 * Transports are per chain but loop buffers are per PLANE, and a topology change
 * re-maps which planes a chain owns. Leaving a transport running across that
 * remap would point it at somebody else's audio - e.g. going 4-mono to 2-stereo,
 * the transport that was recording plane 1 would suddenly be advancing planes
 * 2 and 3.
 *
 * The recorded AUDIO is deliberately kept, and so are the loop lengths, because
 * the plane buffers themselves do not move. That gives a genuinely useful
 * behaviour for free: two mono loops of equal length merge into one stereo loop
 * when their chains merge, and a stereo loop splits into its L and R as two mono
 * loops when the chains split.
 */
extern void Looper_TopologyChanged(void);

/**
 * @brief Transport command for one chain. Call from the super-loop.
 *
 * @param nChain   chain index
 * @param eAction  PROTO_TRANSPORT_ACT
 */
extern STD_RESULT Looper_Transport(const U8 nChain, const U8 eAction);

/**
 * @brief Run one chain's looper over one block, in place.
 *
 * Call from the audio ISR only.
 */
extern void Looper_Process(const GRID* const pGrid,
                           const U8 nChain,
                           FLOAT32* const apChain[],
                           const U16 nFrames);

/**
 * @brief Fill in the looper part of a telemetry frame.
 */
extern void Looper_GetTelemetry(PROTO_TELEMETRY* const pTelem);



#endif // #ifndef LOOPER_H

/****************************************** end of file *******************************************/
