/**
 * @file      grid.c
 *
 * @details   Topology table, configuration validation, and the column loop.
 *
 *            Block dispatch lives here rather than in a separate block.c: it is
 *            a five-case switch and it belongs next to the loop that reads the
 *            grid.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "grid.h"

#include "mem_map.h"
#include "params.h"
#include "fxblock.h"
#include "mixer.h"
#include "looper.h"
#include "recorder.h"
#include "Effects/fx_common.h"



/***************************************************************************************************
* Definitions of global (public) variables
***************************************************************************************************/

/*
 * The four topologies.
 *
 * Note that the widths in every row sum to AUDIO_CH_QTY. That is the invariant
 * the whole memory design rests on - if you add a topology, it must hold.
 */
const TOPOLOGY_DESC g_aTopology[TOPO_QTY] =
{
    /* TOPO_4_MONO    */ { 4U, { {0U, 1U}, {1U, 1U}, {2U, 1U}, {3U, 1U} } },
    /* TOPO_ST1_2MONO */ { 3U, { {0U, 2U}, {2U, 1U}, {3U, 1U}, {0U, 0U} } },
    /* TOPO_2MONO_ST2 */ { 3U, { {0U, 1U}, {1U, 1U}, {2U, 2U}, {0U, 0U} } },
    /* TOPO_2_STEREO  */ { 2U, { {0U, 2U}, {2U, 2U}, {0U, 0U}, {0U, 0U} } },
};



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Two grids, ping-ponged. The super-loop fills the one that is not running and
 * then publishes it by storing a pointer - a single aligned 32-bit store, which
 * is atomic on Cortex-M7. That store is the entire synchronisation mechanism
 * between the control path and the audio path.
 *
 * DTCM is NOLOAD, so these are garbage until Grid_Init().
 */
static GRID           aGrid[2] IN_DTCM;
static GRID* volatile pActive  IN_DTCM;
static U8             nShadow  IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Is this effect type present in this chain's FX block?
 */
static BOOLEAN FxPresent(const GRID* const pGrid, const U8 nChain, const U8 eFxType)
{
    BOOLEAN bFound = FALSE;
    U8      i;

    // An FX block only runs if the chain actually contains one.
    for (i = 0U; i < GRID_SLOT_QTY; i++)
    {
        if (pGrid->aSlot[nChain][i] == (U8)BLOCK_FX)
        {
            break;
        }
    }

    if (i < GRID_SLOT_QTY)
    {
        for (i = 0U; i < FXBLOCK_SLOT_QTY; i++)
        {
            if (pGrid->aFxSlot[nChain][i] == eFxType)
            {
                bFound = TRUE;
                break;
            }
        }
    }

    return bFound;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Reject anything structurally impossible before it can reach the ISR.
 *
 * Note what is NOT checked: there is no memory check and no recorder-slot
 * conflict check, because neither can fail. Memory is reserved statically for
 * every (plane, effect type), and a recorder tap consumes exactly its chain's
 * width, which always sums to REC_SLOT_QTY. Every arrangement the GUI can
 * express is playable.
 */
static PROTO_RESULT Validate(const PROTO_CFG* const pCfg)
{
    PROTO_RESULT eResult   = PROTO_RES_OK;
    U8           nChainQty;
    U8           nChain;
    U8           nCol;

    if (pCfg->nVersion != (U8)PROTO_VERSION)
    {
        eResult = PROTO_RES_BAD_VERSION;
    }
    else if (pCfg->eTopology >= (U8)TOPO_QTY)
    {
        eResult = PROTO_RES_BAD_TOPOLOGY;
    }
    else if ((pCfg->nMixerCol != GRID_MIXER_COL_NONE) &&
             ((pCfg->nMixerCol < 0) || (pCfg->nMixerCol >= (S8)GRID_SLOT_QTY)))
    {
        eResult = PROTO_RES_BAD_GRID;
    }
    else
    {
        nChainQty = g_aTopology[pCfg->eTopology].nChainQty;

        for (nChain = 0U; (nChain < nChainQty) && (eResult == PROTO_RES_OK); nChain++)
        {
            U8 aBlockSeen[BLOCK_TYPE_QTY] = { 0U, 0U, 0U, 0U, 0U };
            U8 aFxSeen[FX_TYPE_QTY];
            U8 i;

            for (i = 0U; i < (U8)FX_TYPE_QTY; i++)
            {
                aFxSeen[i] = 0U;
            }

            for (nCol = 0U; (nCol < GRID_SLOT_QTY) && (eResult == PROTO_RES_OK); nCol++)
            {
                const U8 eBlock = pCfg->aSlot[nChain][nCol];

                if (eBlock >= (U8)BLOCK_TYPE_QTY)
                {
                    eResult = PROTO_RES_BAD_GRID;
                }
                else if (eBlock == (U8)BLOCK_MIXER)
                {
                    // The mixer spans a whole column, so it may only appear in
                    // the declared mixer column - and it must appear there in
                    // every active chain.
                    if ((S8)nCol != pCfg->nMixerCol)
                    {
                        eResult = PROTO_RES_BAD_GRID;
                    }
                }
                else if (eBlock != (U8)BLOCK_NONE)
                {
                    aBlockSeen[eBlock]++;
                    if (aBlockSeen[eBlock] > 1U)
                    {
                        eResult = PROTO_RES_BAD_GRID;   // one of each type per chain
                    }
                }
                else
                {
                    do_nothing();
                }
            }

            // If a mixer column was declared, that cell must hold the mixer.
            if ((eResult == PROTO_RES_OK) && (pCfg->nMixerCol != GRID_MIXER_COL_NONE))
            {
                if (pCfg->aSlot[nChain][pCfg->nMixerCol] != (U8)BLOCK_MIXER)
                {
                    eResult = PROTO_RES_BAD_GRID;
                }
            }

            // Effects inside the FX block: valid ids, at most one of each type,
            // and wide enough a chain for the effect to make sense.
            //
            // The one-of-each rule is what makes both the parameter addressing
            // and the static memory reservation work.
            for (i = 0U; (i < FXBLOCK_SLOT_QTY) && (eResult == PROTO_RES_OK); i++)
            {
                const U8 eFx = pCfg->aFxSlot[nChain][i];

                if (eFx == (U8)FX_TYPE_NONE)
                {
                    do_nothing();
                }
                else if (eFx >= (U8)FX_TYPE_QTY)
                {
                    eResult = PROTO_RES_BAD_GRID;
                }
                else if (g_aFxDesc[eFx].nWidth !=
                         g_aTopology[pCfg->eTopology].aChain[nChain].nWidth)
                {
                    // Every effect is mono-only or stereo-only, so the width must
                    // match EXACTLY. This is what makes "you cannot change a
                    // delay from mono to stereo" a rule the machine enforces
                    // rather than a convention.
                    //
                    // It is also why a chain that changes width must arrive with
                    // its FX block cleared or repopulated with the other
                    // variants: the interface holds the same descriptor table and
                    // resolves the id with FX_VARIANT_FOR_WIDTH, so a correct
                    // interface can never trip this. Tripping it is an interface
                    // bug, and the running graph is left untouched.
                    eResult = PROTO_RES_BAD_WIDTH;
                }
                else
                {
                    aFxSeen[eFx]++;
                    if (aFxSeen[eFx] > 1U)
                    {
                        eResult = PROTO_RES_BAD_GRID;
                    }
                }
            }
        }
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Clear the state of every effect that the new grid adds.
 *
 * Runs in the super-loop. Safe because an effect that is being added is by
 * definition not being read by the ISR yet, and necessary because clearing a
 * 4-second delay line is a multi-millisecond memset that must never happen
 * inside the audio ISR.
 *
 * Parameters are deliberately NOT reset: removing an effect and putting it back
 * should return it with the settings the user left on it.
 */
static void ResetAddedState(const GRID* const pOld, const GRID* const pNew)
{
    const BOOLEAN bTopologyChanged = (pOld->eTopology != pNew->eTopology) ? TRUE : FALSE;
    U8            nChain;
    U8            eFxType;

    for (nChain = 0U; nChain < pNew->nChainQty; nChain++)
    {
        const U8 nBase  = pNew->aPlaneBase[nChain];
        const U8 nWidth = pNew->aWidth[nChain];

        for (eFxType = 0U; eFxType < (U8)FX_TYPE_QTY; eFxType++)
        {
            const BOOLEAN bWasThere = FxPresent(pOld, nChain, eFxType);
            const BOOLEAN bIsThere  = FxPresent(pNew, nChain, eFxType);

            // A topology change re-maps which planes belong to which chain, so
            // every plane's history is stale even where the effect list did not
            // change.
            if ((bIsThere != FALSE) && ((bWasThere == FALSE) || (bTopologyChanged != FALSE)))
            {
                if (g_aFxEntry[eFxType].pfReset != NULL_PTR)
                {
                    g_aFxEntry[eFxType].pfReset(nBase, nWidth);
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Run one non-mixer block.
 */
static void ProcessCell(const GRID* const pGrid,
                        const U8 nChain,
                        const U8 eBlock,
                        FLOAT32* const apChain[],
                        const U16 nFrames)
{
    switch (eBlock)
    {
        case (U8)BLOCK_FX:
            FxBlock_Process(pGrid, nChain, apChain, nFrames);
            break;

        case (U8)BLOCK_RECORDER:
            Recorder_Process(pGrid, nChain, apChain, nFrames);
            break;

        case (U8)BLOCK_LOOPER:
            Looper_Process(pGrid, nChain, apChain, nFrames);
            break;

        case (U8)BLOCK_NONE:
        case (U8)BLOCK_MIXER:
        default:
            // BLOCK_NONE is a passthrough and BLOCK_MIXER is handled by the
            // caller, which sees the whole column at once.
            do_nothing();
            break;
    }
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT Grid_Init(void)
{
    U8 i;
    U8 nChain;
    U8 nCol;

    for (i = 0U; i < 2U; i++)
    {
        const TOPOLOGY_DESC* const pTopo = &g_aTopology[TOPO_4_MONO];

        aGrid[i].eTopology = (U8)TOPO_4_MONO;
        aGrid[i].nChainQty = pTopo->nChainQty;
        aGrid[i].nMixerCol = GRID_MIXER_COL_NONE;
        aGrid[i].bAutoGain = (U8)TRUE;

        for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
        {
            aGrid[i].aPlaneBase[nChain] = pTopo->aChain[nChain].nPlaneBase;
            aGrid[i].aWidth[nChain]     = pTopo->aChain[nChain].nWidth;
            aGrid[i].aFxEnabled[nChain] = 0U;
            aGrid[i].aRecSlot[nChain]   = (U8)REC_SLOT_NONE;

            for (nCol = 0U; nCol < GRID_SLOT_QTY; nCol++)
            {
                aGrid[i].aSlot[nChain][nCol] = (U8)BLOCK_NONE;
            }
            for (nCol = 0U; nCol < FXBLOCK_SLOT_QTY; nCol++)
            {
                aGrid[i].aFxSlot[nChain][nCol] = (U8)FX_TYPE_NONE;
            }
        }
    }

    nShadow = 1U;
    pActive = &aGrid[0];

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT Grid_Apply(const PROTO_CFG* const pCfg, PROTO_ACK* const pAck)
{
    STD_RESULT   eStdResult = RESULT_NOT_OK;
    PROTO_RESULT eResult;
    U8           nChain;

    if (pCfg == NULL_PTR)
    {
        eResult = PROTO_RES_BAD_GRID;
    }
    else
    {
        eResult = Validate(pCfg);
    }

    if (eResult == PROTO_RES_OK)
    {
        GRID* const                pNew  = &aGrid[nShadow];
        const GRID* const          pOld  = pActive;
        const TOPOLOGY_DESC* const pTopo = &g_aTopology[pCfg->eTopology];

        pNew->eTopology = pCfg->eTopology;
        pNew->nChainQty = pTopo->nChainQty;
        pNew->nMixerCol = pCfg->nMixerCol;
        pNew->bAutoGain = (pCfg->bAutoGain != 0U) ? (U8)TRUE : (U8)FALSE;

        for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
        {
            U8 i;

            pNew->aPlaneBase[nChain] = pTopo->aChain[nChain].nPlaneBase;
            pNew->aWidth[nChain]     = pTopo->aChain[nChain].nWidth;
            pNew->aFxEnabled[nChain] = pCfg->aFxEnabled[nChain];

            for (i = 0U; i < GRID_SLOT_QTY; i++)
            {
                pNew->aSlot[nChain][i] = (nChain < pTopo->nChainQty)
                                       ? pCfg->aSlot[nChain][i]
                                       : (U8)BLOCK_NONE;
            }
            for (i = 0U; i < FXBLOCK_SLOT_QTY; i++)
            {
                pNew->aFxSlot[nChain][i] = (nChain < pTopo->nChainQty)
                                         ? pCfg->aFxSlot[nChain][i]
                                         : (U8)FX_TYPE_NONE;
            }

            // Recorder slot assignment.
            //
            // A chain's recorder occupies the stream slots that correspond to
            // its own planes. Because chain widths always sum to AUDIO_CH_QTY,
            // this can never collide and never needs arbitration - the map is a
            // pure function of the topology.
            pNew->aRecSlot[nChain] = (U8)REC_SLOT_NONE;

            if (nChain < pTopo->nChainQty)
            {
                for (i = 0U; i < GRID_SLOT_QTY; i++)
                {
                    if (pNew->aSlot[nChain][i] == (U8)BLOCK_RECORDER)
                    {
                        pNew->aRecSlot[nChain] = pNew->aPlaneBase[nChain];
                        break;
                    }
                }
            }
        }

        ResetAddedState(pOld, pNew);

        // A topology change re-maps planes between chains, so any transport that
        // is running would end up pointing at somebody else's audio. Stop them
        // before the new grid goes live. Loop content and lengths are kept.
        if (pOld->eTopology != pNew->eTopology)
        {
            Looper_TopologyChanged();
        }

        Mixer_Apply(pCfg, pNew);
        Looper_Apply(pCfg, pNew);
        Recorder_Apply(pNew);

        // Publish. Single aligned pointer store: atomic on Cortex-M7, and the
        // only handoff between the control path and the audio path.
        pActive = pNew;
        nShadow = (nShadow == 0U) ? 1U : 0U;

        eStdResult = RESULT_OK;
    }

    if (pAck != NULL_PTR)
    {
        const GRID* const pCur = pActive;

        pAck->eResult  = (U8)eResult;
        pAck->eEchoCmd = (U8)PROTO_CMD_SET_CONFIG;

        // The SPI stream to the interface always carries REC_SLOT_QTY slots at a
        // fixed stride, with silence in unused ones. Keeping the stride constant
        // means the interface never has to reprogram its de-interleave descriptor
        // when the slot COUNT changes - only if the bit depth ever changes. The
        // map below is informational: it tells the interface which chain landed
        // in which slot, so recordings get the right names.
        pAck->nRecSlotQty  = 0U;
        pAck->nStreamWidth = (U8)REC_SLOT_QTY;

        for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
        {
            pAck->aRecSlot[nChain] = pCur->aRecSlot[nChain];

            if (pCur->aRecSlot[nChain] != (U8)REC_SLOT_NONE)
            {
                pAck->nRecSlotQty += pCur->aWidth[nChain];
            }
        }
    }

    return eStdResult;
}

//--------------------------------------------------------------------------------------------------

void Grid_Process(FLOAT32* const apPlane[], const U16 nFrames)
{
    // Latch the graph pointer once. If the super-loop publishes a new grid
    // half-way through this block, we finish the block with the old one and pick
    // the new one up next time - which is exactly the behaviour we want.
    const GRID* const pGrid = pActive;
    U8                nCol;
    U8                nChain;

    for (nCol = 0U; nCol < GRID_SLOT_QTY; nCol++)
    {
        if ((S8)nCol == pGrid->nMixerCol)
        {
            // The mixer sees every chain at once. Column-wise iteration
            // guarantees all chains have finished the previous column, which is
            // precisely the precondition it needs - nothing to synchronise.
            Mixer_Process(pGrid, apPlane, nFrames);
        }
        else
        {
            for (nChain = 0U; nChain < pGrid->nChainQty; nChain++)
            {
                FLOAT32* apChain[CHAIN_MAX_WIDTH];
                U8       p;

                for (p = 0U; p < CHAIN_MAX_WIDTH; p++)
                {
                    const U8 nPlane = pGrid->aPlaneBase[nChain] + p;

                    // A mono chain still gets a valid pointer in slot 1 so that
                    // an effect which forgets to check nWidth cannot run off the
                    // end of the array. It must not USE it.
                    apChain[p] = (p < pGrid->aWidth[nChain])
                               ? apPlane[nPlane]
                               : apPlane[pGrid->aPlaneBase[nChain]];
                }

                ProcessCell(pGrid, nChain, pGrid->aSlot[nChain][nCol], apChain, nFrames);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------

const GRID* Grid_Active(void)
{
    return pActive;
}

/****************************************** end of file *******************************************/
