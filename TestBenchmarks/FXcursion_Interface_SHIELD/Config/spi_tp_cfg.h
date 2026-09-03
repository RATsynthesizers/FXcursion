/**
 * @file      spi_tp_cfg.h
 *
 * @details   SPI transport configuration - INTERFACE controller.
 *
 *            SPI1 carries the recorder stream and, during a transfer, the loop
 *            bulk as extra slots in the same frame. This board is the SLAVE:
 *            the audio controller owns the timing, because the frame goes when
 *            the audio block says so.
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

/** Receiving end. The audio controller drives SCK. */
#define SPI_TP_ROLE                     (SPI_TP_ROLE_SLAVE)

/**
 * ======== WHY 96 MHz AND NOT 48 ========
 *
 * MUST MATCH THE AUDIO CONTROLLER. This is the wire.
 *
 * The frame widens during a loop transfer: REC_SLOT_QTY recorder slots plus up
 * to FX_LOOP_SLOT_QTY_MAX loop slots, 256 bytes per slot per audio block. The
 * burst has to FINISH inside the 1333 us block, or the next block's data has
 * nowhere to go and the block is dropped - silently, in both the recording and
 * the loop, because a positionally framed stream has nothing to report it with.
 *
 * Burst as a share of the block:
 *
 *     frame width        48 MHz     96 MHz
 *     4  (rec only)         13%         6%
 *     12 (+8 loop)          38%        19%
 *     20 (+16 loop)         64%        32%
 *     24 (+20 loop)         77%        38%
 *
 * 48 MHz would work on paper. It is rejected because 64-77% is not a margin, it
 * is a cliff, and it is SUSTAINED for the several seconds a loop transfer takes
 * - concurrently with the DSP at full tilt and both SDRAM banks serving a
 * looper alongside delay or reverb. Any one block whose burst starts late eats
 * into what is left, and the cost of being wrong is dropped audio that cannot
 * be recovered. At 96 MHz the same transfer leaves 62-68% of every block free.
 *
 * The SD card cannot absorb more than about 3-4 MB/s, so 96 MHz is not chosen
 * for throughput - the loop transfer is deliberately slower than the link. It
 * is chosen for HEADROOM inside the block.
 */
#define SPI_TP_SCK_HZ                   (96000000UL)

/**
 * PLL3P, via Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL3.
 *
 *   HSI 64 MHz / PLL3M 8 = 8 MHz  ->  x PLL3N 120 = 960 MHz VCO  ->  / PLL3P 4
 *
 * 240 MHz, NOT the 192 MHz the master runs.
 *
 * A slave resynchronises the incoming clock into its own kernel domain, so the
 * kernel must be at least twice SCK. At exactly 2x - which 192 MHz would give
 * against a 96 MHz wire - there is no margin left for cable delay or the
 * master's clock-to-out, and the failure is silent corruption of a stream with
 * no CRC to catch it. 240 MHz gives 2.5x.
 *
 * The two ends therefore run DIFFERENT kernel clocks on purpose. Only SCK has
 * to match.
 */
#define SPI_TP_KERNEL_HZ                (240000000UL)



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
