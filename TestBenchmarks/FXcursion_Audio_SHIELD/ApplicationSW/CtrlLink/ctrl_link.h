/**
 * @file      ctrl_link.h
 *
 * @details   What the AUDIO controller does with a control frame.
 *
 *            The framing, the CRC and the parse state machine are not here -
 *            they live in Shared/fx_link.h and are compiled into both firmwares
 *            from the same source. This module is only the audio side's answer
 *            to "a frame arrived, now what": apply a grid, set a parameter, move
 *            the transport, gate the recorder stream.
 *
 *            The split matters because those are the two things that have to be
 *            true at once. The wire format must be identical on both boards, and
 *            the response to it must not be - the interface controller answers
 *            almost nothing and sends almost everything.
 *
 *            WHAT THIS REPLACES
 *
 *            The old link sent 5-byte unframed commands over a circular DMA
 *            buffer, mutated that buffer in place while the DMA was writing to
 *            it, had no CRC, and made the GUI spin on "while (!updateFlag);".
 *            A single dropped byte desynchronised it permanently, and a
 *            desynchronised command is still a VALID command - it would silently
 *            rearrange the signal chain.
 *
 *            Now: sync word, length, CRC-16, resynchronisation on any error,
 *            and nothing blocks on either side.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 * \date      02.09.2026 - Framing moved to Shared/fx_link.c so the interface
 *                         controller inherits the same parser and its tests
 *
 * @copyright RAT Synthesizers
 */

#ifndef CTRL_LINK_H
#define CTRL_LINK_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"
#include "fx_protocol.h"
#include "fx_link.h"



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/*
 * Aliases, so this module's callers did not have to change when the framing
 * moved out. Both are structurally the shared types, not copies of them.
 */
typedef FX_LINK_TX_FN CTRL_TX_FN;
typedef FX_LINK_STATS CTRL_STATS;



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/** @param pfTx  transmit callback, may be NULL on a receive-only test harness */
extern STD_RESULT CtrlLink_Init(const CTRL_TX_FN pfTx);

/**
 * @brief Push one received byte. Safe to call from an ISR.
 *
 * Bytes go into a lock-free single-producer ring buffer. Nothing is parsed here.
 */
extern void CtrlLink_RxByte(const U8 nByte);

/**
 * @brief Drain the ring, parse complete frames, dispatch them.
 *
 * Call from the SUPER-LOOP only - dispatch calls Grid_Apply, which may clear
 * megabytes of delay line.
 *
 * @return number of frames dispatched
 */
extern U16 CtrlLink_Poll(void);

/** Build and send a telemetry frame. Call from the super-loop. */
extern STD_RESULT CtrlLink_SendTelemetry(void);

/** Send an arbitrary frame. Exposed mainly for tests. */
extern STD_RESULT CtrlLink_SendFrame(const U8 eCmd, const U8* const pPayload, const U8 nLength);

/** Link statistics. Never NULL. */
extern const CTRL_STATS* CtrlLink_Stats(void);



#endif // #ifndef CTRL_LINK_H

/****************************************** end of file *******************************************/
