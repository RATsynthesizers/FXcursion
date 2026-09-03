/**
 * @file      chan_map.h
 *
 * @details   Between two stereo DMA buffers and one four-channel block.
 *
 *            ------------------------------------------------------------------
 *            WHAT THIS SOLVES
 *            ------------------------------------------------------------------
 *
 *            The engine wants one interleaved four-channel block:
 *
 *                pBlock[frame * 4 + plane]
 *
 *            The hardware does not provide one. Two converters means two SAI
 *            blocks, each with its own DMA buffer of two slots:
 *
 *                aSai1Rx[frame * 2 + slot]      planes 0, 1
 *                aSai2Rx[frame * 2 + slot]      planes 2, 3
 *
 *            Gather merges them, Scatter splits them again. 512 words moved per
 *            block, about a microsecond of 1333 - the price of keeping the
 *            engine free of any knowledge of how many converters the board has.
 *
 *            ------------------------------------------------------------------
 *            AND WHAT ELSE IT SOLVES, WHICH MATTERS MORE
 *            ------------------------------------------------------------------
 *
 *            THE SAI DOES NOT SIGN EXTEND.
 *
 *            In 24-bit mode with 32-bit slots the SAI stores received data
 *            RIGHT ALIGNED in the 32-bit data register, bits 23:0, with the top
 *            byte left as zero. Read that straight into an S32 and every
 *            negative sample - half of all audio - reads as a large positive
 *            number instead.
 *
 *            The symptom is not silence or noise. It is full-scale buzz on
 *            anything but the quietest signal, and it looks like a codec
 *            problem, or a clock problem, or a wiring problem. It is none of
 *            those. It is these six lines, and they are tested on the host.
 *
 *            Going the other way the top byte must be masked off, because the
 *            engine's clamp produces a genuinely negative S32 and the SAI only
 *            wants the low 24 bits.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef CHAN_MAP_H
#define CHAN_MAP_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"

#include "audio_io_cfg.h"



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Merge two stereo receive buffers into one four-channel block,
 *        sign extending each 24-bit sample.
 *
 * @param pSai1   SAI1 receive half, nFrames x AIO_SLOTS_PER_SAI words
 * @param pSai2   SAI2 receive half, same shape
 * @param pBlock  output, nFrames x AUDIO_CH_QTY words
 * @param nFrames frames to convert
 */
extern void ChanMap_Gather(const S32* const pSai1,
                           const S32* const pSai2,
                           S32* const pBlock,
                           const U16 nFrames);

/**
 * @brief Split a four-channel block back into two stereo transmit buffers,
 *        masking each sample to the 24 bits the SAI will shift out.
 *
 * @param pBlock  input, nFrames x AUDIO_CH_QTY words
 * @param pSai1   SAI1 transmit half, nFrames x AIO_SLOTS_PER_SAI words
 * @param pSai2   SAI2 transmit half, same shape
 * @param nFrames frames to convert
 */
extern void ChanMap_Scatter(const S32* const pBlock,
                            S32* const pSai1,
                            S32* const pSai2,
                            const U16 nFrames);

/**
 * @brief Fill a transmit buffer with digital silence.
 *
 * Not memset: a zeroed 24-bit word IS silence, but this states the intent and
 * keeps the one place that knows the hardware word format in this file.
 */
extern void ChanMap_Silence(S32* const pBuf, const U32 nWords);



#endif // #ifndef CHAN_MAP_H

/****************************************** end of file *******************************************/
