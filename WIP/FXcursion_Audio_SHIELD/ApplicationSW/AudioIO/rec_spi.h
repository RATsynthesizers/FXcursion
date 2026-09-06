/**
 * @file      rec_spi.h
 *
 * @details   Transport for the recorder stream: SPI1 master, DMA, one burst per
 *            audio block. The staging and the state machine live in
 *            rec_stream.h; this file is only the HAL.
 *
 *            ------------------------------------------------------------------
 *            THE NUMBERS
 *            ------------------------------------------------------------------
 *
 *            SPI1's kernel clock is the 24.576 MHz crystal through I2S_CKIN, not
 *            a PLL - SPI1/2/3 share one clock selection on the H7 and I2S3 must
 *            have the crystal, so SPI1 gets it too. With the prescaler at 2 the
 *            bit clock is 12.288 Mbit/s. See the clock note in main.c.
 *
 *                stream    4 slots x 32 bit x 48 kHz  =  6.144 Mbit/s
 *                burst     64 frames x 4 slots x 4 B  =  1024 B per block
 *                time      8192 bits / 12.288 Mbit/s  =  667 us
 *                block     64 / 48000                 = 1333 us
 *
 *            So a transfer occupies half of every block period and has always
 *            finished before the next one is staged. That is what makes two
 *            staging blocks enough, and it is why REC_STREAM_STATS.nBlocksDropped
 *            should read zero forever.
 *
 *            ------------------------------------------------------------------
 *            WHY THE INTERFACE ASKS FOR THE STREAM
 *            ------------------------------------------------------------------
 *
 *            Nothing is transmitted until the interface sends
 *            PROTO_CMD_STREAM(enable). It receives into a circular DMA and
 *            de-interleaves by position, so it must have armed that DMA before
 *            the first word arrives - otherwise every recorded channel ends up
 *            in the wrong file, with plausible audio in it. See fx_protocol.h.
 *
 *            ------------------------------------------------------------------
 *            CONCURRENCY
 *            ------------------------------------------------------------------
 *
 *            Two contexts drive the state machine and they are NOT the same
 *            priority: RecSpi_PushBlock runs in the audio interrupt
 *            (IRQ_PRIO_AUDIO_STREAM, 5) and the completion callback runs from
 *            the SPI1 / DMA1 interrupt (IRQ_PRIO_RECORDER, 8). The audio ISR can
 *            therefore preempt the completion callback halfway through, so every
 *            state transition here is inside a PRIMASK critical section - the
 *            same treatment PumpTx gets in ctrl_uart.c, for the same reason.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef REC_SPI_H
#define REC_SPI_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "rec_stream.h"



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Reset the staging layer. Call from AudioSys_Init, before audio runs.
 *
 * Does not transmit anything: the stream stays off until the interface asks.
 */
extern STD_RESULT RecSpi_Init(void);

/**
 * @brief Stage the block the recorder just interleaved and transmit it.
 *
 * Call from the audio ISR, once per block, AFTER the chain has run - the
 * recorder's post-effect taps are written during Grid_Process.
 *
 * Cheap when the stream is off: one flag test and a return.
 */
extern void RecSpi_PushBlock(void);

/** TRUE when the interface has asked for the stream and it is running. */
extern BOOLEAN RecSpi_IsStreaming(void);

/** Transfer counters, for PROTO_DIAG. Never NULL. */
extern const REC_STREAM_STATS* RecSpi_Stats(void);



#endif // #ifndef REC_SPI_H

/****************************************** end of file *******************************************/
