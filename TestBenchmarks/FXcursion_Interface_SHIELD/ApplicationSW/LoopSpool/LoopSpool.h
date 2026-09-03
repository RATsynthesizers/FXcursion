/***************************************************************************************************
* @file     LoopSpool.h
*
* @brief    Loop staging slots, and the card at the other end of them.
*
*           Renamed from LoopStore to keep it distinct from the audio board's
*           loop_mem.c, which owns the looper memory itself. This is the
*           interface side: it SPOOLS loops between the audio board and the SD
*           card and owns neither end.
*
*           ------------------------------------------------------------------
*           WHAT THIS OWNS
*           ------------------------------------------------------------------
*
*           Two 5.5 MiB slots, SDRAM_LOOP_A and SDRAM_LOOP_B, bounded by linker
*           symbols so no address appears here. Each holds exactly one active
*           stereo loop as the audio board records it. A loop passes through in
*           both directions:
*
*             SAVE   audio --SPI--> slot --f_write--> loopN.wav
*             LOAD   loopN.wav --f_read--> slot --SPI--> audio
*
*           Shared/fx_loop.c is the SPI half and both boards run it. This file
*           is the card half and exists only here.
*
*           ------------------------------------------------------------------
*           WHY TWO SLOTS
*           ------------------------------------------------------------------
*
*           The card is slower than the link. With one slot every save would
*           have to reach the SD before the next loop could be handed over, so
*           the player would be blocked by storage they cannot see. With two,
*           slot B receives while slot A is being written.
*
*           ------------------------------------------------------------------
*           WHO GETS THE CARD, AND IN WHAT ORDER
*           ------------------------------------------------------------------
*
*           The recorder writes WAVs too, and a card handles one sequential
*           stream far better than two interleaved. So they take turns, and the
*           turn order is not simply first-come:
*
*             1. A loop write holds the card for its whole duration. The
*                recorder waits, filling its ring.
*
*             2. When that write finishes, THE RECORDER GOES NEXT - even if
*                another loop is already queued. It has been locked out for
*                seconds and is carrying a backlog; making it wait behind a
*                second loop is how a backlog becomes an overrun, and overrun
*                audio is gone.
*
*             3. Only once the recorder has caught up does the queued loop
*                start. It starts immediately at that point - there is no
*                polling interval to wait out.
*
*           LoopSpool_IsBusy stays TRUE across ALL of that: through the first
*           write, through the recorder's recovery, and through the second
*           write. It never blinks false in the gaps. That matters twice over -
*           it is the "saving" indicator the player sees, and it is the gate
*           that stops a third card operation starting mid-sequence.
*
***************************************************************************************************/

#ifndef LOOPSPOOL_H
#define LOOPSPOOL_H

/***************************************************************************************************
* Included header files
***************************************************************************************************/

#include "general.h"
#include "fx_loop.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/** Staging slots. One receiving while the other is written. */
#define LOOPSPOOL_SLOT_QTY              (2U)

/** Sample rate a loop file must be in to be usable without resampling. */
#define LOOPSPOOL_SAMPLE_RATE           (48000UL)

/** Longest loop file name, including the extension and the terminator. */
#define LOOPSPOOL_NAME_MAX              (32U)

/** No slot. Returned by LoopSpool_Acquire when both are in use. */
#define LOOPSPOOL_SLOT_NONE             (0xFFU)

/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/** What a staged loop is, once it is in a slot. */
typedef struct stLOOPSPOOL_INFO
{
    U32 nBytes;                 /**< payload bytes in the slot                   */
    U32 nCrc;                   /**< Crc32_Ieee over exactly those bytes         */
    U32 nSamples;               /**< per plane                                   */

    U8  nPlaneQty;              /**< 1 mono, 2 stereo                            */
    U8  eFormat;                /**< PROTO_LOOP_FMT                              */
    U8  nReserved[2];

} LOOPSPOOL_INFO;

/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Create the card lock and start the spool thread.
 *
 * Call once, BEFORE the recorder thread starts: the recorder takes the card
 * lock around every pass, and LoopSpool_SdLock only short-circuits while the
 * lock does not yet exist.
 */
extern STD_RESULT LoopSpool_Init(void);

/**
 * @brief Claim a free slot to receive into.
 *
 * @return slot index, or LOOPSPOOL_SLOT_NONE when both are in use - which is
 *         the honest answer to "can you take another loop right now" and the
 *         reason PROTO_LOOP_OPEN can be refused with BUSY
 */
extern U8 LoopSpool_Acquire(void);

/** @brief Base of a slot's buffer, or NULL_PTR for a bad index. */
extern U8* LoopSpool_Buffer(const U8 nSlot);

/** @brief Bytes one slot can hold. */
extern U32 LoopSpool_SlotBytes(void);

/** @brief Give a slot back without writing it, e.g. on an aborted transfer. */
extern void LoopSpool_Release(const U8 nSlot);

/**
 * @brief Hand a filled slot to the card. Returns as soon as it is queued.
 *
 * Does NOT block on the write - the spool thread does that. Which slot goes
 * first is decided there, subject to the recorder having priority.
 *
 * @param nCrc   what the transfer session computed, carried so it can be
 *               compared against the audio board's value
 * @param pName  file name, or NULL_PTR for the default
 */
extern STD_RESULT LoopSpool_Commit(const U8 nSlot,
                                   const U32 nBytes,
                                   const U8  nPlaneQty,
                                   const U8  eFormat,
                                   const U32 nCrc,
                                   const char* const pName);

/**
 * @brief Read a WAV from the card into a free slot. BLOCKS.
 *
 * Accepts 48 kHz, 24-bit, mono or stereo PCM. Anything else is refused with a
 * result naming which property was wrong - the file may well be one the user
 * copied from a PC, and "it did not load" is not something they can act on.
 *
 * Refused with RESULT_BUSY while the card is committed to a loop, rather than
 * queued: after a save the lock is free but the recorder is still working off
 * the backlog that save created, and reading now would lock it out again
 * before it had recovered.
 *
 * @param pnSlot  receives the slot the loop landed in
 *
 * @return RESULT_INVALID_PARAM_2 wrong sample rate
 *         RESULT_INVALID_PARAM_3 wrong bit depth
 *         RESULT_INVALID_PARAM_4 wrong channel count
 *         RESULT_INVALID_PARAM_5 will not fit a slot
 *         RESULT_BUSY            card committed, or no free slot
 *         RESULT_NOT_OK          missing, unreadable, or not a RIFF/WAVE file
 */
extern STD_RESULT LoopSpool_Load(const char* const pName, U8* const pnSlot);

/** @brief What is in a slot. NULL_PTR when the slot is free or the index bad. */
extern const LOOPSPOOL_INFO* LoopSpool_Info(const U8 nSlot);

/**
 * @brief TRUE while the card is committed to a loop, INCLUDING the recovery
 *        and anything still queued.
 *
 * The "saving" indicator, and the gate on starting anything else that touches
 * the card. See the header comment for why it does not clear when a file
 * closes.
 */
extern BOOLEAN LoopSpool_IsBusy(void);

/** @brief Loops queued or being written. 0 when the card is free. */
extern U8 LoopSpool_PendingQty(void);

/**
 * @brief The card lock, for the recorder.
 *
 * @param nTimeoutMs  osWaitForever to block until the loop is done
 *
 * @return RESULT_TIMEOUT when it could not be taken in time
 */
extern STD_RESULT LoopSpool_SdLock(const U32 nTimeoutMs);

extern void LoopSpool_SdUnlock(void);

#ifdef __cplusplus
}
#endif

#endif // #ifndef LOOPSPOOL_H

/****************************************** end of file *******************************************/
