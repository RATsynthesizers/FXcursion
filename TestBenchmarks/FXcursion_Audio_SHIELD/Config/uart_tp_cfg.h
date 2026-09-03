/**
 * @file      uart_tp_cfg.h
 *
 * @details   UART transport configuration - AUDIO controller.
 *
 *            USART2 carries commands and telemetry between the two
 *            controllers. It is the ONLY control path: the SPI link is a
 *            positionally framed sample stream with no framing, no
 *            acknowledgement and no reply, so every request, every answer and
 *            every diagnostic goes over this.
 *
 *            Copied from SystemSW/Services/UART_TP/uart_tp_cfg.h.tmpl.
 *
 * @version   1.0.0
 *
 * \date      04.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef UART_TP_CFG_H
#define UART_TP_CFG_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

#define UART_TP_IN_USE                  (ON)

/**
 * USART2 on BOTH controllers.
 *
 * MOVED FROM USART1. The interface controller was already on USART2, and
 * having the two ends on differently numbered peripherals is a standing
 * invitation to cross the control UART with a debug one on the bench.
 *
 * USART1 is now free for the per-board debug console.
 */
#define UART_TP_INSTANCE                (UART_TP_USART2)

/**
 * Both ends must match.
 *
 * Telemetry is 54 bytes at 25 Hz - 1350 B/s against 92 160 B/s, about 1.5%. The
 * speed is not chosen for throughput; it is chosen so a command and its answer
 * cost well under a millisecond, because the UI waits on them.
 *
 * WHAT TO WATCH: both boards run from the internal oscillator with default
 * calibration. 8N1 tolerates roughly +-2% between the two ends and an
 * uncalibrated HSI can be +-1% on its own at temperature, so the margin is
 * real but not large. This link now carries ALL control traffic, so a dropped
 * command is a functional failure rather than a missed telemetry frame - if
 * UART_TP_STATS.nUartErrors climbs, suspect the clocks before the cable.
 */
#define UART_TP_BAUDRATE                (921600UL)

/**
 * 1024 bytes is 11 ms of slack at this baud rate (92 bytes per millisecond).
 * Sized from the longest gap between reads, not from the frame size.
 */
#define UART_TP_RX_RING_BYTES           (1024U)

/** A few of the largest frames. PROTO_FRAME_MAX is 102 bytes. */
#define UART_TP_TX_QUEUE_BYTES          (512U)

/**
 * Where the transport's DMA buffers are placed.
 *
 * This board calls the section .ramd2dma; the interface controller calls its
 * equivalent .dma_buffers. That difference is exactly why the service takes the
 * attribute from here instead of reaching for a project macro - it would build
 * in one firmware and not the other.
 */
#define UART_TP_DMA_SECTION \
    __attribute__((section(".ramd2dma"), used, aligned(32)))



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/// None.



#endif  // #ifndef UART_TP_CFG_H

/****************************************** end of file *******************************************/
