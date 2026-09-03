/**
 * @file      fxblock.c
 *
 * @details   Runs the effects inside one FX block.
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

#include "fxblock.h"

#include "mem_map.h"
#include "params.h"
#include "Effects/fx_common.h"



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Dry copy used by the "trails on" bypass. DTCM: zero wait state, never cached.
 * Only ever touched inside the audio ISR, so a single shared buffer is enough.
 */
static FLOAT32 aDry[CHAIN_MAX_WIDTH][AUDIO_BLOCK_FRAMES] IN_DTCM;



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

void FxBlock_Process(const GRID* const pGrid,
                     const U8 nChain,
                     FLOAT32* const apChain[],
                     const U16 nFrames)
{
    const U8 nWidth   = pGrid->aWidth[nChain];
    const U8 nEnabled = pGrid->aFxEnabled[nChain];
    U8       nSlot;

    for (nSlot = 0U; nSlot < FXBLOCK_SLOT_QTY; nSlot++)
    {
        const U8 eFxType = pGrid->aFxSlot[nChain][nSlot];

        if (eFxType < (U8)FX_TYPE_QTY)
        {
            const FX_ENTRY* const pEntry  = &g_aFxEntry[eFxType];
            const BOOLEAN         bActive = ((nEnabled & (U8)(1U << nSlot)) != 0U) ? TRUE : FALSE;

            if (pEntry->pfProcess != NULL_PTR)
            {
                FX_CTX tCtx;
                U8     p;

                tCtx.nChain     = nChain;
                tCtx.nPlaneBase = pGrid->aPlaneBase[nChain];
                tCtx.nWidth     = nWidth;
                tCtx.nReserved  = 0U;
                tCtx.pParam     = Params_Get(nChain, eFxType);
                tCtx.pTempo     = Params_Tempo();

                if (bActive != FALSE)
                {
                    pEntry->pfProcess(&tCtx, apChain, nFrames);
                }
                else
                {
                    // Trails-on bypass: let the effect run so its tail keeps
                    // decaying, then put the dry signal back. See fxblock.h.
                    for (p = 0U; p < nWidth; p++)
                    {
                        (void)memcpy(aDry[p], apChain[p], (size_t)nFrames * sizeof(FLOAT32));
                    }

                    pEntry->pfProcess(&tCtx, apChain, nFrames);

                    for (p = 0U; p < nWidth; p++)
                    {
                        (void)memcpy(apChain[p], aDry[p], (size_t)nFrames * sizeof(FLOAT32));
                    }
                }
            }
        }
    }
}

/****************************************** end of file *******************************************/
