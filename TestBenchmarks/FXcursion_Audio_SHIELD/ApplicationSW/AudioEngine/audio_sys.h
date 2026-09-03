/**
 * @file      audio_sys.h
 *
 * @details   Top of the audio engine: one block in, one block out.
 *
 *            ------------------------------------------------------------------
 *            THE WHOLE CONCURRENCY DESIGN
 *            ------------------------------------------------------------------
 *
 *            There is no RTOS and there are no locks.
 *
 *              AUDIO ISR (SAI1_A DMA half/full complete)
 *                  AudioSys_ProcessBlock()
 *                  Everything DSP happens here and nowhere else.
 *
 *              SUPER-LOOP (main)
 *                  drain the control link, validate configurations, publish
 *                  graphs, emit telemetry, then __WFI().
 *
 *              HANDOFF
 *                  one aligned pointer store in grid.c. That is all.
 *
 *            All four SAI blocks are already synchronous to SAI1_A in the
 *            existing sai.c (SAI_SYNCHRONOUS_EXT_SAI1), so the four channels
 *            share one frame sync and ONE interrupt drives the whole machine.
 *            SAI2's callbacks should be left disabled. This is also why the old
 *            "saiAdapterCheckUpdate(a1) || saiAdapterCheckUpdate(a2)" bug cannot
 *            be expressed here: there is only one timebase.
 *
 *            ------------------------------------------------------------------
 *            BLOCK ORDER
 *            ------------------------------------------------------------------
 *
 *              1. de-interleave and convert the input to planar FLOAT32
 *              2. Recorder_BeginBlock  - silence unused recorder slots
 *              3. Grid_Process         - the column loop
 *              4. meters
 *              5. convert and interleave the output
 *              6. Params_TempoAdvance
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef AUDIO_SYS_H
#define AUDIO_SYS_H



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
 * @brief Initialise every module in the audio engine.
 *
 * MUST be called before the SAI DMA is started.
 *
 * This function exists because C has no constructors AND because the .dtcm
 * section is NOLOAD - it gets neither the .data copy nor the .bss zero fill, so
 * every static in this project is garbage until its module's Init runs. In the
 * old C++ project this was hidden by global constructors, which is exactly the
 * kind of thing that makes a C++ to C port fail mysteriously.
 */
extern STD_RESULT AudioSys_Init(void);

/**
 * @brief Process one block. Call from the audio ISR only.
 *
 * @param pIn      nFrames * AUDIO_CH_QTY interleaved samples, 24-bit in S32
 * @param pOut     nFrames * AUDIO_CH_QTY interleaved samples, 24-bit in S32
 * @param nFrames  frames in this block, <= AUDIO_BLOCK_FRAMES
 */
extern void AudioSys_ProcessBlock(const S32* const pIn, S32* const pOut, const U16 nFrames);

/**
 * @brief Report how long the last block took.
 *
 * Called by the platform layer, which owns the cycle counter. Keeping the DWT
 * out of Modules/ is what lets the whole DSP layer build and run on a PC.
 *
 * @param nCycles  cycles consumed by AudioSys_ProcessBlock
 * @param nBudget  cycles available per block (core clock / blocks per second)
 */
extern void AudioSys_ReportLoad(const U32 nCycles, const U32 nBudget);

/**
 * @brief Count a block that missed its deadline.
 *
 * The platform layer calls this when a DMA interrupt arrives while the previous
 * block is still being processed. This is the only reliable glitch detector in
 * the system - without it, overruns are silent.
 */
extern void AudioSys_NotifyOverrun(void);

/** Fill a telemetry frame. Call from the super-loop. */
extern void AudioSys_GetTelemetry(PROTO_TELEMETRY* const pTelem);



#endif // #ifndef AUDIO_SYS_H

/****************************************** end of file *******************************************/
