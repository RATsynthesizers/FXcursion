/***************************************************************************************************
* @file     fx_loop.c
*
* @brief    Loop transport session state machine. See fx_loop.h.
*
***************************************************************************************************/

/***************************************************************************************************
* Included header files
***************************************************************************************************/

#include "fx_loop.h"
#include "fx_crc.h"

/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/*
 * CRC OVER THE PAYLOAD
 *
 * Crc32_Ieee, not the Crc16_Ccitt that guards control frames. Over 11 MiB a
 * CRC-16 misses a corruption about once in 65536, which is not a number to
 * rest a recording on.
 *
 * It being the ORDINARY CRC32 - zlib's, gzip's, the one a desktop `crc32`
 * prints - is the other half of the reason. The audio board reports a value
 * for the loop it sent; the same value can be recomputed from the file that
 * ended up on the card, using a tool the user already has. Without that, "the
 * loop plays back as noise" has no next step.
 *
 * Chains without a finalise step, so a session folds in arbitrary pieces as
 * they arrive and the running value is directly comparable at any point.
 */
#define FX_LOOP_CRC_INIT                (0UL)

/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Bytes per sample on the wire, or 0 when the format is not one.
 */
static U32 FxLoop_SampleBytes(const U8 eFormat)
{
    U32 nBytes;

    switch (eFormat)
    {
        case (U8)LOOP_FMT_S24:  nBytes = FX_LOOP_BYTES_S24; break;
        case (U8)LOOP_FMT_S32:  nBytes = FX_LOOP_BYTES_S32; break;
        default:                nBytes = 0UL;               break;
    }

    return nBytes;
}


/**
 * @brief Everything about a session that is checkable without knowing which
 *        side is asking. Used by both Open and Accept, so the initiator cannot
 *        propose geometry the responder would have to refuse.
 */
static STD_RESULT FxLoop_CheckGeometry(const U8 eDir,
                                       const U8 nLooper,
                                       const U8 nPlaneQty,
                                       const U8 eFormat,
                                       const U8 nSlotQty)
{
    if ((eDir != (U8)LOOP_DIR_SAVE) && (eDir != (U8)LOOP_DIR_LOAD))
    {
        return RESULT_NOT_OK;
    }

    if (nLooper >= (U8)LOOPER_QTY)
    {
        return RESULT_NOT_OK;
    }

    /* A looper is mono or a stereo pair, like a chain. Nothing else exists. */
    if ((nPlaneQty != 1U) && (nPlaneQty != 2U))
    {
        return RESULT_NOT_OK;
    }

    if (FxLoop_SampleBytes(eFormat) == 0UL)
    {
        return RESULT_NOT_OK;
    }

    /*
     * Zero slots would leave a session that can never make progress: it would
     * reach RUNNING, widen the stream by nothing, and sit there until it was
     * aborted by a timeout somewhere else. Refuse it at the negotiation, which
     * is the only place that can say why.
     */
    if ((nSlotQty == 0U) || (nSlotQty > (U8)FX_LOOP_SLOT_QTY_MAX))
    {
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}

/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

void FxLoop_Reset(FX_LOOP_SESSION* const pSession)
{
    if (pSession == NULL_PTR)
    {
        return;
    }

    pSession->nBytesTotal = 0UL;
    pSession->nBytesMoved = 0UL;
    pSession->nCrc        = (U32)FX_LOOP_CRC_INIT;
    pSession->nSession    = 0U;
    pSession->eState      = (U8)FX_LOOP_IDLE;
    pSession->eDir        = (U8)LOOP_DIR_SAVE;
    pSession->eResult     = (U8)PROTO_RES_OK;
    pSession->nLooper     = 0U;
    pSession->nPlaneQty   = 1U;
    pSession->eFormat     = (U8)LOOP_FMT_S24;
    pSession->nSlotQty    = 0U;
}


STD_RESULT FxLoop_BytesFor(const U32 nSamples,
                           const U8  nPlaneQty,
                           const U8  eFormat,
                           U32* const pnBytes)
{
    const U32 nSampleBytes = FxLoop_SampleBytes(eFormat);
    U32       nPerPlane;

    if (pnBytes == NULL_PTR)
    {
        return RESULT_NOT_OK;
    }

    *pnBytes = 0UL;

    if (nSampleBytes == 0UL)
    {
        return RESULT_NOT_OK;
    }

    if ((nPlaneQty != 1U) && (nPlaneQty != 2U))
    {
        return RESULT_NOT_OK;
    }

    /*
     * Checked, not assumed. A 32-bit byte count tops out at 4 GiB, which no
     * looper here approaches - but this function is the ONE place both boards
     * size a transfer, so an overflow would be agreed on by both of them and
     * produce a short file that reports success. Divide rather than multiply
     * so the check itself cannot overflow.
     */
    if (nSamples > (0xFFFFFFFFUL / nSampleBytes))
    {
        return RESULT_NOT_OK;
    }

    nPerPlane = nSamples * nSampleBytes;

    if (nPerPlane > (0xFFFFFFFFUL / (U32)nPlaneQty))
    {
        return RESULT_NOT_OK;
    }

    *pnBytes = nPerPlane * (U32)nPlaneQty;

    return RESULT_OK;
}


STD_RESULT FxLoop_Open(FX_LOOP_SESSION* const pSession,
                       const U8  nSessionId,
                       const U8  eDir,
                       const U8  nLooper,
                       const U8  nPlaneQty,
                       const U8  eFormat,
                       const U8  nSlotQty,
                       const U32 nBytes,
                       PROTO_LOOP_OPEN* const pOpen)
{
    if ((pSession == NULL_PTR) || (pOpen == NULL_PTR))
    {
        return RESULT_NOT_OK;
    }

    /* One transfer at a time. The stream can only be one width, and a second
       session would silently reuse the first one's slots. */
    if (pSession->eState != (U8)FX_LOOP_IDLE)
    {
        return RESULT_NOT_OK;
    }

    if (FxLoop_CheckGeometry(eDir, nLooper, nPlaneQty, eFormat, nSlotQty) != RESULT_OK)
    {
        return RESULT_NOT_OK;
    }

    /* A load has to say how big the file is; only a save may ask. */
    if ((eDir == (U8)LOOP_DIR_LOAD) && (nBytes == 0UL))
    {
        return RESULT_NOT_OK;
    }

    FxLoop_Reset(pSession);

    pSession->nSession    = nSessionId;
    pSession->eDir        = eDir;
    pSession->nLooper     = nLooper;
    pSession->nPlaneQty   = nPlaneQty;
    pSession->eFormat     = eFormat;
    pSession->nSlotQty    = nSlotQty;
    pSession->nBytesTotal = nBytes;
    pSession->eState      = (U8)FX_LOOP_OPENING;

    pOpen->nBytes    = nBytes;
    pOpen->nSession  = nSessionId;
    pOpen->eDir      = eDir;
    pOpen->nLooper   = nLooper;
    pOpen->nPlaneQty = nPlaneQty;
    pOpen->eFormat   = eFormat;
    pOpen->nSlotQty  = nSlotQty;
    pOpen->aReserved[0] = 0U;
    pOpen->aReserved[1] = 0U;

    return RESULT_OK;
}


STD_RESULT FxLoop_Accept(FX_LOOP_SESSION* const pSession,
                         const PROTO_LOOP_OPEN* const pOpen,
                         const U32 nAvailBytes,
                         PROTO_LOOP_STAT* const pStat)
{
    U32 nBytes;

    if ((pSession == NULL_PTR) || (pOpen == NULL_PTR) || (pStat == NULL_PTR))
    {
        return RESULT_NOT_OK;
    }

    /*
     * Refusals are still replies, so the session id and a result go back in
     * every path below. The interface is holding staging memory open waiting
     * for this; a silent drop turns a refusal into a timeout, and a timeout
     * says nothing about why.
     */
    pStat->nSession = pOpen->nSession;
    pStat->nCrc     = 0UL;

    if (pSession->eState != (U8)FX_LOOP_IDLE)
    {
        pStat->nBytes   = 0UL;
        pStat->eState   = (U8)FX_LOOP_FAILED;
        pStat->eResult  = (U8)PROTO_RES_BUSY;
        pStat->nSlotQty = 0U;
        return RESULT_NOT_OK;
    }

    if (FxLoop_CheckGeometry(pOpen->eDir, pOpen->nLooper, pOpen->nPlaneQty,
                             pOpen->eFormat, pOpen->nSlotQty) != RESULT_OK)
    {
        pStat->nBytes   = 0UL;
        pStat->eState   = (U8)FX_LOOP_FAILED;
        pStat->eResult  = (U8)PROTO_RES_BAD_PARAM;
        pStat->nSlotQty = 0U;
        return RESULT_NOT_OK;
    }

    /*
     * WHO DECIDES THE BYTE COUNT
     *
     * On a SAVE this side owns the loop, so it owns the length: an nBytes of 0
     * means "tell me", and any other value is a claim about a loop the asker
     * cannot measure, so it is only honoured if it matches what is really
     * there. On a LOAD the asker owns the file and this side only has to fit
     * it.
     */
    if (pOpen->eDir == (U8)LOOP_DIR_SAVE)
    {
        nBytes = (pOpen->nBytes == 0UL) ? nAvailBytes : pOpen->nBytes;

        if (nBytes > nAvailBytes)
        {
            pStat->nBytes   = nAvailBytes;
            pStat->eState   = (U8)FX_LOOP_FAILED;
            pStat->eResult  = (U8)PROTO_RES_BAD_PARAM;
            pStat->nSlotQty = 0U;
            return RESULT_NOT_OK;
        }
    }
    else
    {
        nBytes = pOpen->nBytes;

        if (nBytes > nAvailBytes)
        {
            pStat->nBytes   = nAvailBytes;
            pStat->eState   = (U8)FX_LOOP_FAILED;
            pStat->eResult  = (U8)PROTO_RES_NO_SPACE;
            pStat->nSlotQty = 0U;
            return RESULT_NOT_OK;
        }
    }

    /* An empty loop is not a transfer. Saying so here is what stops the
       interface creating a zero-length file and calling it a take. */
    if (nBytes == 0UL)
    {
        pStat->nBytes   = 0UL;
        pStat->eState   = (U8)FX_LOOP_FAILED;
        pStat->eResult  = (U8)PROTO_RES_BAD_PARAM;
        pStat->nSlotQty = 0U;
        return RESULT_NOT_OK;
    }

    FxLoop_Reset(pSession);

    pSession->nSession    = pOpen->nSession;
    pSession->eDir        = pOpen->eDir;
    pSession->nLooper     = pOpen->nLooper;
    pSession->nPlaneQty   = pOpen->nPlaneQty;
    pSession->eFormat     = pOpen->eFormat;
    pSession->nSlotQty    = pOpen->nSlotQty;
    pSession->nBytesTotal = nBytes;

    /* Straight to READY: for the responder there is nothing to wait for. */
    pSession->eState      = (U8)FX_LOOP_READY;

    FxLoop_Report(pSession, pStat);

    return RESULT_OK;
}


STD_RESULT FxLoop_OpenReply(FX_LOOP_SESSION* const pSession,
                            const PROTO_LOOP_STAT* const pStat)
{
    if ((pSession == NULL_PTR) || (pStat == NULL_PTR))
    {
        return RESULT_NOT_OK;
    }

    /*
     * A reply to a session that is already gone. Ignoring it is deliberate:
     * failing the CURRENT session on a stale reply is how a slow answer kills
     * the transfer that replaced it, and the symptom - transfers that fail
     * only when one was aborted shortly before - is close to untraceable.
     */
    if (pStat->nSession != pSession->nSession)
    {
        return RESULT_NOT_OK;
    }

    if (pSession->eState != (U8)FX_LOOP_OPENING)
    {
        return RESULT_NOT_OK;
    }

    if ((pStat->eResult != (U8)PROTO_RES_OK) ||
        (pStat->eState  != (U8)FX_LOOP_READY))
    {
        pSession->eState  = (U8)FX_LOOP_FAILED;
        pSession->eResult = pStat->eResult;
        return RESULT_NOT_OK;
    }

    /*
     * The responder's numbers win. It may grant fewer slots than were asked
     * for, and on a save it is the side that knows how long the loop is - so
     * taking its answer here is what makes the two sides agree rather than
     * merely both be plausible.
     */
    pSession->nBytesTotal = pStat->nBytes;
    pSession->nSlotQty    = pStat->nSlotQty;
    pSession->eState      = (U8)FX_LOOP_READY;

    if ((pSession->nBytesTotal == 0UL) || (pSession->nSlotQty == 0U))
    {
        pSession->eState  = (U8)FX_LOOP_FAILED;
        pSession->eResult = (U8)PROTO_RES_BAD_PARAM;
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}


STD_RESULT FxLoop_Start(FX_LOOP_SESSION* const pSession)
{
    if (pSession == NULL_PTR)
    {
        return RESULT_NOT_OK;
    }

    if (pSession->eState != (U8)FX_LOOP_READY)
    {
        return RESULT_NOT_OK;
    }

    pSession->nBytesMoved = 0UL;
    pSession->nCrc        = (U32)FX_LOOP_CRC_INIT;
    pSession->eState      = (U8)FX_LOOP_RUNNING;

    return RESULT_OK;
}


STD_RESULT FxLoop_Advance(FX_LOOP_SESSION* const pSession,
                          const U8* const pData,
                          const U32 nBytes)
{
    if (pSession == NULL_PTR)
    {
        return RESULT_NOT_OK;
    }

    if (pSession->eState != (U8)FX_LOOP_RUNNING)
    {
        return RESULT_NOT_OK;
    }

    if (nBytes == 0UL)
    {
        return RESULT_OK;
    }

    /*
     * PAST THE END IS A FAILURE, NOT A CLAMP.
     *
     * Getting here means the two sides disagree about the size of the loop.
     * Clamping would write exactly nBytesTotal to the card and report success,
     * so the take would be short by however much the disagreement was and
     * nothing would ever say so.
     */
    if (nBytes > (pSession->nBytesTotal - pSession->nBytesMoved))
    {
        pSession->eState  = (U8)FX_LOOP_FAILED;
        pSession->eResult = (U8)PROTO_RES_BAD_PARAM;
        return RESULT_NOT_OK;
    }

    if (pData != NULL_PTR)
    {
        pSession->nCrc = Crc32_Ieee(pData, nBytes, pSession->nCrc);
    }

    pSession->nBytesMoved += nBytes;

    if (pSession->nBytesMoved == pSession->nBytesTotal)
    {
        pSession->eState = (U8)FX_LOOP_COMPLETE;
    }

    return RESULT_OK;
}


void FxLoop_Abort(FX_LOOP_SESSION* const pSession, const U8 eResult)
{
    if (pSession == NULL_PTR)
    {
        return;
    }

    pSession->eState  = (U8)FX_LOOP_FAILED;
    pSession->eResult = eResult;
}


void FxLoop_Report(const FX_LOOP_SESSION* const pSession,
                   PROTO_LOOP_STAT* const pStat)
{
    if (pStat == NULL_PTR)
    {
        return;
    }

    if (pSession == NULL_PTR)
    {
        pStat->nBytes   = 0UL;
        pStat->nCrc     = 0UL;
        pStat->nSession = 0U;
        pStat->eState   = (U8)FX_LOOP_FAILED;
        pStat->eResult  = (U8)PROTO_RES_BAD_PARAM;
        pStat->nSlotQty = 0U;
        return;
    }

    pStat->nBytes   = pSession->nBytesTotal;
    pStat->nSession = pSession->nSession;
    pStat->eState   = pSession->eState;
    pStat->eResult  = pSession->eResult;
    pStat->nSlotQty = pSession->nSlotQty;

    /* Only meaningful once every byte has been folded in. Reporting a partial
       CRC would invite the other side to compare it against a complete one. */
    pStat->nCrc = (pSession->eState == (U8)FX_LOOP_COMPLETE)
                      ? pSession->nCrc
                      : 0UL;
}


BOOLEAN FxLoop_IsStreaming(const FX_LOOP_SESSION* const pSession)
{
    if (pSession == NULL_PTR)
    {
        return FALSE;
    }

    return (pSession->eState == (U8)FX_LOOP_RUNNING) ? TRUE : FALSE;
}


U8 FxLoop_StreamWidth(const FX_LOOP_SESSION* const pSession)
{
    U8 nWidth = (U8)REC_SLOT_QTY;

    if (FxLoop_IsStreaming(pSession) == TRUE)
    {
        nWidth = (U8)((U32)REC_SLOT_QTY + (U32)pSession->nSlotQty);
    }

    return nWidth;
}

/****************************************** end of file *******************************************/
