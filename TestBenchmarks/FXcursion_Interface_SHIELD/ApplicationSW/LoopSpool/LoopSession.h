/***************************************************************************************************
* @file     LoopSession.h
*
* @brief    The interface half of a loop transfer, end to end.
*
*           ------------------------------------------------------------------
*           WHERE THIS SITS
*           ------------------------------------------------------------------
*
*           Four things already exist and do not talk to each other:
*
*             fx_loop.c     the session state machine, run by BOTH boards
*             Recorder.c    the MDMA route that lands loop slots in memory
*             LoopSpool.c   the staging slots and the card
*             ctrl_link_if  the UART that carries LOOP_OPEN / CTL / STAT
*
*           This is what connects them. It owns the FX_LOOP_SESSION on this
*           side, decides which staging slot a transfer uses, arms and disarms
*           the MDMA route around it, and hands the finished loop to the
*           spooler.
*
*           It is the mirror of loop_xfer.c on the audio board, deliberately -
*           the two run the same shared state machine, so a change to the
*           protocol shows up as a compile error on both ends rather than as a
*           transfer that stalls.
*
*           ------------------------------------------------------------------
*           THE INTERFACE INITIATES, IN BOTH DIRECTIONS
*           ------------------------------------------------------------------
*
*           It owns the card and the staging memory, so it is the only side
*           that knows whether there is a file and whether there is room. On a
*           SAVE the audio board still decides the LENGTH - it is the only one
*           that knows how long the take is - which is why LOOP_OPEN may ask
*           with a byte count of zero and be told the answer.
*
*               SAVE   acquire a slot, arm the route, LOOP_OPEN
*                      <- LOOP_STAT(READY, bytes, slots)
*                      LOOP_CTL(START), stream widens, blocks land
*                      <- LOOP_STAT(COMPLETE, crc), compare, spool to card
*
*               LOAD   read the WAV into a slot, LOOP_OPEN with its size
*                      <- LOOP_STAT(READY)
*                      LOOP_CTL(START), blocks go out on MISO
*                      <- LOOP_STAT(COMPLETE, crc), compare
*
*           ------------------------------------------------------------------
*           THE ARMING ORDER IS NOT ARBITRARY
*           ------------------------------------------------------------------
*
*           Route armed BEFORE the audio board is told to start, disarmed
*           BEFORE it is told to stop. The stream carries no framing, so a
*           block that arrives with nothing routed is not recoverable and a
*           route left armed after the stream narrows writes whatever the wire
*           leaves in slots nobody is filling. Both failures are silent.
*
***************************************************************************************************/

#ifndef LOOPSESSION_H
#define LOOPSESSION_H

/***************************************************************************************************
* Included header files
***************************************************************************************************/

#include "general.h"
#include "fx_loop.h"
#include "LoopSpool.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/** @brief Reset the session. Call once, after LoopSpool_Init. */
extern STD_RESULT LoopSession_Init(void);

/**
 * @brief Begin saving a looper to the card.
 *
 * Returns as soon as the request is on the wire; the transfer and the card
 * write both happen afterwards. Watch LoopSession_IsBusy, or LoopSpool_IsBusy
 * for the card specifically.
 *
 * @param nLooper    0 .. LOOPER_QTY-1
 * @param nPlaneQty  1 mono, 2 stereo
 * @param pName      file name, or NULL_PTR for the default
 *
 * @return RESULT_BUSY when a transfer is already running or no slot is free
 */
extern STD_RESULT LoopSession_StartSave(const U8 nLooper,
                                        const U8 nPlaneQty,
                                        const char* const pName);

/**
 * @brief Begin loading a WAV from the card into a looper.
 *
 * The read happens FIRST and blocks - a file has to be in memory before its
 * size can be negotiated, and the audio board must be told how much is coming
 * before any of it arrives.
 *
 * @return whatever LoopSpool_Load returned when the file was the problem, so
 *         "wrong sample rate" reaches the caller intact
 */
extern STD_RESULT LoopSession_StartLoad(const char* const pName,
                                        const U8 nLooper);

/** @brief Handle PROTO_CMD_LOOP_STAT from the audio board. */
extern void LoopSession_OnStat(const PROTO_LOOP_STAT* const pStat);

/**
 * @brief Drive the transfer. Call from the link thread.
 *
 * Watches how much the MDMA route has actually landed and closes the session
 * out when it reaches the agreed size. Polled rather than interrupt-driven on
 * purpose: the completion does bulk work - a CRC over several megabytes, then
 * a spool commit - and none of that belongs in a DMA callback.
 */
extern void LoopSession_Poll(void);

/**
 * @brief LOAD: fill one half of the SPI transmit ring with loop bytes.
 *
 * Called from the SPI half callback - INTERRUPT CONTEXT - for the half the
 * master has just finished clocking out, while it clocks the other. Filling the
 * half being clocked would put a partly written frame on the wire, and a
 * positionally framed stream has nothing to notice that with.
 *
 * The exact mirror of LoopXfer_Block on the audio board: four payload bytes per
 * slot, loop slots only, recorder slots left alone. When no load is running it
 * zeroes the loop slots instead - a ring left holding the tail of a finished
 * transfer would keep sending it.
 *
 * @param pFrames  first frame of the half
 * @param nFrames  frames in the half
 * @param nStride  slots per frame
 */
extern void LoopSession_FillTx(S32* const pFrames,
                               const U32 nFrames,
                               const U8  nStride);

/** @brief TRUE while a transfer is negotiating or moving. */
extern BOOLEAN LoopSession_IsBusy(void);

/** @brief The session, for a status screen. */
extern const FX_LOOP_SESSION* LoopSession_Get(void);

/**
 * @brief Transfers that failed, and why the last one did.
 *
 * A CRC mismatch here is the number that matters: it means the link delivered
 * something other than what was sent, which at 96 MHz over flying leads is the
 * first thing to suspect.
 */
extern U32 LoopSession_Failures(void);
extern U8  LoopSession_LastResult(void);

#ifdef __cplusplus
}
#endif

#endif // #ifndef LOOPSESSION_H

/****************************************** end of file *******************************************/
