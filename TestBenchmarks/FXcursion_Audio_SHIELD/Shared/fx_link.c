/**
 * @file      fx_link.c
 *
 * @details   Control link framing. See fx_link.h for the model and for why the
 *            resynchronisation rules below exist in exactly one place.
 *
 *            ############################################################
 *            #  DUPLICATED IN THE INTERFACE CONTROLLER - keep in sync.  #
 *            #  Use TestBenchmarks/sync_shared.py.                      #
 *            ############################################################
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      02.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_link.h"

#include "fx_crc.h"

#include <string.h>



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

typedef enum enFX_RX_STATE
{
    RX_SYNC0 = 0,
    RX_SYNC1,
    RX_LEN,
    RX_CMD,
    RX_PAYLOAD,
    RX_CRC_LO,
    RX_CRC_HI

} FX_RX_STATE;



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/* Lock-free single-producer (may be an ISR) single-consumer (the poller) ring. */
static volatile U8  aRxRing[FX_LINK_RX_RING_BYTES];
static volatile U16 nRxHead;
static volatile U16 nRxTail;

static U8 aPayload[PROTO_PAYLOAD_MAX];
static U8 aTxFrame[PROTO_FRAME_MAX];

static FX_RX_STATE eRxState;
static U8          nRxLen;
static U8          nRxCmd;
static U8          nRxIdx;
static U16         nRxCrc;

static FX_LINK_TX_FN       pfTransmit;
static FX_LINK_DISPATCH_FN pfDispatch;

static FX_LINK_STATS tStats;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static void Resync(void)
{
    eRxState = RX_SYNC0;
    tStats.nResyncs++;
}

//--------------------------------------------------------------------------------------------------

static void ParseByte(const U8 nByte)
{
    switch (eRxState)
    {
        case RX_SYNC0:
            if (nByte == (U8)PROTO_SYNC0)
            {
                eRxState = RX_SYNC1;
            }
            break;

        case RX_SYNC1:
            if (nByte == (U8)PROTO_SYNC1)
            {
                eRxState = RX_LEN;
            }
            else if (nByte == (U8)PROTO_SYNC0)
            {
                do_nothing();               // stay here: 0xA5 0xA5 0x5A is valid
            }
            else
            {
                eRxState = RX_SYNC0;
            }
            break;

        case RX_LEN:
            if (nByte > (U8)PROTO_PAYLOAD_MAX)
            {
                /* Resynchronise rather than clamp. A clamped length silently
                   mis-frames every byte after it, which is far worse than
                   dropping one frame. */
                Resync();
            }
            else
            {
                nRxLen   = nByte;
                nRxIdx   = 0U;
                nRxCrc   = Crc16_Ccitt(&nByte, 1U, 0xFFFFU);
                eRxState = RX_CMD;
            }
            break;

        case RX_CMD:
            nRxCmd   = nByte;
            nRxCrc   = Crc16_Ccitt(&nByte, 1U, nRxCrc);
            eRxState = (nRxLen == 0U) ? RX_CRC_LO : RX_PAYLOAD;
            break;

        case RX_PAYLOAD:
            aPayload[nRxIdx] = nByte;
            nRxCrc           = Crc16_Ccitt(&nByte, 1U, nRxCrc);
            nRxIdx++;

            if (nRxIdx >= nRxLen)
            {
                eRxState = RX_CRC_LO;
            }
            break;

        case RX_CRC_LO:
            nRxIdx   = nByte;               // reuse as CRC low byte
            eRxState = RX_CRC_HI;
            break;

        case RX_CRC_HI:
        default:
        {
            const U16 nGot = (U16)((U16)nRxIdx | ((U16)nByte << 8U));

            if (nGot == nRxCrc)
            {
                tStats.nFramesOk++;

                if (pfDispatch != NULL_PTR)
                {
                    pfDispatch(nRxCmd, aPayload, nRxLen);
                }
            }
            else
            {
                tStats.nCrcErrors++;
            }

            eRxState = RX_SYNC0;
            break;
        }
    }
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT FxLink_Init(const FX_LINK_TX_FN pfTx, const FX_LINK_DISPATCH_FN pfDisp)
{
    pfTransmit = pfTx;
    pfDispatch = pfDisp;

    nRxHead  = 0U;
    nRxTail  = 0U;
    eRxState = RX_SYNC0;
    nRxLen   = 0U;
    nRxCmd   = 0U;
    nRxIdx   = 0U;
    nRxCrc   = 0U;

    tStats.nFramesOk    = 0UL;
    tStats.nCrcErrors   = 0UL;
    tStats.nResyncs     = 0UL;
    tStats.nRxOverflows = 0UL;

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

void FxLink_RxByte(const U8 nByte)
{
    const U16 nNext = (U16)((nRxHead + 1U) & (FX_LINK_RX_RING_BYTES - 1U));

    if (nNext == nRxTail)
    {
        tStats.nRxOverflows++;              // drop; the CRC will catch the damage
    }
    else
    {
        aRxRing[nRxHead] = nByte;
        nRxHead          = nNext;
    }
}

//--------------------------------------------------------------------------------------------------

U16 FxLink_Poll(void)
{
    const U32 nBefore = tStats.nFramesOk;

    while (nRxTail != nRxHead)
    {
        const U8 nByte = aRxRing[nRxTail];

        nRxTail = (U16)((nRxTail + 1U) & (FX_LINK_RX_RING_BYTES - 1U));

        ParseByte(nByte);
    }

    return (U16)(tStats.nFramesOk - nBefore);
}

//--------------------------------------------------------------------------------------------------

STD_RESULT FxLink_Send(const U8 eCmd, const U8* const pPayload, const U8 nLength)
{
    STD_RESULT eResult;

    if (nLength > (U8)PROTO_PAYLOAD_MAX)
    {
        eResult = RESULT_INVALID_PARAM_2;
    }
    else if (pfTransmit == NULL_PTR)
    {
        eResult = RESULT_NOT_INIT;
    }
    else
    {
        U16 nCrc;
        U16 nPos = 0U;

        aTxFrame[nPos] = (U8)PROTO_SYNC0; nPos++;
        aTxFrame[nPos] = (U8)PROTO_SYNC1; nPos++;
        aTxFrame[nPos] = nLength;         nPos++;
        aTxFrame[nPos] = eCmd;            nPos++;

        if ((pPayload != NULL_PTR) && (nLength > 0U))
        {
            (void)memcpy(&aTxFrame[nPos], pPayload, (size_t)nLength);
            nPos = (U16)(nPos + nLength);
        }

        // CRC covers LEN, CMD and payload - not the sync word.
        nCrc = Crc16_Ccitt(&aTxFrame[2], (U16)(nLength + 2U), 0xFFFFU);

        aTxFrame[nPos] = (U8)(nCrc & 0xFFU);         nPos++;
        aTxFrame[nPos] = (U8)((nCrc >> 8U) & 0xFFU); nPos++;

        eResult = pfTransmit(aTxFrame, nPos);
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

const FX_LINK_STATS* FxLink_Stats(void)
{
    return &tStats;
}

/****************************************** end of file *******************************************/
