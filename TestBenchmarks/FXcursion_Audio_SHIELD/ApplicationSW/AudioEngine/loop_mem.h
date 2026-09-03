/***************************************************************************************************
* @file     loop_mem.h
*
* @brief    Loop audio memory, and the per-block window onto it.
*
*           Replaced loop_store.[ch], which staged loop audio to and from QSPI
*           PSRAM by MDMA. There is no PSRAM on this board any more: loop audio
*           lives in the two SDRAM banks, 11 MiB of each, and the CPU can
*           address it directly.
*
*           ------------------------------------------------------------------
*           WHY THERE IS STILL A WINDOW
*           ------------------------------------------------------------------
*
*           The obvious simplification, now that the memory is addressable, is
*           to hand the loopers a raw pointer into SDRAM and delete this file.
*           Two reasons not to:
*
*           1. WRAP. A block that straddles the loop end is not contiguous in
*              the buffer. A raw pointer cannot express that, so every caller
*              would have to handle the split - and the one that forgets
*              produces a click at the loop point that only appears at certain
*              loop lengths.
*
*           2. SPEED. DTCM is zero wait state. SDRAM costs a row activate on
*              every new row, and the loopers touch a window repeatedly within
*              a block - read, sum, write back. Copying 192 bytes in and out
*              once per block is cheaper than paying SDRAM latency per sample,
*              and it is why the window survived the move rather than being
*              an artefact of PSRAM.
*
*           The copies are memcpy now rather than MDMA, so they complete inside
*           the block that asked for them. That removes a whole class of
*           failure with them: there is no chain to drain, so nothing can fail
*           to drain in time. LoopMem_Underruns exists only to keep the
*           diagnostic frame's shape and reads zero.
*
*           ------------------------------------------------------------------
*           LAYOUT
*           ------------------------------------------------------------------
*
*             planes 0,1  ->  SDRAM bank 1, .sdram_loop_a, beside the delay lines
*             planes 2,3  ->  SDRAM bank 2, .sdram_loop_b, beside the reverb
*
*           Each plane is held TWICE: the take, and the snapshot undo restores.
*           Both copies of a plane are in the same bank, so an undo is a copy
*           within one chip select rather than across two.
*
***************************************************************************************************/

#ifndef LOOP_MEM_H
#define LOOP_MEM_H

/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"

#include "audio_cfg.h"
#include "mem_map.h"
#include "fx_defs.h"

/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/** Frames of loop audio staged in DTCM per plane. One audio block. */
#define LOOP_WINDOW_FRAMES              (AUDIO_BLOCK_FRAMES)

/** Bytes per window. 64 frames x 3 B = 192 B. */
#define LOOP_WINDOW_BYTES               (LOOP_WINDOW_FRAMES * LOOP_BYTES_PER_SAMPLE)

/** Which of the two copies of a plane. */
#define LOOP_COPY_TAKE                  (0U)
#define LOOP_COPY_UNDO                  (1U)
#define LOOP_COPY_QTY                   (2U)

/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Clear every window. Call once at start-up, before any audio runs.
 */
extern STD_RESULT LoopMem_Init(void);

/**
 * @brief Open the block: fill the armed windows from SDRAM.
 *
 * Call ONCE per audio block, before any looper runs. It has side effects - an
 * armed plane becomes valid here - which is why it is not a plain query.
 *
 * @return TRUE when the windows are usable this block. Always TRUE now that
 *         the fill is synchronous; the return is kept so the call sites did
 *         not have to change.
 */
extern BOOLEAN LoopMem_BeginBlock(void);

/** @brief What LoopMem_BeginBlock latched, for the loopers to read per chain. */
extern BOOLEAN LoopMem_Ready(void);

/**
 * @brief TRUE when this plane's window holds real loop content.
 *
 * FALSE between arming a plane and the following block, and whenever the loop
 * is shorter than one window - which is what an empty loop should sound like.
 */
extern BOOLEAN LoopMem_Valid(const U8 nPlane);

/** @brief The DTCM window for one plane. */
extern U8* LoopMem_Window(const U8 nPlane);

/**
 * @brief Ask for a window at a given position, e.g. on a transport start.
 *
 * The window is zeroed immediately so nothing stale can be heard, marked
 * invalid, and filled at the next LoopMem_BeginBlock.
 *
 * @param nLen  loop length in frames; below one window the plane is left
 *              zeroed and invalid
 */
extern void LoopMem_Arm(const U8 nPlane, const U32 nPos, const U32 nLen);

/**
 * @brief Open a window at a position WITHOUT reading it back.
 *
 * For starting a recording, where the block is about to overwrite every sample
 * anyway, so the take starts on the sample the player expects rather than one
 * block later. Using it where the previous contents are needed silently erases
 * a block of the loop on the first write-back.
 */
extern void LoopMem_ArmBlank(const U8 nPlane, const U32 nPos, const U32 nLen);

/**
 * @brief Hand a window back and ask for the next one.
 *
 * Writes the window back when bDirty, then advances to nNextPos. The write
 * happens here rather than at Kick so that a plane's own read and write cannot
 * be reordered against each other.
 */
extern void LoopMem_Commit(const U8 nPlane,
                           const U32 nNextPos,
                           const U32 nLen,
                           const BOOLEAN bDirty);

/**
 * @brief End of block. Retained as the counterpart to BeginBlock; the transfers
 *        it used to start are already done.
 */
extern void LoopMem_Kick(void);

/** @brief Zero every window and mark them invalid, e.g. on a topology change. */
extern void LoopMem_Invalidate(void);

/** Kept for the diagnostic frame's shape. Both read zero: the copies are
    synchronous, so there is no chain that can fail to drain. */
extern U32 LoopMem_Underruns(void);
extern U32 LoopMem_Errors(void);

/***************************************************************************************************
* Whole-buffer access, for the loop transport and for undo
***************************************************************************************************/

/**
 * @brief Base of one plane's buffer.
 *
 * The transport reads and writes whole loops through this; the loopers do not
 * use it, they go through the window.
 *
 * @param nPlane  0 .. AUDIO_PLANE_QTY-1
 * @param nCopy   LOOP_COPY_TAKE or LOOP_COPY_UNDO
 *
 * @return NULL_PTR on a bad plane or copy index
 */
extern U8* LoopMem_PlaneBase(const U8 nPlane, const U8 nCopy);

/** @brief Bytes in one plane's buffer. */
extern U32 LoopMem_PlaneBytes(void);

/**
 * @brief Copy the take to the undo snapshot, or back.
 *
 * @param nLooper  0 .. LOOPER_QTY-1
 * @param nFrames  frames to copy; the rest of the buffer is left alone
 * @param bRestore FALSE takes a snapshot, TRUE puts it back
 */
extern STD_RESULT LoopMem_Snapshot(const U8 nLooper,
                                   const U32 nFrames,
                                   const BOOLEAN bRestore);

#endif // #ifndef LOOP_MEM_H

/****************************************** end of file *******************************************/
