/**
 * @file      uart_tp.h
 *
 * @details   UART byte transport.
 *
 *            ------------------------------------------------------------------
 *            WHAT THIS IS, AND WHAT IT IS NOT
 *            ------------------------------------------------------------------
 *
 *            A byte pipe over one UART, and nothing else. It owns the
 *            peripheral, the DMA, the interrupt callbacks and a receive ring.
 *            It does NOT know about frames, sync words, lengths, CRCs or
 *            commands - that belongs to whatever protocol layer sits on top,
 *            and keeping it out of here is what lets the same transport carry
 *            different protocols in different projects.
 *
 *            ------------------------------------------------------------------
 *            ONE TRANSPORT PER INTERFACE
 *            ------------------------------------------------------------------
 *
 *            Deliberately a singleton rather than a handle-based API. There is
 *            exactly one UART transport per firmware, chosen in
 *            uart_tp_cfg.h, so a handle parameter would be a pointer that can
 *            only ever have one value - and every call site would have to know
 *            where that value lives. Instead the configuration names the
 *            peripheral and this module resolves it to the CubeMX handle at
 *            compile time.
 *
 *            To move the transport to a different UART, change
 *            UART_TP_INSTANCE in uart_tp_cfg.h. Nothing else changes.
 *
 *            ------------------------------------------------------------------
 *            RECEIVE: CIRCULAR DMA, DRAINED BY THE CALLER
 *            ------------------------------------------------------------------
 *
 *            Reception is a circular DMA into a ring this module owns, and the
 *            caller drains it with UART_TP_Read whenever it likes. That
 *            combination is what makes the link robust against a slow reader:
 *            the DMA never stops, never needs re-arming, and cannot be caught
 *            between transfers - which is where a byte goes missing and a
 *            framed protocol spends the next frame resynchronising.
 *
 *            The cost is that a reader slow enough to be lapped loses the
 *            OLDEST bytes rather than the newest. UART_TP_Read reports that as
 *            an overrun rather than silently returning a spliced stream,
 *            because a protocol layer must resynchronise deliberately rather
 *            than discover it from a bad CRC.
 *
 * @version   1.0.0
 *
 * \date      04.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef UART_TP_H
#define UART_TP_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"

// Get the project's choice of peripheral, baud rate and buffer sizes
#include "uart_tp_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/* Peripheral selectors for UART_TP_INSTANCE. Numbered rather than symbolic so
   the choice can be compared with #if in the implementation, which is how the
   CubeMX handle and the IRQ number get resolved without a runtime table. */
#define UART_TP_USART1                  (1U)
#define UART_TP_USART2                  (2U)
#define UART_TP_USART3                  (3U)
#define UART_TP_UART4                   (4U)
#define UART_TP_UART5                   (5U)
#define UART_TP_USART6                  (6U)
#define UART_TP_UART7                   (7U)
#define UART_TP_UART8                   (8U)



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief Transport counters. All should stay at zero on a healthy link.
 *
 * Kept separate from any protocol-level statistics on purpose: these say
 * whether the WIRE is working, and a protocol layer's own counters say whether
 * the other end is talking sense. Confusing the two makes a noisy cable look
 * like a firmware bug.
 */
typedef struct stUART_TP_STATS
{
    /** Bytes the DMA overwrote before the caller read them. Non-zero means the
        reader is too slow, or a burst was larger than UART_TP_RX_RING_BYTES. */
    U32 nRxOverruns;

    /** Sends refused because the transmit queue was full. */
    U32 nTxDropped;

    /** HAL error callbacks: framing, parity, noise, receiver overrun. A cable
        or a baud-rate mismatch shows up here first. */
    U32 nUartErrors;

    /** Bytes handed to the caller since init, and bytes accepted for send. */
    U32 nRxBytes;
    U32 nTxBytes;

} UART_TP_STATS;



/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Take over the configured UART and start receiving.
 *
 * Reconfigures the peripheral to UART_TP_BAUDRATE, so the configuration here
 * wins over whatever CubeMX generated. That is deliberate: the baud rate is a
 * property of the LINK, which both ends have to agree on, and it should be
 * changeable in one place rather than in a generated file that a regeneration
 * will overwrite.
 *
 * @return RESULT_NOT_OK if the peripheral or its DMA refused to start
 */
extern STD_RESULT UART_TP_Init(void);

/** @brief Stop the transport and release the peripheral. */
extern STD_RESULT UART_TP_DeInit(void);

/**
 * @brief Queue bytes for transmission. Returns immediately.
 *
 * Copies into an internal queue and starts the DMA if it is idle, so the
 * caller's buffer is free on return and a caller in an interrupt does not have
 * to wait for the wire. A send larger than the free space in the queue is
 * refused WHOLE rather than partially accepted - a protocol layer cannot do
 * anything useful with half a frame on the wire, so it is better to drop it and
 * count it.
 *
 * @return RESULT_BUSY when the queue cannot take all of it
 */
extern STD_RESULT UART_TP_Send(const U8* const pData, const U16 nLength);

/**
 * @brief Drain received bytes into the caller's buffer.
 *
 * Safe to call as often as the caller likes, including when nothing has
 * arrived. Returns 0 in that case rather than blocking.
 *
 * @param pnOverrun  optional; set TRUE when bytes were lost BEFORE the ones
 *                   being returned, so the caller can resynchronise
 *                   deliberately instead of discovering it from a bad CRC
 *
 * @return bytes written to pBuf
 */
extern U16 UART_TP_Read(U8* const pBuf, const U16 nMaxLength, BOOLEAN* const pnOverrun);

/** @brief Bytes waiting to be read. */
extern U16 UART_TP_RxPending(void);

/** @brief TRUE while a transmission is in flight. */
extern BOOLEAN UART_TP_IsTxBusy(void);

/** @brief Copy out the transport counters. */
extern void UART_TP_GetStats(UART_TP_STATS* const pStats);

/**
 * @brief Service the transport from a periodic context.
 *
 * Only needed to restart a transmission that could not be chained from the
 * completion callback. Harmless to call, and harmless not to - but calling it
 * turns a dropped transmit into a delayed one.
 */
extern void UART_TP_Poll(void);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef UART_TP_H

/****************************************** end of file *******************************************/
