/**
 * @file      ctrl_uart.h
 *
 * @details   The control link's transport: USART1, both directions on DMA.
 *
 *            ------------------------------------------------------------------
 *            WHY THIS IS A SEPARATE FILE FROM ctrl_link.c
 *            ------------------------------------------------------------------
 *
 *            Same split as AudioIO. ctrl_link.c is the protocol - framing, CRC,
 *            parsing, dispatch - and touches no HAL header, so the whole of it
 *            runs and is tested on the host. This file is the part that cannot
 *            be: DMA streams, ring buffers in a particular RAM, and a tick.
 *
 *            The seam is one function pointer. ctrl_link calls a CTRL_TX_FN to
 *            get bytes out and has no idea what a UART is.
 *
 *            ------------------------------------------------------------------
 *            RECEIVE
 *            ------------------------------------------------------------------
 *
 *            Circular DMA into a buffer that is never stopped or restarted, and
 *            a read pointer chased along by the super-loop. No interrupt, no
 *            idle-line detection, no state machine: the DMA controller keeps
 *            writing and CtrlUart_Service works out how far it has got by
 *            reading NDTR.
 *
 *            That makes the receive path immune to being late. Falling behind
 *            costs latency, not bytes, until the ring actually laps - and
 *            nRxNearFull is there to say so before it does.
 *
 *            ------------------------------------------------------------------
 *            TRANSMIT
 *            ------------------------------------------------------------------
 *
 *            A byte ring, drained by DMA one contiguous run at a time.
 *
 *            A single frame buffer would be simpler and would drop frames: an
 *            ACK for a configuration is generated in the middle of the
 *            super-loop and a telemetry frame may well already be going out.
 *            At 115200 baud a telemetry frame occupies the link for 4.7 ms, so
 *            that collision is not rare, it is normal. The ACK is the one frame
 *            in the protocol that must not be lost - the interface waits for it
 *            before trusting the sample stream layout.
 *
 *            ------------------------------------------------------------------
 *            THE BUFFERS ARE IN RAM_D2, AND THAT IS NOT A STYLE CHOICE
 *            ------------------------------------------------------------------
 *
 *            ctrl_link.c assembles frames in DTCM, which DMA1 and DMA2 cannot
 *            address at all. So the transmit callback COPIES into the D2 ring
 *            rather than handing the DMA a pointer to ctrl_link's buffer. A
 *            pointer would compile, link, and transmit nothing.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef CTRL_UART_H
#define CTRL_UART_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"

#include "audio_cfg.h"
#include "fx_protocol.h"



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Bind the protocol to USART1 and start receiving.
 *
 * @param bLoopMemOk  whether the loop SDRAM answered a pattern write/read at
 *                    boot.
 *
 *                    Passed in rather than asked for: the check is destructive,
 *                    so it can only run before any loop audio exists. Reported
 *                    in PROTO_DIAG so a board with an unpopulated or dead bank
 *                    says so, instead of looking healthy until somebody
 *                    presses record.
 */
extern STD_RESULT CtrlUart_Init(const BOOLEAN bLoopMemOk);

/**
 * @brief Super-loop hook: drain the receiver, pump the transmitter, and send
 *        telemetry when it is due.
 *
 * Call as often as possible. Everything the control link does happens here;
 * nothing of it runs in the audio interrupt.
 */
extern void CtrlUart_Service(void);

/** TRUE once at least one well formed frame has arrived. */
extern BOOLEAN CtrlUart_IsLinked(void);

/** Frames that would not fit the transmit ring. Should stay zero. */
extern U32 CtrlUart_TxDropped(void);

/** Times the receive ring was more than three quarters full at a poll. */
extern U32 CtrlUart_RxNearFull(void);

/** USART error interrupts - overrun, framing, noise. */
extern U32 CtrlUart_UartErrors(void);



#endif // #ifndef CTRL_UART_H

/****************************************** end of file *******************************************/
