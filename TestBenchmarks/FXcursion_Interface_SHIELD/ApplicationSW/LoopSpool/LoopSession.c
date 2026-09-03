/***************************************************************************************************
* @file     LoopSession.c
*
* @brief    Interface half of a loop transfer. See LoopSession.h.
*
***************************************************************************************************/

/***************************************************************************************************
* Included header files
***************************************************************************************************/

#include "LoopSession.h"

#include <string.h>
#include <stdio.h>

#include "ctrl_link_if.h"
#include "Recorder.h"
#include "fx_crc.h"
#include "fx_defs.h"

/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/**
 * Loop slots to ask for.
 *
 * The frame widens to REC_SLOT_QTY + this for the life of a transfer, so it is
 * a trade between how fast a loop moves and how much of each audio block the
 * SPI burst occupies:
 *
 *     slots   wire       burst at 96 MHz   one 5.5 MiB loop
 *        8    1.54 MB/s        19%              3.8 s
 *       16    3.07 MB/s        32%              1.9 s
 *
 * Sixteen, because the burst still leaves two thirds of every block free and
 * the audio board's buffer is released twice as fast. The card is slower than
 * either figure, which is the point of staging.
 */
#define LOOPSESSION_SLOT_QTY            (16U)

/** Wire format. Packed 24-bit is what the WAV wants and what the looper holds. */
#define LOOPSESSION_FORMAT              (LOOP_FMT_S24)

/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

static FX_LOOP_SESSION tSession;

/** Staging slot this transfer is using. */
static U8   nSlot;

/** File name, held until the transfer completes and the spooler takes it. */
static char aName[LOOPSPOOL_NAME_MAX];

/** Wraps. Only has to distinguish a reply from its predecessor. */
static U8   nNextSession = 1U;

static U32  nFailures;
static U8   eLastResult;

/** TRUE once LOOP_CTL(START) has gone out, so Poll knows to watch progress. */
static BOOLEAN bRunning;

/*
 * What the audio board computed over what it SENT, and whether it has said so.
 *
 * NOT tSession.nCrc. That field is maintained by FxLoop_Advance, and this side
 * never calls it - the MDMA route lands loop bytes in the staging slot without
 * the CPU seeing them, which is the whole point of routing them there. So the
 * session's own CRC stays at its initial value here and comparing against it
 * would fail every transfer.
 *
 * The comparison that means something is: this side's CRC over the staging
 * slot, against the far side's CRC over the looper. They cross in
 * LOOP_STAT(COMPLETE).
 */
static U32     nPeerCrc;
static BOOLEAN bPeerDone;

/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Give everything back. Safe to call from any state.
 *
 * Disarms the route BEFORE releasing the slot, because the route writes into
 * that slot: releasing first would hand a buffer to the spooler while the MDMA
 * was still landing blocks in it.
 */
static void SessionRelease(const U8 eResult)
{
    Recorder_DisarmLoopDest();

    if (nSlot != (U8)LOOPSPOOL_SLOT_NONE)
    {
        LoopSpool_Release(nSlot);
        nSlot = (U8)LOOPSPOOL_SLOT_NONE;
    }

    bRunning    = FALSE;
    eLastResult = eResult;

    /* Cleared with everything else. A stale bPeerDone would let the NEXT
       transfer's Poll close out on the previous one's report, comparing a fresh
       staging slot against a CRC from a loop that is no longer there. */
    bPeerDone   = FALSE;
    nPeerCrc    = 0UL;

    if (eResult != (U8)PROTO_RES_OK)
    {
        nFailures++;
    }

    FxLoop_Reset(&tSession);
}


/** @brief Send LOOP_CTL. */
static STD_RESULT SendCtl(const U8 eAction)
{
    PROTO_LOOP_CTL tCtl;

    tCtl.nSession    = tSession.nSession;
    tCtl.eAction     = eAction;
    tCtl.aReserved[0] = 0U;
    tCtl.aReserved[1] = 0U;

    return CtrlLinkIf_SendFrame((U8)PROTO_CMD_LOOP_CTL,
                                (const U8*)&tCtl, (U8)sizeof(tCtl));
}

/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT LoopSession_Init(void)
{
    FxLoop_Reset(&tSession);

    nSlot       = (U8)LOOPSPOOL_SLOT_NONE;
    bRunning    = FALSE;
    bPeerDone   = FALSE;
    nPeerCrc    = 0UL;
    nFailures   = 0UL;
    eLastResult = (U8)PROTO_RES_OK;

    (void)memset(aName, 0, sizeof(aName));

    return RESULT_OK;
}


STD_RESULT LoopSession_StartSave(const U8 nLooper,
                                 const U8 nPlaneQty,
                                 const char* const pName)
{
    PROTO_LOOP_OPEN tOpen;
    U8*             pBuf;

    if (LoopSession_IsBusy() == TRUE)
    {
        return RESULT_BUSY;
    }

    nSlot = LoopSpool_Acquire();

    if (nSlot == (U8)LOOPSPOOL_SLOT_NONE)
    {
        /* Both slots hold loops the card has not taken yet. Honest answer to
           "can you take another", and the reason PROTO_LOOP_OPEN can be
           refused with BUSY from this side too. */
        return RESULT_BUSY;
    }

    pBuf = LoopSpool_Buffer(nSlot);

    if (pBuf == NULL_PTR)
    {
        SessionRelease((U8)PROTO_RES_BAD_PARAM);
        return RESULT_NOT_OK;
    }

    if (pName != NULL_PTR)
    {
        (void)snprintf(aName, sizeof(aName), "%s", pName);
    }
    else
    {
        aName[0] = '\0';
    }

    /* nBytes 0: only the audio board knows how long the take is. */
    if (FxLoop_Open(&tSession, nNextSession, (U8)LOOP_DIR_SAVE, nLooper,
                    nPlaneQty, (U8)LOOPSESSION_FORMAT,
                    (U8)LOOPSESSION_SLOT_QTY, 0UL, &tOpen) != RESULT_OK)
    {
        SessionRelease((U8)PROTO_RES_BAD_PARAM);
        return RESULT_NOT_OK;
    }

    nNextSession++;

    /*
     * ARM THE ROUTE BEFORE ASKING. The audio board does not begin until
     * LOOP_CTL(START), which is sent from OnStat - but arming here rather than
     * there means the route is in place before any possibility of a block, and
     * an arm costs nothing while idle.
     */
    if (Recorder_ArmLoopDest((U32)(uintptr_t)pBuf, LoopSpool_SlotBytes()) != RESULT_OK)
    {
        SessionRelease((U8)PROTO_RES_BAD_PARAM);
        return RESULT_NOT_OK;
    }

    if (CtrlLinkIf_SendFrame((U8)PROTO_CMD_LOOP_OPEN,
                             (const U8*)&tOpen, (U8)sizeof(tOpen)) != RESULT_OK)
    {
        SessionRelease((U8)PROTO_RES_BUSY);
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}


STD_RESULT LoopSession_StartLoad(const char* const pName, const U8 nLooper)
{
    PROTO_LOOP_OPEN       tOpen;
    const LOOPSPOOL_INFO* pInfo;
    STD_RESULT            eResult;
    U8                    nLoaded = (U8)LOOPSPOOL_SLOT_NONE;

    if (LoopSession_IsBusy() == TRUE)
    {
        return RESULT_BUSY;
    }

    /* The file comes off the card FIRST and this blocks while it does. A size
       cannot be negotiated before it is known, and the audio board has to be
       told how much is coming before any of it arrives. */
    eResult = LoopSpool_Load(pName, &nLoaded);

    if (eResult != RESULT_OK)
    {
        /* Passed through unchanged, so "wrong sample rate" reaches the caller
           as itself rather than as a generic failure. */
        eLastResult = (U8)PROTO_RES_BAD_PARAM;
        return eResult;
    }

    nSlot = nLoaded;
    pInfo = LoopSpool_Info(nSlot);

    if (pInfo == NULL_PTR)
    {
        SessionRelease((U8)PROTO_RES_BAD_PARAM);
        return RESULT_NOT_OK;
    }

    if (FxLoop_Open(&tSession, nNextSession, (U8)LOOP_DIR_LOAD, nLooper,
                    pInfo->nPlaneQty, (U8)LOOPSESSION_FORMAT,
                    (U8)LOOPSESSION_SLOT_QTY, pInfo->nBytes, &tOpen) != RESULT_OK)
    {
        SessionRelease((U8)PROTO_RES_BAD_PARAM);
        return RESULT_NOT_OK;
    }

    nNextSession++;

    if (CtrlLinkIf_SendFrame((U8)PROTO_CMD_LOOP_OPEN,
                             (const U8*)&tOpen, (U8)sizeof(tOpen)) != RESULT_OK)
    {
        SessionRelease((U8)PROTO_RES_BUSY);
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}


void LoopSession_OnStat(const PROTO_LOOP_STAT* const pStat)
{
    if (pStat == NULL_PTR)
    {
        return;
    }

    /*
     * A reply to a session that has already gone. FxLoop_OpenReply drops it by
     * session id, and the check is repeated here so that a stale COMPLETE
     * cannot close the transfer that replaced it either.
     */
    if (pStat->nSession != tSession.nSession)
    {
        return;
    }

    switch (tSession.eState)
    {
        case (U8)FX_LOOP_OPENING:
            if (FxLoop_OpenReply(&tSession, pStat) != RESULT_OK)
            {
                SessionRelease(tSession.eResult);
                return;
            }

            /*
             * Both sides now agree on the size and the slot count. The route is
             * already armed, so the audio board can be told to begin.
             *
             * Re-armed with the AGREED size rather than the whole slot: the
             * audio board's answer is authoritative on a save, and bounding the
             * route to it means a stream that runs long is refused by
             * RecorderLoopDest instead of overrunning into the next slot.
             */
            (void)Recorder_DisarmLoopDest();

            if (tSession.eDir == (U8)LOOP_DIR_SAVE)
            {
                U8* const pBuf = LoopSpool_Buffer(nSlot);

                if ((pBuf == NULL_PTR) ||
                    (tSession.nBytesTotal > LoopSpool_SlotBytes()) ||
                    (Recorder_ArmLoopDest((U32)(uintptr_t)pBuf,
                                          tSession.nBytesTotal) != RESULT_OK))
                {
                    SessionRelease((U8)PROTO_RES_NO_SPACE);
                    return;
                }
            }

            if (SendCtl((U8)LOOP_ACT_START) != RESULT_OK)
            {
                SessionRelease((U8)PROTO_RES_BUSY);
                return;
            }

            (void)FxLoop_Start(&tSession);
            bRunning = TRUE;
            break;

        case (U8)FX_LOOP_RUNNING:
            if (pStat->eState == (U8)FX_LOOP_FAILED)
            {
                SessionRelease(pStat->eResult);
            }
            else if (pStat->eState == (U8)FX_LOOP_COMPLETE)
            {
                /* The far side has sent everything and told us its CRC. Kept
                   rather than acted on: this side may still be a block or two
                   behind, and Poll closes out once its own count agrees. The
                   comparison and the spool commit are bulk work that belongs in
                   a thread. */
                nPeerCrc  = pStat->nCrc;
                bPeerDone = TRUE;
            }
            else
            {
                do_nothing();
            }
            break;

        default:
            /* Nothing in flight, or already finishing. */
            break;
    }
}


void LoopSession_Poll(void)
{
    U32 nTaken;
    U32 nCrc;

    if (bRunning == FALSE)
    {
        return;
    }

    if (tSession.eDir != (U8)LOOP_DIR_SAVE)
    {
        /*
         * A LOAD is driven by the audio board consuming what this side puts on
         * MISO, so there is no local progress counter to watch - the transfer
         * is finished when the far end says so. Left explicit rather than
         * folded into the save path, because the two really are different
         * machines and pretending otherwise is how one of them gets the other's
         * completion test.
         */
        if (tSession.eState == (U8)FX_LOOP_COMPLETE)
        {
            SessionRelease((U8)PROTO_RES_OK);
        }
        return;
    }

    nTaken = Recorder_LoopBytesTaken();

    /*
     * BOTH conditions. Every byte has landed here AND the far side has said it
     * sent everything - which is also how its CRC arrives.
     *
     * Waiting only on the local count would run the comparison against a CRC
     * that had not been received yet; waiting only on the far side's report
     * would read a staging slot the MDMA had not finished filling.
     */
    if ((nTaken < tSession.nBytesTotal) || (bPeerDone == FALSE))
    {
        return;
    }

    /* Every byte has landed. Stop routing before anything reads the buffer. */
    Recorder_DisarmLoopDest();
    bRunning = FALSE;

    /*
     * CRC over exactly what arrived, compared against what the audio board
     * computed over what it sent. This is the ONLY check on a link that carries
     * no CRC of its own, and at 96 MHz over flying leads it is the first thing
     * that will fail if the wiring is marginal.
     *
     * Several megabytes of non-cached SDRAM, so tens of milliseconds - which is
     * why this is in a polled thread and not in the DMA callback that could
     * have detected completion a little sooner.
     */
    nCrc = Crc32_Ieee(LoopSpool_Buffer(nSlot), tSession.nBytesTotal, 0UL);

    if (nCrc != nPeerCrc)
    {
        /* Do NOT spool it. A loop that arrived corrupted is worse on the card
           than absent: it looks like a take that can be loaded. */
        SessionRelease((U8)PROTO_RES_BAD_PARAM);
        return;
    }

    if (LoopSpool_Commit(nSlot, tSession.nBytesTotal, tSession.nPlaneQty,
                         tSession.eFormat, nCrc,
                         (aName[0] != '\0') ? aName : NULL_PTR) != RESULT_OK)
    {
        SessionRelease((U8)PROTO_RES_BAD_PARAM);
        return;
    }

    /* The slot belongs to the spooler now - it will free it once written - so
       this must not release it. */
    nSlot       = (U8)LOOPSPOOL_SLOT_NONE;
    eLastResult = (U8)PROTO_RES_OK;

    FxLoop_Reset(&tSession);
}


BOOLEAN LoopSession_IsBusy(void)
{
    return (tSession.eState != (U8)FX_LOOP_IDLE) ? TRUE : FALSE;
}


const FX_LOOP_SESSION* LoopSession_Get(void)
{
    return &tSession;
}


U32 LoopSession_Failures(void)
{
    return nFailures;
}


U8 LoopSession_LastResult(void)
{
    return eLastResult;
}

/****************************************** end of file *******************************************/
