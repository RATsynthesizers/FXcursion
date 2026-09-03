/**
 * @file      uart_tp.c
 *
 * @details   UART byte transport. See uart_tp.h.
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "uart_tp.h"

#if (UART_TP_IN_USE == ON)

#include <string.h>

// Get the CubeMX UART handles
#include "usart.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/*
 * Resolve the configured peripheral to its CubeMX handle at COMPILE TIME.
 *
 * A runtime table would mean this module could not know which IRQ to touch
 * without a search, and would leave every unused handle linked in. This way an
 * instance the project has not generated fails the build here, naming the
 * peripheral, rather than failing at link time on a missing symbol.
 */
#if   (UART_TP_INSTANCE == UART_TP_USART1)
    #define UART_TP_HANDLE              (huart1)
#elif (UART_TP_INSTANCE == UART_TP_USART2)
    #define UART_TP_HANDLE              (huart2)
#elif (UART_TP_INSTANCE == UART_TP_USART3)
    #define UART_TP_HANDLE              (huart3)
#elif (UART_TP_INSTANCE == UART_TP_UART4)
    #define UART_TP_HANDLE              (huart4)
#elif (UART_TP_INSTANCE == UART_TP_UART5)
    #define UART_TP_HANDLE              (huart5)
#elif (UART_TP_INSTANCE == UART_TP_USART6)
    #define UART_TP_HANDLE              (huart6)
#elif (UART_TP_INSTANCE == UART_TP_UART7)
    #define UART_TP_HANDLE              (huart7)
#elif (UART_TP_INSTANCE == UART_TP_UART8)
    #define UART_TP_HANDLE              (huart8)
#else
    #error "uart_tp_cfg.h: UART_TP_INSTANCE is not one of the UART_TP_* selectors"
#endif

/* The ring must be a power of two: the write index comes from the DMA's own
   remaining-count register, and turning that into a ring position is a mask
   rather than a modulo. A modulo would work but this runs on every read. */
#if ((UART_TP_RX_RING_BYTES & (UART_TP_RX_RING_BYTES - 1U)) != 0U)
    #error "uart_tp_cfg.h: UART_TP_RX_RING_BYTES must be a power of two"
#endif

#define UART_TP_RX_MASK                 (UART_TP_RX_RING_BYTES - 1U)



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Receive ring, filled by a circular DMA that never stops.
 *
 * IN_DMA_BUF places it where a bus master can reach it without cache
 * maintenance. On a part with a D-cache and no such section this would need a
 * clean or an invalidate around every access, which for a ring the DMA is
 * writing continuously is not a thing that can be done correctly.
 */
static U8 aRxRing[UART_TP_RX_RING_BYTES] IN_DMA_BUF;

/** Where the reader has got to. The DMA owns the write position implicitly. */
static U16 nRxRead;

/** Transmit queue. Copied into so the caller's buffer is free on return. */
static U8  aTxQueue[UART_TP_TX_QUEUE_BYTES] IN_DMA_BUF;
static U16 nTxHead;                 /**< next free slot                        */
static U16 nTxTail;                 /**< next byte to send                     */
static U16 nTxInFlight;             /**< bytes the DMA is currently sending    */

static UART_TP_STATS tStats;

static volatile BOOLEAN bInitDone = FALSE;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Ring position the DMA is about to write.
 *
 * NDTR counts DOWN from the transfer size, so the position is the size minus
 * what is left. Reading it is the only way to know where a circular DMA has
 * got to - there is no interrupt per byte, and asking for one at 921600 baud
 * would be 92 000 interrupts a second.
 */
static U16 UART_TP_RxWritePos(void)
{
    const U32 nRemaining = __HAL_DMA_GET_COUNTER(UART_TP_HANDLE.hdmarx);

    return (U16)(((U32)UART_TP_RX_RING_BYTES - nRemaining) & (U32)UART_TP_RX_MASK);
}


/**
 * @brief Hand the next queued run to the DMA, if it is idle.
 *
 * Sends up to the end of the queue in one transfer rather than wrapping, so the
 * DMA always sees a contiguous run. The remainder goes on the next call, which
 * is why this is called from the completion callback as well as from Send.
 */
static void UART_TP_TxKick(void)
{
    U16 nRun;

    if ((nTxInFlight > 0U) || (nTxHead == nTxTail))
    {
        return;
    }

    nRun = (nTxHead > nTxTail) ? (U16)(nTxHead - nTxTail)
                               : (U16)(UART_TP_TX_QUEUE_BYTES - nTxTail);

    if (HAL_UART_Transmit_DMA(&UART_TP_HANDLE, &aTxQueue[nTxTail], nRun) == HAL_OK)
    {
        nTxInFlight = nRun;
    }
    else
    {
        /* Counted, not retried here: retrying inside a callback would recurse.
           UART_TP_Poll picks it up. */
        tStats.nUartErrors++;
    }
}


/** @brief Free space in the transmit queue. One slot is always left empty so
    that head == tail means empty rather than being ambiguous with full. */
static U16 UART_TP_TxFree(void)
{
    if (nTxHead >= nTxTail)
    {
        return (U16)(UART_TP_TX_QUEUE_BYTES - 1U - (nTxHead - nTxTail));
    }

    return (U16)(nTxTail - nTxHead - 1U);
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT UART_TP_Init(void)
{
    nRxRead     = 0U;
    nTxHead     = 0U;
    nTxTail     = 0U;
    nTxInFlight = 0U;

    (void)memset(&tStats, 0, sizeof(tStats));
    (void)memset(aRxRing, 0, sizeof(aRxRing));

    /*
     * The configured baud rate WINS over whatever CubeMX generated.
     *
     * The baud rate is a property of the link that both ends must agree on, so
     * it belongs in one configuration header rather than in a generated file
     * that the next regeneration silently reverts.
     */
    UART_TP_HANDLE.Init.BaudRate = UART_TP_BAUDRATE;

    if (HAL_UART_Init(&UART_TP_HANDLE) != HAL_OK)
    {
        return RESULT_NOT_OK;
    }

    /*
     * Circular receive, armed once and never re-armed.
     *
     * The alternative - a fixed-length receive restarted from the completion
     * callback - has a window between transfers in which arriving bytes are
     * lost. At this baud rate that window is hit routinely, and the symptom is
     * a protocol layer that resynchronises every few frames for no visible
     * reason.
     */
    if (HAL_UART_Receive_DMA(&UART_TP_HANDLE, aRxRing,
                             (U16)UART_TP_RX_RING_BYTES) != HAL_OK)
    {
        return RESULT_NOT_OK;
    }

    /*
     * Silence the half-transfer interrupt. Nothing uses it - the reader takes
     * whatever has arrived whenever it asks - and at this baud rate it is
     * thousands of pointless interrupts a second.
     */
    __HAL_DMA_DISABLE_IT(UART_TP_HANDLE.hdmarx, DMA_IT_HT);

    bInitDone = TRUE;

    return RESULT_OK;
}


STD_RESULT UART_TP_DeInit(void)
{
    bInitDone = FALSE;

    (void)HAL_UART_DMAStop(&UART_TP_HANDLE);

    if (HAL_UART_DeInit(&UART_TP_HANDLE) != HAL_OK)
    {
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}


STD_RESULT UART_TP_Send(const U8* const pData, const U16 nLength)
{
    U16 i;

    if ((pData == NULL_PTR) || (nLength == 0U))
    {
        return RESULT_INVALID_PARAM_1;
    }

    if (bInitDone == FALSE)
    {
        return RESULT_NOT_INIT;
    }

    /*
     * Refused WHOLE, never partially.
     *
     * A protocol layer can do nothing useful with half a frame on the wire, and
     * the far end would spend the next frame resynchronising on the remains. So
     * a send that will not fit is dropped and counted, and the caller can
     * decide whether to retry.
     */
    if (nLength > UART_TP_TxFree())
    {
        tStats.nTxDropped++;
        return RESULT_BUSY;
    }

    for (i = 0U; i < nLength; i++)
    {
        aTxQueue[nTxHead] = pData[i];
        nTxHead = (U16)((nTxHead + 1U) % (U16)UART_TP_TX_QUEUE_BYTES);
    }

    tStats.nTxBytes += (U32)nLength;

    UART_TP_TxKick();

    return RESULT_OK;
}


U16 UART_TP_Read(U8* const pBuf, const U16 nMaxLength, BOOLEAN* const pnOverrun)
{
    U16 nWrite;
    U16 nAvail;
    U16 nQty;
    U16 i;

    if (pnOverrun != NULL_PTR)
    {
        *pnOverrun = FALSE;
    }

    if ((pBuf == NULL_PTR) || (nMaxLength == 0U) || (bInitDone == FALSE))
    {
        return 0U;
    }

    nWrite = UART_TP_RxWritePos();
    nAvail = (U16)((nWrite - nRxRead) & (U16)UART_TP_RX_MASK);

    if (nAvail == 0U)
    {
        return 0U;
    }

    /*
     * OVERRUN DETECTION, AND WHY IT IS APPROXIMATE.
     *
     * A circular DMA gives no signal when it laps the reader; the write
     * position simply passes the read position and keeps going. What CAN be
     * seen is the ring being nearly full, which for a reader that is keeping up
     * never happens - so treating "almost full" as a lap is the only warning
     * available, and it is better than none.
     *
     * Reported rather than hidden: a protocol layer that knows bytes went
     * missing can resynchronise deliberately, where one that finds out from a
     * bad CRC has already mixed good and bad data.
     */
    if (nAvail > (U16)(UART_TP_RX_RING_BYTES - (UART_TP_RX_RING_BYTES / 8U)))
    {
        tStats.nRxOverruns++;

        if (pnOverrun != NULL_PTR)
        {
            *pnOverrun = TRUE;
        }
    }

    nQty = (nAvail > nMaxLength) ? nMaxLength : nAvail;

    for (i = 0U; i < nQty; i++)
    {
        pBuf[i] = aRxRing[nRxRead];
        nRxRead = (U16)((nRxRead + 1U) & (U16)UART_TP_RX_MASK);
    }

    tStats.nRxBytes += (U32)nQty;

    return nQty;
}


U16 UART_TP_RxPending(void)
{
    if (bInitDone == FALSE)
    {
        return 0U;
    }

    return (U16)((UART_TP_RxWritePos() - nRxRead) & (U16)UART_TP_RX_MASK);
}


BOOLEAN UART_TP_IsTxBusy(void)
{
    return (nTxInFlight > 0U) ? TRUE : FALSE;
}


void UART_TP_GetStats(UART_TP_STATS* const pStats)
{
    if (pStats != NULL_PTR)
    {
        *pStats = tStats;
    }
}


void UART_TP_Poll(void)
{
    if (bInitDone == TRUE)
    {
        /* Restarts a transmission that could not be chained from the callback,
           e.g. because the peripheral was momentarily busy. */
        UART_TP_TxKick();
    }
}



/***************************************************************************************************
* HAL callbacks
*
* These are the weak HAL symbols, so this module OWNS them for the configured
* peripheral. A project that needs another UART's callbacks must add a handle
* check of its own - which is why every one of these tests the handle first
* rather than assuming it is the only user.
***************************************************************************************************/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart != &UART_TP_HANDLE)
    {
        return;
    }

    nTxTail     = (U16)((nTxTail + nTxInFlight) % (U16)UART_TP_TX_QUEUE_BYTES);
    nTxInFlight = 0U;

    /* Chain straight into the next run: at 921600 baud, waiting for a poll
       between frames would halve the usable throughput. */
    UART_TP_TxKick();
}


void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart != &UART_TP_HANDLE)
    {
        return;
    }

    tStats.nUartErrors++;

    /*
     * A receiver error - framing, noise, parity, overrun - aborts the DMA on
     * some HAL versions and leaves it running on others. Re-arming
     * unconditionally is wrong (it would restart a healthy transfer and lose
     * the ring position), so only restart when the HAL says it stopped.
     *
     * The read position is reset with it: the ring contents are no longer a
     * continuous stream, and carrying on from the old position would splice
     * pre-error and post-error bytes into one frame.
     */
    if (UART_TP_HANDLE.RxState == HAL_UART_STATE_READY)
    {
        nRxRead = 0U;

        if (HAL_UART_Receive_DMA(&UART_TP_HANDLE, aRxRing,
                                 (U16)UART_TP_RX_RING_BYTES) == HAL_OK)
        {
            __HAL_DMA_DISABLE_IT(UART_TP_HANDLE.hdmarx, DMA_IT_HT);
        }
    }
}

#endif  // #if (UART_TP_IN_USE == ON)

/****************************************** end of file *******************************************/
