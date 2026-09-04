/**
 * @file      ctrl_link_if.c
 *
 * @details   Interface-side control link transport. See ctrl_link_if.h.
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

#include "ctrl_link_if.h"

#include "fx_link.h"
#include "pubsub.h"

/* Owns the loop transfer state machine; this file only routes its commands. */
#include "LoopSession.h"
#include "common_cfg.h"

#include "cmsis_os.h"

/* The UART, its baud rate and its callbacks belong to the transport; nothing
   in this file names a peripheral any more. */
#include "uart_tp.h"

#include <string.h>



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * The buffers, the ring indices and the DMA placement that used to be here all
 * belong to UART_TP now - including the RAM_D2 non-cacheable placement, which
 * it takes from UART_TP_DMA_SECTION in uart_tp_cfg.h.
 *
 * This file is the PROTOCOL side of the link: what to send, when to ping, what
 * to do with an ACK. It no longer knows which USART carries any of it.
 */

/** Tick of the last ping, so a silent peer is probed at a bounded rate. */
static U32 nLastPingTick;

static BOOLEAN bBound;

static U32 nLastTelemetryTick;

static osThreadId xLinkThreadHandle;

static SUB_HANDLE xAckHandle;

/** The last ACK, kept so the recorder can ask at any time. */
static PROTO_ACK tLastAck;
static BOOLEAN   bHaveAck;

/* An ACK generated while a configuration frame is still going out must have
 * somewhere to sit. The queue is the transport's now, so the requirement is
 * checked against ITS size - the invariant did not move, only the buffer. */
FXC_STATIC_ASSERT(UART_TP_TX_QUEUE_BYTES >= (2U * PROTO_FRAME_MAX),
                  ctrl_if_tx_queue_holds_two_frames);



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief The transmit callback handed to the shared framer.
 *
 * Copies into the transport's queue rather than handing the framer's own buffer
 * to the DMA: that buffer is reused by the very next FxLink_Send, and pointing a
 * transfer at it would be the "mutate a live DMA buffer" bug this protocol was
 * written to get away from. UART_TP_Send does that copy.
 *
 * WHAT MOVED. This file used to carry its own transmit ring, its own PumpTx
 * with a critical section around the test-and-start, its own circular receive
 * DMA, and both HAL UART callbacks. All of it is UART_TP now, byte for byte the
 * same design - it was duplicated here and in the audio controller's
 * ctrl_uart.c, which is exactly the kind of thing SystemSW exists to hold once.
 *
 * The refusal semantics are unchanged: a frame that will not fit is rejected
 * WHOLE, because half a frame on the wire makes the far end resynchronise.
 */
static STD_RESULT LinkTx(const U8* const pData, const U16 nLength)
{
    if ((pData == NULL_PTR) || (nLength == 0U))
    {
        return RESULT_INVALID_PARAM_1;
    }

    return UART_TP_Send(pData, nLength);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief One accepted frame from the audio controller.
 *
 * Everything is republished rather than acted on here. The GUI owns the model;
 * this task owns a UART. Keeping that line clean is why a TouchGFX screen can be
 * rewritten without touching the link, and vice versa.
 */
static void Dispatch(const U8 eCmd, const U8* const pPayload, const U8 nLength)
{
    switch (eCmd)
    {
        case (U8)PROTO_CMD_TELEMETRY:
            if (nLength == (U8)sizeof(PROTO_TELEMETRY))
            {
                nLastTelemetryTick = osKernelSysTick();
                (void)PUBSUB_Publish(PUBSUB_TOPIC_TELEMETRY, (void*)pPayload, nLength);
            }
            break;

        case (U8)PROTO_CMD_ACK:
            if (nLength == (U8)sizeof(PROTO_ACK))
            {
                /* Carries the recorder slot map that was actually committed.
                   Whoever reprograms the MDMA de-interleave has to see this
                   before the stream is asked for - see fx_protocol.h. */
                (void)memcpy(&tLastAck, pPayload, sizeof(tLastAck));
                bHaveAck = TRUE;

                (void)PUBSUB_Publish(PUBSUB_TOPIC_ACK, (void*)pPayload, nLength);
            }
            break;

        case (U8)PROTO_CMD_DIAG:
            if (nLength == (U8)sizeof(PROTO_DIAG))
            {
                (void)PUBSUB_Publish(PUBSUB_TOPIC_DIAG, (void*)pPayload, nLength);
            }
            break;

        case (U8)PROTO_CMD_LOOP_STAT:
            if (nLength == (U8)sizeof(PROTO_LOOP_STAT))
            {
                /* Handed straight over rather than republished. A loop transfer
                   is a conversation with one owner, not a value several screens
                   might want - and it has to be answered in order, which a
                   topic cannot guarantee.

                   Copied out of the byte array first: pPayload is U8-aligned,
                   and reading a U32 field through a cast to it is an unaligned
                   access that faults wherever the MPU forbids one. */
                PROTO_LOOP_STAT tStat;

                (void)memcpy(&tStat, pPayload, sizeof(tStat));
                LoopSession_OnStat(&tStat);
            }
            break;

        case (U8)PROTO_CMD_PONG:
            /* Liveness only. FxLink has already counted the frame. */
            do_nothing();
            break;

        default:
            /* A report from a newer audio build. Dropped, not resynchronised
               on - the frame was well formed and CRC-clean. */
            do_nothing();
            break;
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Feed the framer everything the transport has received since last time.
 *
 * Nothing here can block: UART_TP_Read returns 0 when the ring is empty rather
 * than waiting.
 */
static void DrainRx(void)
{
    U8      aChunk[64];
    BOOLEAN bOverrun = FALSE;
    U16     nQty;

    /* Drain in chunks until the transport has nothing left. The ring walk and
       the NDTR arithmetic that used to be here are UART_TP's now. */
    do
    {
        U16 i;

        nQty = UART_TP_Read(aChunk, (U16)sizeof(aChunk), &bOverrun);

        /*
         * Bytes were lost BEFORE these ones. Telling the framer to drop what it
         * has half-assembled is the point of surfacing this: it is already
         * holding the front of a frame whose middle is missing, and letting it
         * run on would have it accept a frame spliced from two - which the CRC
         * would probably catch, but "probably" is not the same as
         * resynchronising deliberately.
         */
        if (bOverrun == TRUE)
        {
            FxLink_Resync();
        }

        for (i = 0U; i < nQty; i++)
        {
            FxLink_RxByte(aChunk[i]);
        }

    } while (nQty == (U16)sizeof(aChunk));
}

//--------------------------------------------------------------------------------------------------

static void LinkThreadWrapper(void const* argument)
{
    (void)argument;

    (void)PUBSUB_CreateTopic(PUBSUB_TOPIC_TELEMETRY, (U16)sizeof(PROTO_TELEMETRY));
    (void)PUBSUB_CreateTopic(PUBSUB_TOPIC_ACK,       (U16)sizeof(PROTO_ACK));
    (void)PUBSUB_CreateTopic(PUBSUB_TOPIC_DIAG,      (U16)sizeof(PROTO_DIAG));

    /* Subscribed so the ACK topic retains its last value even when no screen is
       looking - the recorder needs the slot map whenever it is next armed, not
       only if a view happened to be open when it arrived. */
    xAckHandle = PUBSUB_Subscribe(PUBSUB_TOPIC_ACK, NULL);

    /* Takes the peripheral, applies UART_TP_BAUDRATE and arms the circular
       receive. Which USART and how fast are both in uart_tp_cfg.h now, so this
       file no longer names either. */
    if (UART_TP_Init() == RESULT_OK)
    {
        bBound = TRUE;
    }

    for (;;)
    {
        if (bBound != FALSE)
        {
            DrainRx();
            (void)FxLink_Poll();

            /* Drives a loop transfer to completion: watches how much the MDMA
               route has landed, then does the CRC and the spool commit. Both
               are bulk work and belong in a thread, which is why the transfer
               is polled rather than closed out from the DMA callback that could
               have noticed a little sooner. */
            LoopSession_Poll();

            /* In case a transfer finished while the queue was empty and
               something has been queued since, with no interrupt left to
               start it. */
            UART_TP_Poll();

            /*
             * Probe a silent peer. This is the only traffic this task
             * generates on its own - once telemetry is flowing,
             * IsPeerAlive is TRUE and nothing is sent.
             */
            if (CtrlLinkIf_IsPeerAlive() == FALSE)
            {
                const U32 nNow = osKernelSysTick();

                /* Unsigned subtraction, correct across the tick wrap. */
                if ((nNow - nLastPingTick) >= (U32)CTRL_IF_PING_MS)
                {
                    nLastPingTick = nNow;
                    (void)CtrlLinkIf_Ping();
                }
            }
        }

        osDelay(CTRL_IF_POLL_MS);
    }
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT CtrlLinkIf_Init(void)
{
    /* The ring indices that used to be reset here belong to UART_TP, which
       clears them in UART_TP_Init - called from the link thread once the
       topics exist. */
    bBound      = FALSE;

    nLastPingTick = 0UL;

    /* Zero, not "now": the peer has not spoken yet, and seeding this with the
       current tick would report it alive for the first timeout period. */
    nLastTelemetryTick = 0UL;
    bHaveAck           = FALSE;

    if (FxLink_Init(&LinkTx, &Dispatch) != RESULT_OK)
    {
        return RESULT_NOT_OK;
    }

    osThreadDef(CtrlLinkThread, LinkThreadWrapper, osPriorityNormal, 0, 1024U);
    xLinkThreadHandle = osThreadCreate(osThread(CtrlLinkThread), NULL);

    if (NULL == xLinkThreadHandle)
    {
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLinkIf_SendConfig(const PROTO_CFG* const pCfg)
{
    if (pCfg == NULL_PTR)
    {
        return RESULT_INVALID_PARAM_1;
    }

    return FxLink_Send((U8)PROTO_CMD_SET_CONFIG, (const U8*)pCfg, (U8)sizeof(*pCfg));
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLinkIf_SetParam(const PROTO_SET_PARAM* const pCmd)
{
    if (pCmd == NULL_PTR)
    {
        return RESULT_INVALID_PARAM_1;
    }

    return FxLink_Send((U8)PROTO_CMD_SET_PARAM, (const U8*)pCmd, (U8)sizeof(*pCmd));
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLinkIf_SetTempo(const U16 nBpmX10, const U8 nBeatsPerBar, const U8 nBeatUnit)
{
    PROTO_SET_TEMPO tCmd;

    tCmd.nBpmX10      = nBpmX10;
    tCmd.nBeatsPerBar = nBeatsPerBar;
    tCmd.nBeatUnit    = nBeatUnit;

    return FxLink_Send((U8)PROTO_CMD_SET_TEMPO, (const U8*)&tCmd, (U8)sizeof(tCmd));
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLinkIf_Transport(const U8 nChain, const U8 eAction)
{
    PROTO_TRANSPORT tCmd;

    tCmd.nChain       = nChain;
    tCmd.eAction      = eAction;
    tCmd.nReserved[0] = 0U;
    tCmd.nReserved[1] = 0U;

    return FxLink_Send((U8)PROTO_CMD_TRANSPORT, (const U8*)&tCmd, (U8)sizeof(tCmd));
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLinkIf_SendFrame(const U8 eCmd, const U8* const pPayload, const U8 nLength)
{
    /*
     * The generic send, for callers that build their own payload.
     *
     * Every other command here has a typed wrapper, which is the right shape
     * when this file knows what the command means. It does not know what a loop
     * transfer means and should not: LoopSession owns that state machine, and
     * giving it a typed wrapper per command would put the protocol's shape in
     * two places.
     */
    if ((pPayload == NULL_PTR) && (nLength != 0U))
    {
        return RESULT_INVALID_PARAM_2;
    }

    return FxLink_Send(eCmd, pPayload, nLength);
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLinkIf_Stream(const BOOLEAN bEnable)
{
    PROTO_STREAM tCmd;

    tCmd.bEnable      = (U8)((bEnable != FALSE) ? TRUE : FALSE);
    tCmd.nReserved[0] = 0U;
    tCmd.nReserved[1] = 0U;
    tCmd.nReserved[2] = 0U;

    return FxLink_Send((U8)PROTO_CMD_STREAM, (const U8*)&tCmd, (U8)sizeof(tCmd));
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLinkIf_Ping(void)
{
    return FxLink_Send((U8)PROTO_CMD_PING, NULL_PTR, 0U);
}

//--------------------------------------------------------------------------------------------------

BOOLEAN CtrlLinkIf_IsPeerAlive(void)
{
    BOOLEAN bAlive = FALSE;

    if (nLastTelemetryTick != 0UL)
    {
        /* Unsigned subtraction, so this stays correct across the tick wrap. */
        const U32 nAge = osKernelSysTick() - nLastTelemetryTick;

        bAlive = (nAge < (U32)CTRL_IF_PEER_TIMEOUT_MS) ? TRUE : FALSE;
    }

    return bAlive;
}

//--------------------------------------------------------------------------------------------------

const FX_LINK_STATS* CtrlLinkIf_Stats(void)
{
    return FxLink_Stats();
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLinkIf_GetAck(PROTO_ACK* const pAck)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if (pAck == NULL_PTR)
    {
        eResult = RESULT_INVALID_PARAM_1;
    }
    else if (bHaveAck != FALSE)
    {
        /* One frame is 8 bytes and the link task only ever replaces it whole,
           so a reader cannot see half of two different ACKs. */
        (void)memcpy(pAck, &tLastAck, sizeof(*pAck));
        eResult = RESULT_OK;
    }
    else
    {
        do_nothing();
    }

    return eResult;
}



/***************************************************************************************************
* HAL callbacks
***************************************************************************************************/
/*
 * The two HAL UART callbacks that used to live here - transmit-complete and
 * error - belong to UART_TP now. It owns those weak symbols because it owns the
 * buffers they manipulate, and the error recovery in particular has to be one
 * decision: whether to re-arm a circular receive depends on whether the HAL
 * stopped it, which only the code that armed it can know.
 *
 * The loss that error recovery used to hide is now REPORTED instead. When
 * UART_TP re-arms after an error it tells DrainRx through the overrun flag, and
 * DrainRx calls FxLink_Resync - so the framer abandons the frame it was
 * assembling deliberately, rather than discovering the damage from a CRC one
 * frame later.
 */

/****************************************** end of file *******************************************/
