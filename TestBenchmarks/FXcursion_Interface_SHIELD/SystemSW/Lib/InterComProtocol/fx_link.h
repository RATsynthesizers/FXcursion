/**
 * @file      fx_link.h
 *
 * @details   Framing, parsing and CRC for the control link. Shared by BOTH
 *            firmwares, byte for byte.
 *
 *            ############################################################
 *            #  DUPLICATED IN THE INTERFACE CONTROLLER - keep in sync.  #
 *            #  Use TestBenchmarks/sync_shared.py.                      #
 *            ############################################################
 *
 *            ------------------------------------------------------------------
 *            WHY THIS IS SHARED AND NOT WRITTEN TWICE
 *            ------------------------------------------------------------------
 *
 *            The two boards have to agree about the wire down to the byte, and
 *            the hard part is not the happy path - it is what happens on BAD
 *            input. A link between two boards sees corruption, half frames and
 *            resets, and a receiver that resynchronises wrongly can wedge
 *            permanently on a stream that is otherwise fine. That behaviour is
 *            subtle enough to be worth exactly one implementation:
 *
 *              - 0xA5 0xA5 0x5A is a valid frame start, so seeing the first sync
 *                byte again while waiting for the second must NOT reset to the
 *                beginning.
 *              - a length above PROTO_PAYLOAD_MAX resynchronises rather than
 *                being clamped, because a clamped length silently mis-frames
 *                everything after it.
 *              - a well formed frame with an unknown command is DROPPED, not
 *                resynchronised on, so a newer peer can add commands.
 *
 *            The audio project's host test suite drives all of that byte by
 *            byte. Sharing the module means the interface controller inherits
 *            those tests instead of getting a second, untested parser.
 *
 *            ------------------------------------------------------------------
 *            WHAT IS NOT HERE
 *            ------------------------------------------------------------------
 *
 *            Transport and meaning. Bytes go out through a function pointer and
 *            complete frames come back through another, so this file has no
 *            HAL, no DMA and no idea what any command does. That is what makes
 *            it testable on a PC, and it is why the same object code serves a
 *            board that ANSWERS commands and a board that SENDS them.
 *
 *            ------------------------------------------------------------------
 *            MEMORY
 *            ------------------------------------------------------------------
 *
 *            About 470 bytes of plain statics - one instance per firmware,
 *            which is right, because these are two separate programs.
 *
 *            Deliberately NOT placed in DTCM on the audio side, unlike most of
 *            that project's state. Nothing here is touched by the audio ISR or
 *            by any DMA: bytes are drained and frames dispatched from the
 *            super-loop, and dispatch may clear megabytes of delay line. So the
 *            tiering that matters for effect state buys nothing here, and a
 *            section attribute from mem_map.h would make this file
 *            project-specific - which is the one thing it must not be.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_LINK_H
#define FX_LINK_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "fx_protocol.h"

/* One wire contract, two compilers: the audio controller builds this as C, the
 * interface controller includes it from C++. Without this any symbol declared
 * below gets a mangled name and the link fails against the C definition. The
 * guard sits AFTER the includes on purpose - wrapping <string.h> in extern "C"
 * is the classic way to break a C++ build. */
#ifdef __cplusplus
extern "C" {
#endif



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/**
 * Bytes buffered between arrival and parsing. Must be a power of two - the ring
 * indices are masked, not compared.
 *
 * 256 is over two maximum frames, so a whole configuration frame can land while
 * the consumer is busy elsewhere. Overflow is counted rather than blocking; the
 * CRC then rejects the damaged frame, which is the correct outcome - the
 * alternative is a receiver that stalls its own caller.
 */
#define FX_LINK_RX_RING_BYTES           (256U)

FXC_STATIC_ASSERT((FX_LINK_RX_RING_BYTES & (FX_LINK_RX_RING_BYTES - 1U)) == 0U,
                  fx_link_ring_is_power_of_two);

FXC_STATIC_ASSERT(FX_LINK_RX_RING_BYTES >= (2U * PROTO_FRAME_MAX),
                  fx_link_ring_holds_two_frames);



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief Transmit callback. Must not block - queue the bytes and return.
 */
typedef STD_RESULT (*FX_LINK_TX_FN)(const U8* const pData, const U16 nLength);

/**
 * @brief Called once per accepted frame, from FxLink_Poll's context.
 *
 * The payload pointer is only valid for the duration of the call: the buffer is
 * reused by the next frame. Copy anything worth keeping.
 */
typedef void (*FX_LINK_DISPATCH_FN)(const U8 eCmd, const U8* const pPayload, const U8 nLength);

/**
 * @brief Link health. All four should be zero on a good cable.
 */
typedef struct stFX_LINK_STATS
{
    U32 nFramesOk;
    U32 nCrcErrors;
    U32 nResyncs;
    U32 nRxOverflows;

} FX_LINK_STATS;



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Reset the parser and register the two callbacks.
 *
 * @param pfTx        transmit callback, or NULL on a receive-only harness
 * @param pfDispatch  frame callback, or NULL to accept and count frames without
 *                    acting on them
 */
extern STD_RESULT FxLink_Init(const FX_LINK_TX_FN pfTx, const FX_LINK_DISPATCH_FN pfDispatch);

/**
 * @brief Push one received byte. Safe to call from an ISR.
 *
 * Goes into a lock-free single-producer single-consumer ring. Nothing is parsed
 * here, so the cost is bounded and constant - which is the point of doing it
 * this way rather than parsing in the interrupt.
 */
extern void FxLink_RxByte(const U8 nByte);

/**
 * @brief Drain the ring, parse whole frames, dispatch them.
 *
 * NOT for interrupt context: dispatch is where the work happens, and on the
 * audio side that includes rebuilding the grid.
 *
 * @return frames dispatched during this call
 */
extern U16 FxLink_Poll(void);

/** Frame a command and hand it to the transmit callback. */
extern STD_RESULT FxLink_Send(const U8 eCmd, const U8* const pPayload, const U8 nLength);

/** Link statistics. Never NULL. */
extern const FX_LINK_STATS* FxLink_Stats(void);



#ifdef __cplusplus
}
#endif

#endif // #ifndef FX_LINK_H

/****************************************** end of file *******************************************/
