/**
 * @file      spi_tp.h
 *
 * @details   SPI bulk transport - a continuous, positionally framed stream.
 *
 *            ------------------------------------------------------------------
 *            WHAT THIS CARRIES, AND WHY IT IS NOT A MESSAGE TRANSPORT
 *            ------------------------------------------------------------------
 *
 *            High-rate sample data, sent as one fixed-size frame per producer
 *            period, forever. There is no sync word, no length and no CRC: the
 *            receiver knows where it is in the frame by COUNTING, because the
 *            frame is the same size every time and the DMA never stops.
 *
 *            That makes it fast and it makes it fragile in one specific way:
 *            LOSE A SINGLE WORD AND EVERY SLOT IS PERMANENTLY ROTATED. The
 *            stream keeps flowing, the data keeps looking plausible, and every
 *            channel is in the wrong place from then on. It cannot be detected
 *            from the payload, which is why the receiver arms its DMA FIRST and
 *            the transmitter is told to start afterwards - see SPI_TP_Start.
 *
 *            For anything that needs framing, acknowledgement or a reply, use
 *            the UART transport instead. This one is for the case where the
 *            data rate makes per-message overhead unaffordable.
 *
 *            ------------------------------------------------------------------
 *            ONE TRANSPORT, TWO ROLES
 *            ------------------------------------------------------------------
 *
 *            The same service compiles for both ends. SPI_TP_ROLE picks which:
 *
 *              MASTER  drives SCK and sends one frame per call to
 *                      SPI_TP_SendFrame. It owns the timing - the frame goes
 *                      when the producer says so, not when a clock ticks.
 *
 *              SLAVE   receives into a circular ring armed once, and calls back
 *                      at each half. It never re-arms, because the gap between
 *                      transfers is exactly where a word goes missing.
 *
 *            ------------------------------------------------------------------
 *            SPEED
 *            ------------------------------------------------------------------
 *
 *            SPI_TP_SCK_HZ is the wanted bit clock and SPI_TP_KERNEL_HZ is what
 *            the RCC actually feeds the peripheral. The prescaler is derived
 *            from the two at compile time and the result is checked, so a
 *            requested clock the divider cannot produce is a build error naming
 *            both numbers rather than a link that silently runs at half speed.
 *
 * @version   1.0.0
 *
 * \date      04.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef SPI_TP_H
#define SPI_TP_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"

// Get the project's choice of peripheral, role and clock
#include "spi_tp_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/* Peripheral selectors for SPI_TP_INSTANCE. */
#define SPI_TP_SPI1                     (1U)
#define SPI_TP_SPI2                     (2U)
#define SPI_TP_SPI3                     (3U)
#define SPI_TP_SPI4                     (4U)
#define SPI_TP_SPI5                     (5U)
#define SPI_TP_SPI6                     (6U)

/* Role selectors for SPI_TP_ROLE. */
#define SPI_TP_ROLE_MASTER              (0U)
#define SPI_TP_ROLE_SLAVE               (1U)



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief Called when the receive ring reaches a half boundary. SLAVE only.
 *
 * Runs in DMA interrupt context, so it must do routing and nothing else - no
 * file I/O, no blocking, no allocation. It exists to hand the just-filled half
 * to whatever de-interleaves it.
 *
 * @param bSecondHalf FALSE when the FIRST half has just filled, TRUE for the
 *                    second. The consumer takes the half the DMA is NOT in.
 */
typedef void (*SPI_TP_HalfCallback)(const BOOLEAN bSecondHalf);

/** @brief Called when a master frame has gone out. MASTER only. */
typedef void (*SPI_TP_SentCallback)(void);

/**
 * @brief Transport counters. All should stay at zero on a healthy link.
 */
typedef struct stSPI_TP_STATS
{
    /** Frames the master handed to the peripheral. The only way to tell an
        idle link from a link carrying silence. */
    U32 nFramesSent;

    /** Sends refused because the previous frame had not finished. Non-zero
        means the frame does not fit the producer period at this clock, and
        samples are being dropped. */
    U32 nFramesDropped;

    /** Half-boundaries the slave has seen. */
    U32 nHalvesReceived;

    /** HAL error callbacks: overrun, mode fault, CRC. */
    U32 nSpiErrors;

} SPI_TP_STATS;



/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Take over the configured SPI and apply the configured clock.
 *
 * Does NOT start the stream. Arming and starting are separate on purpose: see
 * SPI_TP_Start.
 *
 * @return RESULT_NOT_OK if the peripheral refused to initialise
 */
extern STD_RESULT SPI_TP_Init(void);

/** @brief Stop the stream and release the peripheral. */
extern STD_RESULT SPI_TP_DeInit(void);

/**
 * @brief SLAVE: arm the circular receive ring. Call before the master starts.
 *
 * THE ORDER MATTERS AND IT IS NOT SYMMETRICAL. The receiver must be armed
 * before the first word arrives, because a positionally framed stream joined
 * mid-frame is rotated for as long as it runs, with no way to notice. The
 * master is told to begin only after this returns.
 *
 * @param pRing    ring buffer, in memory a DMA can reach
 * @param nWords   ring size in WORDS of the configured data size; must be even
 *                 so the half boundary falls on a word
 */
extern STD_RESULT SPI_TP_StartReceive(void* const pRing, const U16 nWords);

/**
 * @brief MASTER: send one frame. Returns immediately.
 *
 * Refused with RESULT_BUSY when the previous frame is still going out, which
 * means the frame no longer fits the producer period - the caller should count
 * it as a dropped block rather than wait, because waiting inside an audio
 * interrupt turns one late frame into a stall.
 *
 * @param pWords  frame data, in memory a DMA can reach
 * @param nWords  words of the configured data size
 */
extern STD_RESULT SPI_TP_SendFrame(const void* const pWords, const U16 nWords);

/** @brief SLAVE: called at each half of the receive ring. */
extern STD_RESULT SPI_TP_RegisterHalfCb(const SPI_TP_HalfCallback pfCb);

/** @brief MASTER: called when a frame has finished going out. */
extern STD_RESULT SPI_TP_RegisterSentCb(const SPI_TP_SentCallback pfCb);

/** @brief TRUE while a master frame is in flight. */
extern BOOLEAN SPI_TP_IsBusy(void);

/** @brief The bit clock this build actually produces, Hz.
 *
 *  Derived from the kernel clock and the chosen prescaler, so it is what the
 *  wire will really run at - not what was asked for. Worth reporting in a
 *  diagnostic frame: a link that came up at half the intended speed otherwise
 *  looks like a slow SD card. */
extern U32 SPI_TP_ActualSckHz(void);

/** @brief Copy out the transport counters. */
extern void SPI_TP_GetStats(SPI_TP_STATS* const pStats);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef SPI_TP_H

/****************************************** end of file *******************************************/
