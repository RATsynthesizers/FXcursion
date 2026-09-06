/**
 * @file      spi_tp_cfg.h
 *
 * @details   SPI transport configuration - AUDIO controller.
 *
 *            SPI1 carries the recorder stream and, during a transfer, the loop
 *            bulk as extra slots in the same frame. This board is the MASTER:
 *            it owns the timing, because the frame goes when the audio block
 *            says so rather than when a clock ticks.
 *
 *            Copied from SystemSW/Services/SPI_TP/spi_tp_cfg.h.tmpl.
 *
 * @version   1.0.0
 *
 * \date      04.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef SPI_TP_CFG_H
#define SPI_TP_CFG_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

#define SPI_TP_IN_USE                   (ON)

#define SPI_TP_INSTANCE                 (SPI_TP_SPI1)

/** Driving end. One frame per audio block, sent from the block itself. */
#define SPI_TP_ROLE                     (SPI_TP_ROLE_MASTER)

/**
 * MUST MATCH THE INTERFACE CONTROLLER. This is the wire.
 *
 * 96 MHz, not 48: the frame widens during a loop transfer and the burst has to
 * finish inside the 1333 us audio block. At 48 MHz a 20-slot frame occupies 64%
 * of every block for the whole of a transfer; at 96 MHz it is 32%. The full
 * reasoning, with the table, is in the interface controller's copy of this
 * file - it is the same wire, so the argument only wants writing down once.
 */
#define SPI_TP_SCK_HZ                   (96000000UL)

/**
 * PLL3P, via Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL3.
 *
 *   HSI 64 MHz / PLL3M 8 = 8 MHz  ->  x PLL3N 96 = 768 MHz VCO  ->  / PLL3P 4
 *
 * 192 MHz, so the prescaler is /2 and SCK is exactly 96 MHz with no rounding.
 *
 * THIS REPLACES RCC_SPI123CLKSOURCE_PIN. SPI1 used to inherit I2S_CKIN at
 * 24.576 MHz, which with the H7's minimum /2 prescaler capped SCK at
 * 12.288 MHz - half of which the recorder alone consumed, leaving no room for
 * loop slots at all. Nothing depended on that: the sample-lock that matters is
 * on SAI1/SAI23, which keep their own _PIN selection and are unaffected.
 *
 * The interface deliberately runs a HIGHER kernel (240 MHz) because a slave
 * needs margin over SCK. Only SCK has to match.
 */
#define SPI_TP_KERNEL_HZ                (192000000UL)



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/// None.



#endif  // #ifndef SPI_TP_CFG_H

/****************************************** end of file *******************************************/
