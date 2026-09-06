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

/* REC_SLOTS_PER_FRAME - where the recorder slots end and the loop slots begin. */
#include "common_cfg.h"

/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/**
 * Loop slots to ask for.
 *
 * The frame widens to REC_SLOTS_PER_FRAME + this for the life of a transfer, so
 * it trades how fast a loop moves against how much of each audio block the SPI
 * burst occupies:
 *
 *     slots   width   wire        burst at 96 MHz   one 5.5 MiB loop
 *        4       8    0.77 MB/s        13%              7.5 s
 *       12      16    2.30 MB/s        26%              2.5 s
 *       28      32    5.38 MB/s        51%              1.1 s
 *
 * TWELVE IS NOT A BANDWIDTH CHOICE. The total width has to divide the receiver's
 * half-ring exactly or the half boundary lands mid-frame - see
 * FX_LOOP_SLOT_QTY_MAX. 4096 words per half divides by 8, 16 and 32, so the
 * legal loop counts are 4, 12 and 28. Twelve is the middle one and leaves three
 * quarters of every block free.
 *
 * Asking for anything else is refused by the de-interleave rather than
 * misbehaving, so this cannot quietly become wrong - but it would stop loop
 * transfers entirely, which is why it is pinned to the cap below.
 */
#define LOOPSESSION_SLOT_QTY            (FX_LOOP_SLOT_QTY_MAX)

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

/**
 * How far a LOAD has got, in bytes handed to the transmit ring.
 *
 * The save direction has Recorder_LoopBytesTaken, which counts what the MDMA
 * actually landed. A load has no such witness on this side - the bytes leave
 * when the master clocks them - so this counts what was handed over, and the
 * transfer is finished when the far side says it received them all.
 */
static volatile U32 nTxOfs;

/**
 * @brief Polls since this session last moved a byte, and what it had moved.
 *
 * The stall watchdog in LoopSession_Poll - see there for why it counts polls
 * rather than milliseconds, and why it watches progress rather than age.
 */
static U32 nStallPolls;
static U32 nLastProgress;

/**
 * @brief How long a session may move nothing at all before it is failed.
 *
 * Three seconds. Well past any legitimate pause: the link moves a 20-second
 * take in about 1.1 s, and the card is the only slow part of a save - and the
 * card is not in this path, the spooler takes it afterwards.
 */
#define LOOPSESSION_STALL_MS            (3000U)
#define LOOPSESSION_STALL_POLLS         (LOOPSESSION_STALL_MS / CTRL_IF_POLL_MS)

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
    nTxOfs      = 0UL;

    nStallPolls   = 0UL;
    nLastProgress = 0UL;

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
    nTxOfs      = 0UL;
    nFailures   = 0UL;
    eLastResult = (U8)PROTO_RES_OK;

    nStallPolls   = 0UL;
    nLastProgress = 0UL;

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
                U8* const pBuf  = LoopSpool_Buffer(nSlot);
                const U32 nStep  = Recorder_LoopRouteBytes();
                U32       nArm   = tSession.nBytesTotal;

                /*
                 * Rounded UP to the route's granularity. The route moves a
                 * whole half of loop slots at a time and a loop almost never
                 * ends on that boundary, so bounding it to the exact byte
                 * count has RecorderLoopDest refuse the final half - the
                 * count never reaches nBytesTotal and the save never finishes.
                 *
                 * The padding is the audio board's own, and both the CRC and
                 * the file below stop at nBytesTotal, so it is never read.
                 * One extra half always fits: a slot is 5 767 168 B against a
                 * 5 760 000 B maximum loop, and a half is 13 824 B.
                 */
                if ((nStep != 0UL) && ((nArm % nStep) != 0UL))
                {
                    nArm += nStep - (nArm % nStep);
                }

                if ((pBuf == NULL_PTR) ||
                    (nArm > LoopSpool_SlotBytes()) ||
                    (Recorder_ArmLoopDest((U32)(uintptr_t)pBuf,
                                          nArm) != RESULT_OK))
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
    U32 nProgress;

    if (tSession.eState == (U8)FX_LOOP_IDLE)
    {
        nStallPolls    = 0UL;
        nLastProgress  = 0UL;
        return;
    }

    /*
     * THE WATCHDOG, and it covers OPENING as well as RUNNING.
     *
     * Nothing on this path had a deadline. A session waited on a byte count or
     * on a reply, and if the audio board reset in between it waited for the
     * rest of the run - holding a staging slot, so every later transfer was
     * refused as busy. That is the failure this exists for, and it needs no
     * new clock: Poll is called once per CTRL_IF_POLL_MS by the CtrlLink task.
     *
     * Progress, not elapsed time. A slow card or a busy link must not fail a
     * transfer that is still moving, so the counter resets on every byte that
     * lands. Only a session that has moved NOTHING for the whole window is
     * declared dead.
     */
    nProgress = (tSession.eDir == (U8)LOOP_DIR_SAVE)
                    ? Recorder_LoopBytesTaken()
                    : nTxOfs;

    if (nProgress != nLastProgress)
    {
        nLastProgress = nProgress;
        nStallPolls   = 0UL;
    }
    else
    {
        nStallPolls++;
    }

    /*
     * A dead peer is decided immediately - there is no point waiting out the
     * stall window for a board that has stopped answering pings at all.
     */
    if ((nStallPolls > (U32)LOOPSESSION_STALL_POLLS) ||
        (CtrlLinkIf_IsPeerAlive() == FALSE))
    {
        /* Best effort: if the peer is gone this will not arrive, and if it is
           merely wedged this is what unwedges it. */
        (void)SendCtl((U8)LOOP_ACT_ABORT);

        SessionRelease((U8)PROTO_RES_TIMEOUT);
        return;
    }

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
         *
         * THE FAR SIDE'S REPORT IS THE ONLY SIGNAL. This used to wait on the
         * local eState reaching COMPLETE, which nothing on a load ever sets:
         * FxLoop_Advance is driven by bytes ARRIVING, and on a load they are
         * leaving. The session sat in RUNNING for good, and with the slot never
         * released the next load was refused as busy.
         */
        if (bPeerDone == FALSE)
        {
            return;
        }

        bRunning = FALSE;

        /*
         * The far side checksummed what it received; this side checksums what
         * it was asked to send. Same comparison as a save, in the same place,
         * for the same reason - the SPI stream carries no CRC of its own, so
         * this is the only thing standing between marginal wiring and a loop
         * that plays back as noise.
         */
        nCrc = Crc32_Ieee(LoopSpool_Buffer(nSlot), tSession.nBytesTotal, 0UL);

        SessionRelease((nCrc == nPeerCrc) ? (U8)PROTO_RES_OK
                                          : (U8)PROTO_RES_BAD_PARAM);
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


/*
 * Sequence number for the frames THIS board sends. Independent of the one the
 * audio board sends - the two directions are separate streams that happen to
 * share a clock, and pairing their counters would only invent a coupling the
 * hardware does not have.
 *
 * Free-running from boot and never reset by a session starting or ending: it
 * numbers frames on the wire, not payload, so an idle link keeps counting. That
 * is what lets the far side tell "nothing to send" from "the link stopped".
 */
static U16 nTxSeq = 0U;

void LoopSession_FillTx(S32* const pFrames, const U32 nFrames, const U8 nStride)
{
    const U8* pSrc  = NULL_PTR;
    BOOLEAN   bSend = FALSE;
    U32       nFrame;

    if ((pFrames == NULL_PTR) || (nStride != (U8)FX_FRAME_SLOT_QTY))
    {
        return;
    }

    /*
     * SENDING or not, every slot of every frame is written.
     *
     * The ring is armed for the life of the link, so whatever is in it goes out
     * on every lap. A half still holding the tail of a finished load would send
     * that tail again, forever, into a looper that has moved on - so the idle
     * case is not "skip", it is "write zeros".
     *
     * Writing the whole half unconditionally costs about 1% of one core and
     * removes a class of bug outright: there is no state that says which slots
     * are stale, because none of them ever are.
     */
    if ((bRunning != FALSE) && (tSession.eDir == (U8)LOOP_DIR_LOAD))
    {
        pSrc = LoopSpool_Buffer(nSlot);

        bSend = (pSrc != NULL_PTR) ? TRUE : FALSE;
    }

    for (nFrame = 0UL; nFrame < nFrames; nFrame++)
    {
        S32* const pFrame = &pFrames[nFrame * (U32)nStride];
        U8         s;

        /*
         * Slot 0 first. The far side reads this before it trusts anything else
         * in the frame, and a frame written without it is a frame that will be
         * discarded - so it is not optional even on an idle link.
         */
        pFrame[FX_FRAME_SYNC_SLOT] = FX_FRAME_SYNC_WORD(nTxSeq);
        nTxSeq++;

        /*
         * Everything between the sync word and the file run belongs to the
         * other direction: the recorder planes, the live looper planes and the
         * status word are all things the AUDIO board sends. This board has
         * nothing to put in them, so it sends zeros.
         *
         * One loop rather than three, deliberately. The three regions are
         * contiguous by construction - the static assert in Recorder.c pins
         * that they account for the frame exactly - and writing them
         * individually invites the version where a region is added to
         * fx_frame.h and this one place still zeroes the old set, leaving the
         * new slots carrying whatever the previous frame left in them.
         */
        for (s = (U8)FX_FRAME_REC_SLOT_BASE; s < (U8)FX_FRAME_LOOP_SLOT_BASE; s++)
        {
            pFrame[(U32)s] = 0L;
        }

        /*
         * Four payload bytes per slot, the exact mirror of the audio board's
         * pack. The two have to agree byte for byte: a disagreement about how
         * many bytes a slot holds does not fail, it just delivers a loop that
         * is stretched or truncated.
         */
        for (s = 0U; s < (U8)FX_FRAME_LOOP_SLOT_QTY; s++)
        {
            U32 nWord = 0UL;

            if (bSend != FALSE)
            {
                U8 b;

                for (b = 0U; b < 4U; b++)
                {
                    /* Past the end: pad. The far side stops at nBytesTotal, so
                       these bytes are clocked but never stored. */
                    if (nTxOfs < tSession.nBytesTotal)
                    {
                        nWord |= ((U32)pSrc[nTxOfs]) << (8U * b);
                        nTxOfs++;
                    }
                }
            }

            pFrame[(U32)FX_FRAME_LOOP_SLOT_BASE + s] = (S32)nWord;
        }
    }
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
