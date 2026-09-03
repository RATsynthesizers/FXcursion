/**
 * @file      grid.h
 *
 * @details   The signal graph, and the loop that runs it.
 *
 *            ------------------------------------------------------------------
 *            THE MODEL
 *            ------------------------------------------------------------------
 *
 *            The machine is a grid of CHAIN_MAX_QTY rows by GRID_SLOT_QTY
 *            columns. Signal flows from column 0 (IN) to column GRID_SLOT_QTY-1
 *            (OUT) - left to right here, right to left on the GUI.
 *
 *                 OUT <- [ 3 ][ 2 ][ 1 ][ 0 ] <- IN     chain 0
 *                 OUT <- [ 3 ][ 2 ][ 1 ][ 0 ] <- IN     chain 1
 *                          ...
 *
 *            A cell holds one block: FX, Recorder, Looper, or nothing. One
 *            column may instead hold the Mixer, which spans every chain at once.
 *            At most one block of each type per chain.
 *
 *            There is no "pre-mixer chain" and no "post-mixer chain". There is a
 *            grid and a mixer column index, and the engine is one loop:
 *
 *                for each column:
 *                    if this is the mixer column: run the mixer across all chains
 *                    else:                        run each chain's block
 *
 *            Mixer absent, mixer at either edge, empty chains, empty cells - all
 *            fall out of that loop with no special cases.
 *
 *            ------------------------------------------------------------------
 *            HOW A CONFIGURATION CHANGE IS APPLIED
 *            ------------------------------------------------------------------
 *
 *            Grid_Apply() runs in the SUPER-LOOP, not the ISR. It:
 *              1. validates the incoming PROTO_CFG;
 *              2. diffs it against the running grid and clears the state of any
 *                 effect that is being ADDED - safe to do from the super-loop
 *                 precisely because nothing is reading that state yet, and
 *                 necessary because clearing a 768 KiB delay line takes about
 *                 5 ms and must never happen inside the audio ISR;
 *              3. fills a shadow GRID;
 *              4. publishes it with a single aligned pointer store, which is
 *                 atomic on Cortex-M7.
 *
 *            The ISR reads that pointer once at the top of each block. There is
 *            no lock anywhere in this design.
 *
 *            Because memory is statically reserved per (plane, effect type) and
 *            never moves, applying a configuration relinks a table and nothing
 *            else. There is no fade-out, no gap, and no configuration the GUI
 *            permits can be refused for lack of memory.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef GRID_H
#define GRID_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"
#include "fx_defs.h"
#include "fx_protocol.h"



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief How one chain maps onto physical planes.
 */
typedef struct stCHAIN_MAP
{
    U8 nPlaneBase;              /**< first plane of this chain                   */
    U8 nWidth;                  /**< 1 = mono, 2 = stereo                        */

} CHAIN_MAP;

/**
 * @brief One of the four input topologies.
 */
typedef struct stTOPOLOGY_DESC
{
    U8        nChainQty;
    CHAIN_MAP aChain[CHAIN_MAX_QTY];

} TOPOLOGY_DESC;

/**
 * @brief The running graph. Read by the audio ISR, written by the super-loop.
 */
typedef struct stGRID
{
    U8 eTopology;                                   /**< TOPOLOGY                     */
    U8 nChainQty;                                   /**< active chains, 2..4          */
    S8 nMixerCol;                                   /**< column, or GRID_MIXER_COL_NONE */
    U8 bAutoGain;

    U8 aPlaneBase[CHAIN_MAX_QTY];                   /**< first plane of each chain    */
    U8 aWidth[CHAIN_MAX_QTY];                       /**< 1 or 2                       */

    U8 aSlot[CHAIN_MAX_QTY][GRID_SLOT_QTY];         /**< BLOCK_TYPE per cell          */
    U8 aFxSlot[CHAIN_MAX_QTY][FXBLOCK_SLOT_QTY];    /**< FX_TYPE or FX_TYPE_NONE      */
    U8 aFxEnabled[CHAIN_MAX_QTY];                   /**< bitmask over FX slots        */

    U8 aRecSlot[CHAIN_MAX_QTY];                     /**< first recorder slot, or REC_SLOT_NONE */

} GRID;



/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/** The four topologies. Indexed by TOPOLOGY. Lives in flash. */
extern const TOPOLOGY_DESC g_aTopology[TOPO_QTY];



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Bring up an empty grid: all mono, no blocks, no mixer.
 *
 * MUST be called before any audio runs. The grid lives in DTCM, which the
 * startup code neither copies nor zeroes.
 */
extern STD_RESULT Grid_Init(void);

/**
 * @brief Validate a configuration and, if it is good, make it the running graph.
 *
 * Call from the SUPER-LOOP only.
 *
 * @param pCfg  configuration received from the interface controller
 * @param pAck  filled in with the result and the committed recorder slot map;
 *              may be NULL if the caller does not intend to reply
 *
 * @return RESULT_OK when the graph was replaced, RESULT_NOT_OK otherwise. On
 *         failure the previously running graph is untouched and still valid.
 */
extern STD_RESULT Grid_Apply(const PROTO_CFG* const pCfg, PROTO_ACK* const pAck);

/**
 * @brief Run one block through the whole grid, in place.
 *
 * Call from the audio ISR only.
 *
 * @param apPlane  AUDIO_PLANE_QTY pointers to AUDIO_BLOCK_FRAMES floats each,
 *                 holding the input on entry and the output on exit
 * @param nFrames  frames in this block
 */
extern void Grid_Process(FLOAT32* const apPlane[], const U16 nFrames);

/** The running graph. Never NULL after Grid_Init(). */
extern const GRID* Grid_Active(void);



#endif // #ifndef GRID_H

/****************************************** end of file *******************************************/
