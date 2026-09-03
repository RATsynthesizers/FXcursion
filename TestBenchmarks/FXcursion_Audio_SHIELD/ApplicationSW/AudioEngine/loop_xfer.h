/***************************************************************************************************
* @file     loop_xfer.h
*
* @brief    The audio side of the loop transport.
*
*           InterComProtocol/fx_loop.c is the session state machine and BOTH boards run
*           it. This file is what the audio board wraps around it: it answers
*           PROTO_CMD_LOOP_OPEN against the looper memory it actually has, and
*           it moves bytes between that memory and the recorder stream's extra
*           slots, one audio block at a time.
*
*           ------------------------------------------------------------------
*           HOW THE BULK GETS ONTO THE WIRE
*           ------------------------------------------------------------------
*
*           It does not get its own SPI transaction. The recorder stream is a
*           continuous positional interleave received into a circular DMA on the
*           interface, and re-arming that DMA between bursts is exactly what
*           rotates recorded channels into the wrong files - the hazard
*           PROTO_STREAM already documents.
*
*           So the frame WIDENS. For the life of a session the stream carries
*           REC_SLOT_QTY recorder slots followed by nSlotQty loop slots, and
*           narrows again when the session ends. The loop slots are contiguous,
*           which is what makes them one MDMA route on the far side rather than
*           one per slot.
*
*           Per block that is nSlotQty * 4 bytes of wire for
*           nSlotQty * 4 * 3/4 bytes of payload, because the wire slot is 32-bit
*           and a loop sample is 24.
*
*           ------------------------------------------------------------------
*           WHO STARTS IT, AND THE ORDER
*           ------------------------------------------------------------------
*
*           The interface initiates in both directions - it owns the card and
*           the staging memory. The arming order matters for the same reason it
*           does for the recorder stream: the receiver has to be routed before
*           the sender starts, or the first blocks land somewhere else.
*
*               interface -> LOOP_OPEN     audio validates against its looper
*               audio     -> LOOP_STAT     byte count and slots granted
*               interface reprograms its MDMA routing for the wider frame
*               interface -> LOOP_CTL(START)
*               audio      widens the stream, feeds slots every block
*               audio     -> LOOP_STAT(COMPLETE, crc)
*
***************************************************************************************************/

#ifndef LOOP_XFER_H
#define LOOP_XFER_H

/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"

#include "fx_defs.h"
#include "fx_protocol.h"
#include "fx_loop.h"

/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/** @brief Reset the session. Call once at start-up. */
extern STD_RESULT LoopXfer_Init(void);

/**
 * @brief Handle PROTO_CMD_LOOP_OPEN and produce the PROTO_LOOP_STAT to reply.
 *
 * Sizes a SAVE from the looper's recorded length, and refuses a LOAD that will
 * not fit a plane buffer. pStat is filled on every path including refusals -
 * the interface is holding staging open waiting for an answer.
 *
 * @param nLoopFrames  frames currently recorded in the addressed looper, 0 when
 *                     it is empty
 */
extern STD_RESULT LoopXfer_OnOpen(const PROTO_LOOP_OPEN* const pOpen,
                                  const U32 nLoopFrames,
                                  PROTO_LOOP_STAT* const pStat);

/** @brief Handle PROTO_CMD_LOOP_CTL - start or abort. */
extern STD_RESULT LoopXfer_OnCtl(const PROTO_LOOP_CTL* const pCtl);

/**
 * @brief Slots this block's frame carries, total.
 *
 * REC_SLOT_QTY when no session is running, so the stream builder can use it
 * unconditionally as "the width right now".
 */
extern U8 LoopXfer_StreamWidth(void);

/**
 * @brief Fill this block's loop slots from the looper, or drain them into it.
 *
 * Called once per audio block from the stream builder, AFTER the recorder slots
 * are filled and only when LoopXfer_StreamWidth() exceeds REC_SLOT_QTY.
 *
 * @param pSlots   the loop slots of this block's frame - nSlotQty S32 words per
 *                 frame, AUDIO_BLOCK_FRAMES frames
 * @param nFrames  frames in the block
 *
 * @return RESULT_OK while the session is healthy
 */
extern STD_RESULT LoopXfer_Block(S32* const pSlots, const U32 nFrames);

/** @brief TRUE when a transfer is moving bytes this block. */
extern BOOLEAN LoopXfer_IsRunning(void);

/**
 * @brief The session, for reporting a completion or a failure upstream.
 */
extern const FX_LOOP_SESSION* LoopXfer_Session(void);

/**
 * @brief Fill a PROTO_LOOP_STAT from the current session.
 */
extern void LoopXfer_Report(PROTO_LOOP_STAT* const pStat);

#endif // #ifndef LOOP_XFER_H

/****************************************** end of file *******************************************/
