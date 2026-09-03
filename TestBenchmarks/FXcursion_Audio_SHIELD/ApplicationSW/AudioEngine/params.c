/**
 * @file      params.c
 *
 * @details   Parameter storage, tempo state and the preset blob.
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

#include "params.h"

#include "mem_map.h"
#include "fx_crc.h"
#include "Effects/fx_common.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

#define PARAMS_DEFAULT_BPM_X10          (1200U)     /* 120.0 BPM */
#define PARAMS_DEFAULT_BEATS_PER_BAR    (4U)
#define PARAMS_DEFAULT_BEAT_UNIT        (4U)

/**
 * Default normalised value of every parameter.
 *
 * Rows follow FX_TYPE; columns follow that effect's parameter enum in fx_defs.h.
 * Unused columns are ignored. Kept here rather than inside each effect so that
 * one glance shows how the machine comes up from a factory reset.
 */
static const FLOAT32 aDefaultNorm[FX_TYPE_QTY][FX_PARAM_QTY] =
{
    /* FX_AMP_M          gain                                                    */
    {                    0.50f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_AMP_S          gain   pan    width                                     */
    {                    0.50f, 0.50f, 0.50f, 0.0f,  0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_CHORUS_M       rate   depth  delay  mix                                */
    {                    0.30f, 0.40f, 0.30f, 0.50f, 0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_CHORUS_S       rate   depth  delay  mix    spread                      */
    {                    0.30f, 0.40f, 0.30f, 0.50f, 0.70f, 0.0f, 0.0f, 0.0f },
    /* FX_COMPRESSOR_M   thr    ratio  att    rel    makeup                      */
    {                    0.70f, 0.30f, 0.20f, 0.40f, 0.00f, 0.0f, 0.0f, 0.0f },
    /* FX_COMPRESSOR_S   thr    ratio  att    rel    makeup link                 */
    {                    0.70f, 0.30f, 0.20f, 0.40f, 0.00f, 1.0f, 0.0f, 0.0f },
    /* FX_DELAY_M        time   fb     tone   mix                                */
    {                    0.40f, 0.35f, 0.60f, 0.35f, 0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_DELAY_S        time   fb     tone   mix    pingpong spread             */
    {                    0.40f, 0.35f, 0.60f, 0.35f, 0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_DISTORTION_M   drive  tone   level  mix                                */
    {                    0.50f, 0.50f, 0.50f, 1.00f, 0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_DISTORTION_S   drive  tone   level  mix    spread                      */
    {                    0.50f, 0.50f, 0.50f, 1.00f, 0.30f, 0.0f, 0.0f, 0.0f },
    /* FX_FLANGER_M      rate   depth  fb     mix                                */
    {                    0.20f, 0.50f, 0.40f, 0.50f, 0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_FLANGER_S      rate   depth  fb     mix    spread                      */
    {                    0.20f, 0.50f, 0.40f, 0.50f, 0.70f, 0.0f, 0.0f, 0.0f },
    /* FX_OVERDRIVE_M    drive  bias   level  mix                                */
    {                    0.40f, 0.70f, 0.50f, 1.00f, 0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_OVERDRIVE_S    drive  bias   level  mix    spread                      */
    {                    0.40f, 0.70f, 0.50f, 1.00f, 0.25f, 0.0f, 0.0f, 0.0f },
    /* FX_PHASER_M       rate   depth  fb     stages mix                         */
    {                    0.25f, 0.60f, 0.30f, 0.50f, 0.50f, 0.0f, 0.0f, 0.0f },
    /* FX_PHASER_S       rate   depth  fb     stages mix    spread               */
    {                    0.25f, 0.60f, 0.30f, 0.50f, 0.50f, 0.70f, 0.0f, 0.0f },
    /* FX_REVERB_M       decay  pre    damp   diff   mix                         */
    {                    0.50f, 0.10f, 0.50f, 0.60f, 0.30f, 0.0f, 0.0f, 0.0f },
    /* FX_REVERB_S       decay  pre    damp   diff   mix    width                */
    {                    0.50f, 0.10f, 0.50f, 0.60f, 0.30f, 0.70f, 0.0f, 0.0f },
    /* FX_TREMOLO_M      rate   depth  shape                                     */
    {                    0.35f, 0.50f, 0.00f, 0.0f,  0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_TREMOLO_S      rate   depth  shape  phase                              */
    {                    0.35f, 0.50f, 0.00f, 0.0f,  0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_VIBRATO_M      rate   depth                                            */
    {                    0.30f, 0.30f, 0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f },
    /* FX_VIBRATO_S      rate   depth  spread                                    */
    {                    0.30f, 0.30f, 0.70f, 0.0f,  0.0f,  0.0f, 0.0f, 0.0f },
};




/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * DTCM: zero wait state, never cached, no cache maintenance needed.
 *
 * NOTE: the .dtcm section is NOLOAD, so this array is GARBAGE at reset. It is
 * valid only after Params_Init(). See mem_map.h.
 */
static FX_PARAM aFxParam[CHAIN_MAX_QTY][FX_TYPE_QTY][FX_PARAM_QTY] IN_DTCM;

static TEMPO    tTempo IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static void LoadDefaults(const U8 nChain, const U8 eFxType)
{
    U8 i;

    for (i = 0U; i < FX_PARAM_QTY; i++)
    {
        aFxParam[nChain][eFxType][i].fValue       = aDefaultNorm[eFxType][i];
        aFxParam[nChain][eFxType][i].bSync        = FALSE;
        aFxParam[nChain][eFxType][i].eDivision    = (U8)DIV_1_4;
        aFxParam[nChain][eFxType][i].nReserved[0] = 0U;
        aFxParam[nChain][eFxType][i].nReserved[1] = 0U;
    }
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT Params_Init(void)
{
    U8 nChain;
    U8 eFxType;

    for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
    {
        for (eFxType = 0U; eFxType < (U8)FX_TYPE_QTY; eFxType++)
        {
            LoadDefaults(nChain, eFxType);
        }
    }

    tTempo.fBpm         = (FLOAT32)PARAMS_DEFAULT_BPM_X10 * 0.1f;
    tTempo.nBeatsPerBar = (U8)PARAMS_DEFAULT_BEATS_PER_BAR;
    tTempo.nBeatUnit    = (U8)PARAMS_DEFAULT_BEAT_UNIT;
    tTempo.eSource      = (U8)TEMPO_SRC_INTERNAL;
    tTempo.nReserved    = 0U;
    tTempo.nSampleInBar = 0UL;

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

const FX_PARAM* Params_Get(const U8 nChain, const U8 eFxType)
{
    const FX_PARAM* pResult;

    if ((nChain < CHAIN_MAX_QTY) && (eFxType < (U8)FX_TYPE_QTY))
    {
        pResult = &aFxParam[nChain][eFxType][0];
    }
    else
    {
        // Never hand the audio path a NULL. Chain 0 / effect 0 is always valid
        // and its defaults are harmless; a bad index is a control-path bug that
        // must not become a hard fault inside the ISR.
        pResult = &aFxParam[0][0][0];
    }

    return pResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT Params_Set(const PROTO_SET_PARAM* const pCmd)
{
    STD_RESULT eResult = RESULT_OK;

    if (pCmd == NULL_PTR)
    {
        eResult = RESULT_NOT_OK;
    }
    else if (pCmd->nChain >= CHAIN_MAX_QTY)
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else if (pCmd->eFxType >= (U8)FX_TYPE_QTY)
    {
        eResult = RESULT_INVALID_PARAM_1;
    }
    else if (pCmd->nParamIdx >= g_aFxDesc[pCmd->eFxType].nParamQty)
    {
        eResult = RESULT_INVALID_PARAM_2;
    }
    else if (pCmd->eDivision >= (U8)DIV_QTY)
    {
        eResult = RESULT_INVALID_PARAM_3;
    }
    else
    {
        FX_PARAM* const pParam = &aFxParam[pCmd->nChain][pCmd->eFxType][pCmd->nParamIdx];

        // Order matters slightly: write the mode fields before the value, so a
        // block that lands mid-update sees the new mode with the old value
        // rather than a new value interpreted under the old mode. Either way it
        // is one block; see the concurrency note in params.h.
        pParam->eDivision = pCmd->eDivision;
        pParam->bSync     = (pCmd->bSync != 0U) ? (U8)TRUE : (U8)FALSE;
        pParam->fValue    = (FLOAT32)pCmd->nValue * (1.0f / 65535.0f);
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT Params_ResetFx(const U8 nChain, const U8 eFxType)
{
    STD_RESULT eResult;

    if ((nChain < CHAIN_MAX_QTY) && (eFxType < (U8)FX_TYPE_QTY))
    {
        LoadDefaults(nChain, eFxType);
        eResult = RESULT_OK;
    }
    else
    {
        eResult = RESULT_NOT_OK;
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

const TEMPO* Params_Tempo(void)
{
    return &tTempo;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT Params_SetTempo(const U16 nBpmX10, const U8 nBeatsPerBar, const U8 nBeatUnit)
{
    STD_RESULT eResult = RESULT_OK;

    if ((nBpmX10 < 200U) || (nBpmX10 > 4000U))
    {
        eResult = RESULT_INVALID_PARAM_0;               // 20.0 .. 400.0 BPM
    }
    else if ((nBeatsPerBar == 0U) || (nBeatsPerBar > 32U))
    {
        eResult = RESULT_INVALID_PARAM_1;
    }
    else if ((nBeatUnit != 1U) && (nBeatUnit != 2U) && (nBeatUnit != 4U) &&
             (nBeatUnit != 8U) && (nBeatUnit != 16U))
    {
        eResult = RESULT_INVALID_PARAM_2;
    }
    else
    {
        tTempo.fBpm         = (FLOAT32)nBpmX10 * 0.1f;
        tTempo.nBeatsPerBar = nBeatsPerBar;
        tTempo.nBeatUnit    = nBeatUnit;

        // Bar length just changed, so the old position may be past the new end.
        if (tTempo.nSampleInBar >= Params_BarFrames())
        {
            tTempo.nSampleInBar = 0UL;
        }
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

U32 Params_BarFrames(void)
{
    const FLOAT32 fBarSec    = Tempo_BarSec(&tTempo);
    U32           nBarFrames = (U32)((fBarSec * (FLOAT32)AUDIO_SAMPLE_RATE_HZ) + 0.5f);

    if (nBarFrames == 0UL)
    {
        nBarFrames = 1UL;                               // never divide by zero downstream
    }

    return nBarFrames;
}

//--------------------------------------------------------------------------------------------------

void Params_TempoAdvance(const U16 nFrames)
{
    const U32 nBarFrames = Params_BarFrames();

    tTempo.nSampleInBar += (U32)nFrames;

    while (tTempo.nSampleInBar >= nBarFrames)
    {
        tTempo.nSampleInBar -= nBarFrames;
    }
}

//--------------------------------------------------------------------------------------------------

U32 Params_PresetSize(void)
{
    return (U32)sizeof(PRESET_HDR) + (U32)sizeof(aFxParam) + 2UL;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT Params_PresetWrite(U8* const pBuf,
                              const U32 nBufLen,
                              const PROTO_CFG* const pCfg,
                              U32* const pWritten)
{
    STD_RESULT eResult;
    const U32  nNeeded = Params_PresetSize();

    if ((pBuf == NULL_PTR) || (pCfg == NULL_PTR))
    {
        eResult = RESULT_NOT_OK;
    }
    else if (nBufLen < nNeeded)
    {
        eResult = RESULT_INVALID_PARAM_1;
    }
    else
    {
        PRESET_HDR tHdr;
        U16        nCrc;
        U32        nOffset = 0UL;

        tHdr.aMagic[0]  = (U8)'F';
        tHdr.aMagic[1]  = (U8)'X';
        tHdr.aMagic[2]  = (U8)'C';
        tHdr.aMagic[3]  = (U8)'P';
        tHdr.nVersion   = (U16)PRESET_VERSION;
        tHdr.nHdrBytes  = (U16)sizeof(PRESET_HDR);
        tHdr.nChainQty  = (U8)CHAIN_MAX_QTY;
        tHdr.nFxTypeQty = (U8)FX_TYPE_QTY;
        tHdr.nParamQty  = (U8)FX_PARAM_QTY;
        tHdr.nReserved  = 0U;
        tHdr.tCfg       = *pCfg;

        (void)memcpy(&pBuf[nOffset], &tHdr, sizeof(tHdr));
        nOffset += (U32)sizeof(tHdr);

        (void)memcpy(&pBuf[nOffset], aFxParam, sizeof(aFxParam));
        nOffset += (U32)sizeof(aFxParam);

        nCrc = Crc16_Ccitt(pBuf, (U16)nOffset, 0xFFFFU);
        pBuf[nOffset]      = (U8)(nCrc & 0xFFU);
        pBuf[nOffset + 1U] = (U8)((nCrc >> 8U) & 0xFFU);
        nOffset += 2UL;

        if (pWritten != NULL_PTR)
        {
            *pWritten = nOffset;
        }

        eResult = RESULT_OK;
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT Params_PresetRead(const U8* const pBuf, const U32 nLen, PROTO_CFG* const pCfg)
{
    STD_RESULT eResult;
    const U32  nNeeded = Params_PresetSize();

    if ((pBuf == NULL_PTR) || (pCfg == NULL_PTR))
    {
        eResult = RESULT_NOT_OK;
    }
    else if (nLen < nNeeded)
    {
        eResult = RESULT_INVALID_PARAM_1;
    }
    else
    {
        PRESET_HDR tHdr;
        U16        nCrcCalc;
        U16        nCrcFile;
        const U32  nBodyBytes = nNeeded - 2UL;

        (void)memcpy(&tHdr, pBuf, sizeof(tHdr));

        nCrcCalc = Crc16_Ccitt(pBuf, (U16)nBodyBytes, 0xFFFFU);
        nCrcFile = (U16)((U16)pBuf[nBodyBytes] | ((U16)pBuf[nBodyBytes + 1U] << 8U));

        if ((tHdr.aMagic[0] != (U8)'F') || (tHdr.aMagic[1] != (U8)'X') ||
            (tHdr.aMagic[2] != (U8)'C') || (tHdr.aMagic[3] != (U8)'P'))
        {
            eResult = RESULT_NOT_OK;
        }
        else if (tHdr.nVersion != (U16)PRESET_VERSION)
        {
            eResult = RESULT_NOT_OK;
        }
        else if ((tHdr.nChainQty  != (U8)CHAIN_MAX_QTY) ||
                 (tHdr.nFxTypeQty != (U8)FX_TYPE_QTY)   ||
                 (tHdr.nParamQty  != (U8)FX_PARAM_QTY))
        {
            // A preset saved by a build with a different effect pool. Migration
            // belongs here when it is needed; refusing is correct until then.
            eResult = RESULT_NOT_OK;
        }
        else if (nCrcCalc != nCrcFile)
        {
            eResult = RESULT_NOT_OK;
        }
        else
        {
            (void)memcpy(aFxParam, &pBuf[sizeof(PRESET_HDR)], sizeof(aFxParam));
            *pCfg = tHdr.tCfg;

            (void)Params_SetTempo(tHdr.tCfg.nBpmX10, tHdr.tCfg.nBeatsPerBar, tHdr.tCfg.nBeatUnit);

            eResult = RESULT_OK;
        }
    }

    return eResult;
}

/****************************************** end of file *******************************************/
