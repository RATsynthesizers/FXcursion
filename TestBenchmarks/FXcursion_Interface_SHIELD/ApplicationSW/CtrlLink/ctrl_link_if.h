/**
 * @file      ctrl_link_if.h
 *
 * @details   The interface controller's end of the control link to the audio
 *            controller: USART2, DMA, and a task that owns both.
 *
 *            The framing is NOT here. InterComProtocol/fx_link.c carries the sync word,
 *            the CRC and the parse state machine, compiled from the same source
 *            as the audio side and covered by that project's host tests. This
 *            file is transport plus meaning: what to send, and what to do with
 *            what comes back.
 *
 *            ------------------------------------------------------------------
 *            THE TRAFFIC IS ALMOST ENTIRELY ONE WAY
 *            ------------------------------------------------------------------
 *
 *            This board SENDS: whole configurations, parameter changes, tempo,
 *            transport, and the recorder stream gate. It ANSWERS nothing - the
 *            audio controller never asks it for anything.
 *
 *            Coming back are only reports: ACK, TELEMETRY, DIAG, PONG. Each is
 *            republished on a pubsub topic and consumed in Model::tick, which
 *            is where the GUI already picks up button and encoder events. The
 *            link task therefore never touches TouchGFX state, and the GUI
 *            never touches a UART.
 *
 *            ------------------------------------------------------------------
 *            NOTHING BLOCKS, ON EITHER SIDE
 *            ------------------------------------------------------------------
 *
 *            Design rule 3 of the protocol, and the thing the old link got
 *            worst: the GUI used to spin on "while (!updateFlag);". Here every
 *            sender queues bytes into a ring and returns, and replies arrive
 *            asynchronously on a topic. A send from a TouchGFX callback cannot
 *            stall a frame.
 *
 *            ------------------------------------------------------------------
 *            RECEIVE IS A CHASED CIRCULAR DMA, NOT AN INTERRUPT PER BYTE
 *            ------------------------------------------------------------------
 *
 *            The DMA fills a circular buffer forever and the task works out how
 *            far it got from NDTR. That means no receive interrupt at all, so
 *            the task can run at a modest priority without risking a dropped
 *            byte, and the ISR never calls a FreeRTOS API.
 *
 *            DMA REACHABILITY: the buffers here are ordinary statics, which
 *            land in .bss - and on this project .bss is in RAM_D1 at
 *            0x24000000, which DMA1 can reach. That is NOT true of DTCM, which
 *            DMA1 and DMA2 cannot address at all. It also relies on the D-cache
 *            being off in this firmware; if it is ever enabled, every DMA buffer
 *            in this project needs an MPU region or cache maintenance,
 *            including these two.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef CTRL_LINK_IF_H
#define CTRL_LINK_IF_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "fx_protocol.h"
#include "fx_link.h"

#ifdef __cplusplus
extern "C" {
#endif



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/** Circular receive buffer. Two maximum frames, so a burst cannot outrun a poll. */
#define CTRL_IF_RX_DMA_BYTES            (256U)

/** Outgoing byte ring. Must hold a configuration frame plus whatever follows it. */
#define CTRL_IF_TX_RING_BYTES           (512U)

/** How often the link task drains the receive DMA, in milliseconds. */
#define CTRL_IF_POLL_MS                 (5U)

/**
 * Telemetry silence after which the audio controller is considered absent.
 *
 * It sends every TELEMETRY_PERIOD_MS (40 ms), so this is many missed frames in
 * a row rather than one unlucky gap.
 */
#define CTRL_IF_PEER_TIMEOUT_MS         (500U)

/**
 * How often to ping while the audio controller is silent.
 *
 * Telemetry arrives unprompted once the audio side is running, so a ping is not
 * needed to detect life - it is needed to tell two very different faults apart
 * during bring-up. Silence alone cannot distinguish "the other board is dead"
 * from "my own receive path is broken"; a PONG coming back proves the whole
 * round trip, both directions, framing and CRC included.
 *
 * Only sent while nothing is being heard, so a healthy link carries no ping
 * traffic at all.
 */
#define CTRL_IF_PING_MS                 (250U)



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Create the topics and start the link task. Call once, from the init thread.
 *
 * Starts the UART receive DMA. Does NOT ask for the recorder stream - that waits
 * for an explicit CtrlLinkIf_Stream once the recorder has been armed.
 */
extern STD_RESULT CtrlLinkIf_Init(void);

/**
 * @brief Send the whole machine configuration.
 *
 * SEND STATE, NOT EDITS: there is no "add effect" command. Whenever anything
 * about the grid changes, the GUI sends all 96 bytes and the audio side rebuilds
 * from them. The audio controller answers with an ACK on PUBSUB_TOPIC_ACK.
 */
extern STD_RESULT CtrlLinkIf_SendConfig(const PROTO_CFG* const pCfg);

/** Send one parameter change. Addressed by (chain, effect type, index). */
extern STD_RESULT CtrlLinkIf_SetParam(const PROTO_SET_PARAM* const pCmd);

extern STD_RESULT CtrlLinkIf_SetTempo(const U16 nBpmX10,
                                      const U8 nBeatsPerBar,
                                      const U8 nBeatUnit);

/** Looper transport, per chain. See PROTO_TRANSPORT_ACT. */
extern STD_RESULT CtrlLinkIf_Transport(const U8 nChain, const U8 eAction);

/**
 * @brief Start or stop the recorder audio stream over SPI.
 *
 * ORDER MATTERS. The stream carries no framing of its own and is
 * de-interleaved by position, so the receive DMA must already be armed with the
 * layout from the last ACK before this is called with TRUE. Getting it backwards
 * records every channel into the wrong file, with plausible audio in it.
 */
extern STD_RESULT CtrlLinkIf_Stream(const BOOLEAN bEnable);

/** Liveness probe. The answer arrives as a PONG and only bumps nFramesOk. */
extern STD_RESULT CtrlLinkIf_Ping(void);

/** FALSE when no telemetry has arrived for CTRL_IF_PEER_TIMEOUT_MS. */
extern BOOLEAN CtrlLinkIf_IsPeerAlive(void);

/**
 * @brief The most recent ACK, or RESULT_NOT_OK if none has arrived.
 *
 * Cached here rather than only published, because the recorder needs the slot
 * map and the stream width whenever it next arms - not merely if some screen
 * happened to be watching the topic when the ACK came in.
 */
extern STD_RESULT CtrlLinkIf_GetAck(PROTO_ACK* const pAck);

/** Framing statistics from the shared parser. Never NULL. */
extern const FX_LINK_STATS* CtrlLinkIf_Stats(void);



#ifdef __cplusplus
}
#endif

#endif // #ifndef CTRL_LINK_IF_H

/****************************************** end of file *******************************************/
