/**
 * @file      fxblock.h
 *
 * @details   The FX block: an ordered list of up to FXBLOCK_SLOT_QTY effects
 *            drawn from the pool, at most one of each type.
 *
 *            BYPASS SEMANTICS - "trails on"
 *
 *            A disabled effect is NOT skipped. Its process function still runs,
 *            so its delay line keeps advancing and its tail keeps decaying, but
 *            the block's dry signal is restored afterwards. Stomping a delay off
 *            therefore lets the existing repeats ring out instead of freezing
 *            them, which is what players expect.
 *
 *            This is also why toggling an effect never triggers a graph rebuild:
 *            enabling and disabling is one bit in GRID.aFxEnabled, applied on the
 *            next block, with no gap. Only ADDING or REMOVING an effect changes
 *            the grid.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FXBLOCK_H
#define FXBLOCK_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"
#include "grid.h"



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Run one chain's FX block over one block of audio, in place.
 *
 * Call from the audio ISR only.
 *
 * @param pGrid    running graph
 * @param nChain   chain index
 * @param apChain  CHAIN_MAX_WIDTH plane pointers; only the first
 *                 pGrid->aWidth[nChain] of them may be used
 * @param nFrames  frames in this block
 */
extern void FxBlock_Process(const GRID* const pGrid,
                            const U8 nChain,
                            FLOAT32* const apChain[],
                            const U16 nFrames);



#endif // #ifndef FXBLOCK_H

/****************************************** end of file *******************************************/
