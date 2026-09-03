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
#include "common_cfg.h"

#include "cmsis_os.h"

#include "usart.h"

#include <string.h>



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * RAM_D2, non-cacheable, via MPU region 0 - see MPU_Config in Init.c.
 *
 * Both directions need it now that the D-cache is on: the DMA writes aRxDma
 * and the CPU reads it, and the CPU writes aTxRing for the DMA to read. It is
 * also DMA1's own domain, so neither transfer crosses the D2-to-D1 bridge
 * that RAM_D1 required.
 *
 * .dma_buffers is NOLOAD, so nothing here starts zeroed. That is fine:
 * CtrlLinkIf_Init sets every index it uses, and the receive buffer is filled
 * by DMA before anything reads it.
 */
static U8 aRxDma[CTRL_IF_RX_DMA_BYTES]   IN_DMA_BUF;
static U8 aTxRing[CTRL_IF_TX_RING_BYTES] IN_DMA_BUF;

static volatile U16 nTxHead;
static volatile U16 nTxTail;
static volatile U16 nTxInFlight;

/** Where the receive DMA had got to at the previous poll. */
static U16 nRxLast;

/** Tick of the last ping, so a silent peer is probed at a bounded rate. */
static U32 nLastPingTick;

static BOOLEAN bBound;

static U32 nLastTelemetryTick;

static osThreadId xLinkThreadHandle;

static SUB_HANDLE xAckHandle;

/** The last ACK, kept so the recorder can ask at any time. */
static PROTO_ACK tLastAck;
static BOOLEAN   bHaveAck;

FXC_STATIC_ASSERT((CTRL_IF_TX_RING_BYTES & (CTRL_IF_TX_RING_BYTES - 1U)) == 0U,
                  ctrl_if_tx_ring_is_power_of_two);

/* An ACK generated while a configuration frame is still going out must have
 * somewhere to sit, which is the whole reason this is a ring. */
FXC_STATIC_ASSERT(CTRL_IF_TX_RING_BYTES >= (2U * PROTO_FRAME_MAX),
                  ctrl_if_tx_ring_holds_two_frames);



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Start a transmit if the UART is idle and there is anything queued.
 *
 * Called from the link task AND from the transmit-complete interrupt, so the
 * whole test-and-start is inside a critical section - the same treatment the
 * audio side's PumpTx gets, for the same reason.
 */
static void PumpTx(void)
{
    const U32 nPrimask = __get_PRIMASK();

    __disable_irq();

    if ((nTxInFlight == 0U) && (nTxHead != nTxTail))
    {
        /* Up to the end of the buffer, or up to the head - the DMA takes a
           pointer and a length, so a wrapped run goes out as two transfers. */
        const U16 nRun = (nTxHead > nTxTail) ? (U16)(nTxHead - nTxTail)
                                             : (U16)(CTRL_IF_TX_RING_BYTES - nTxTail);

        if (HAL_UART_Transmit_DMA(&huart2, &aTxRing[nTxTail], nRun) == HAL_OK)
        {
            nTxInFlight = nRun;
        }
    }

    __set_PRIMASK(nPrimask);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief The transmit callback handed to the shared framer.
 *
 * Copies into the ring rather than handing the framer's own buffer to the DMA:
 * that buffer is reused by the very next FxLink_Send, and pointing a transfer at
 * it would be the "mutate a live DMA buffer" bug this protocol was written to
 * get away from.
 */
static STD_RESULT LinkTx(const U8* const pData, const U16 nLength)
{
    STD_RESULT eResult = RESULT_NOT_OK;
    U16        nFree;
    U32        nPrimask;

    if ((pData == NULL_PTR) || (nLength == 0U))
    {
        return RESULT_INVALID_PARAM_1;
    }

    nPrimask = __get_PRIMASK();
    __disable_irq();

    nFree = (U16)((CTRL_IF_TX_RING_BYTES - 1U)
                  - ((U16)(nTxHead - nTxTail) & (CTRL_IF_TX_RING_BYTES - 1U)));

    if (nLength <= nFree)
    {
        U16 i;

        for (i = 0U; i < nLength; i++)
        {
            aTxRing[nTxHead] = pData[i];
            nTxHead = (U16)((nTxHead + 1U) & (CTRL_IF_TX_RING_BYTES - 1U));
        }

        eResult = RESULT_OK;
    }

    __set_PRIMASK(nPrimask);

    if (eResult == RESULT_OK)
    {
        PumpTx();
    }

    return eResult;
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
 * @brief Feed the framer everything the receive DMA has written since last time.
 *
 * The DMA never stops and never interrupts; NDTR counts DOWN, so the write
 * position is size - NDTR. Nothing here can block.
 */
static void DrainRx(void)
{
    U16 nWrite;

    if (huart2.hdmarx == NULL_PTR)
    {
        return;
    }

    nWrite = (U16)(CTRL_IF_RX_DMA_BYTES - __HAL_DMA_GET_COUNTER(huart2.hdmarx));

    while (nRxLast != nWrite)
    {
        FxLink_RxByte(aRxDma[nRxLast]);

        nRxLast++;
        if (nRxLast >= (U16)CTRL_IF_RX_DMA_BYTES)
        {
            nRxLast = 0U;
        }
    }
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

    if (HAL_UART_Receive_DMA(&huart2, aRxDma, (uint16_t)CTRL_IF_RX_DMA_BYTES) == HAL_OK)
    {
        nRxLast = 0U;
        bBound  = TRUE;
    }

    for (;;)
    {
        if (bBound != FALSE)
        {
            DrainRx();
            (void)FxLink_Poll();

            /* In case a transfer finished while the ring was empty and
               something has been queued since, with no interrupt left to
               start it. */
            PumpTx();

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
    nTxHead     = 0U;
    nTxTail     = 0U;
    nTxInFlight = 0U;
    nRxLast     = 0U;
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
 * NOTE: this arrives from USART2_IRQn, not from the transmit DMA stream's own
 * interrupt - the HAL routes UART DMA completion through the UART handler. Both
 * are enabled in usart.c and dma.c.
 *
 * Calls no FreeRTOS API, deliberately, so the priority relative to
 * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY cannot become a problem.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart == &huart2)
    {
        nTxTail     = (U16)((nTxTail + nTxInFlight) & (CTRL_IF_TX_RING_BYTES - 1U));
        nTxInFlight = 0U;

        PumpTx();
    }
}

//--------------------------------------------------------------------------------------------------

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart == &huart2)
    {
        const U32 nErr = HAL_UART_GetError(huart);

        /*
         * ONLY a DMA fault may touch the transmit bookkeeping.
         *
         * The tempting version of this clears nTxInFlight on every error. That
         * is wrong, and the common case - an RX overrun - is exactly when it
         * bites: the overrun says nothing about the transmitter, so a transfer
         * may still be in flight. Clear the count and, when that transfer
         * completes, HAL_UART_TxCpltCallback advances the tail by ZERO - so
         * every byte already on the wire is queued and sent a second time. The
         * CRC would reject the duplicate and resynchronise, but a link that
         * silently doubles frames under load is not a link worth debugging.
         */
        if ((nErr & HAL_UART_ERROR_DMA) != 0UL)
        {
            /* A failed transfer never reports completion, so the ring would
               stall behind it forever. Abort so the HAL state agrees with
               ours. */
            (void)HAL_UART_AbortTransmit(huart);
            nTxInFlight = 0U;
        }

        /*
         * Re-arm receive only if the HAL actually stopped it. The buffer is
         * circular and read by position, so a DMA that is still running needs
         * nothing done to it - and re-arming would reset nRxLast against a
         * stream that never restarted, losing byte alignment for one frame.
         */
        if (huart->RxState == HAL_UART_STATE_READY)
        {
            if (HAL_UART_Receive_DMA(huart, aRxDma, (uint16_t)CTRL_IF_RX_DMA_BYTES) == HAL_OK)
            {
                nRxLast = 0U;
                bBound  = TRUE;
            }
            else
            {
                bBound = FALSE;
            }
        }
    }
}

/****************************************** end of file *******************************************/
