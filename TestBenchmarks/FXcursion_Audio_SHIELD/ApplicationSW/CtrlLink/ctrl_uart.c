/**
 * @file      ctrl_uart.c
 *
 * @details   Control link transport. See ctrl_uart.h for the model.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "ctrl_uart.h"

#include "ctrl_link.h"

#include "mem_map.h"
#include "audio_sys.h"
#include "loop_mem.h"
#include "audio_io.h"
#include "hp_bus.h"
#include "rec_spi.h"

#include "main.h"
#include "usart.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/** Warn above this much of the receive ring consumed between two polls. */
#define CTRL_RX_WARN_BYTES              ((CTRL_RX_RING_BYTES * 3U) / 4U)

/* The transmit ring must hold two maximum frames, or an ACK generated while a
 * telemetry frame is in flight has nowhere to go - which is the exact case the
 * ring exists for. */
FXC_STATIC_ASSERT(CTRL_TX_RING_BYTES >= (2U * PROTO_FRAME_MAX), ctrl_tx_ring_holds_two_frames);



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * RAM_D2. DMA1 and DMA2 cannot address DTCM, so neither the receive buffer nor
 * the transmit ring can live with the rest of this module's state.
 */
static U8 aRxDma[CTRL_RX_RING_BYTES]  IN_DMA_BUF MEM_ALIGN(32);
static U8 aTxRing[CTRL_TX_RING_BYTES] IN_DMA_BUF MEM_ALIGN(32);

/** How far the super-loop has read into the circular receive buffer. */
static U16 nRxRead IN_DTCM;

static volatile U16 nTxHead     IN_DTCM;    /**< next byte the super-loop writes */
static volatile U16 nTxTail     IN_DTCM;    /**< next byte the DMA will send     */
static volatile U16 nTxInFlight IN_DTCM;    /**< length of the running transfer  */

static U32 nTxDropped  IN_DTCM;
static U32 nRxNearFull IN_DTCM;
static U32 nUartErrors IN_DTCM;

static U32     nLastTelemetryMs IN_DTCM;
static U8      nTelemetryCount  IN_DTCM;
static BOOLEAN bLoopMemHealthy  IN_DTCM;
static BOOLEAN bBound           IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Start the next contiguous run of the transmit ring, if idle.
 *
 * Reached from both the super-loop and the transmit-complete interrupt, so the
 * check and the start have to be one operation. The critical section is a
 * handful of register writes - well under a microsecond of a 1333 us audio
 * block - and it is what makes the two callers safe rather than nearly safe.
 */
static void PumpTx(void)
{
    const U32 nPrimask = __get_PRIMASK();

    __disable_irq();

    if ((nTxInFlight == 0U) && (nTxHead != nTxTail))
    {
        /* Up to the end of the buffer, or up to the head - the DMA takes a
         * pointer and a length, so a wrapped run goes out as two transfers. */
        const U16 nRun = (nTxHead > nTxTail) ? (U16)(nTxHead - nTxTail)
                                             : (U16)(CTRL_TX_RING_BYTES - nTxTail);

        if (HAL_UART_Transmit_DMA(&huart1, &aTxRing[nTxTail], nRun) == HAL_OK)
        {
            nTxInFlight = nRun;
        }
    }

    __set_PRIMASK(nPrimask);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief The CTRL_TX_FN handed to ctrl_link.
 *
 * Copies rather than borrows: the frame it is given lives in DTCM, which the
 * DMA cannot reach. See ctrl_uart.h.
 */
static STD_RESULT TxQueue(const U8* const pData, const U16 nLength)
{
    STD_RESULT eResult = RESULT_OK;
    const U16  nUsed   = (U16)((nTxHead - nTxTail) % CTRL_TX_RING_BYTES);
    const U16  nFree   = (U16)(CTRL_TX_RING_BYTES - 1U - nUsed);

    if (pData == NULL_PTR)
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else if (nLength > nFree)
    {
        // Dropped whole, never truncated: half a frame on the wire would cost
        // the receiver a resynchronisation on top of the lost message.
        nTxDropped++;
        eResult = RESULT_NOT_OK;
    }
    else
    {
        U16 i;
        U16 nHead = nTxHead;

        for (i = 0U; i < nLength; i++)
        {
            aTxRing[nHead] = pData[i];
            nHead = (U16)((nHead + 1U) % CTRL_TX_RING_BYTES);
        }

        // Published only once the bytes are all in place, so the interrupt can
        // never see a partially written frame.
        nTxHead = nHead;

        PumpTx();
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Feed everything the DMA has received since the last call to the parser.
 */
static void DrainRx(void)
{
    U16 nWrite;
    U16 nAvail;

    if (huart1.hdmarx == NULL_PTR)
    {
        return;
    }

    /* NDTR counts down, so this is how many bytes the DMA has written in total,
     * modulo the ring. Sampled once: anything that arrives during the loop
     * below is simply picked up on the next call. */
    nWrite = (U16)(CTRL_RX_RING_BYTES - __HAL_DMA_GET_COUNTER(huart1.hdmarx));

    if (nWrite >= (U16)CTRL_RX_RING_BYTES)
    {
        nWrite = 0U;                                // NDTR reads 0 mid-reload
    }

    nAvail = (U16)((nWrite - nRxRead) % CTRL_RX_RING_BYTES);

    if (nAvail > (U16)CTRL_RX_WARN_BYTES)
    {
        // Not yet a loss, but the super-loop is close to being lapped by a link
        // running at 11.5 KB/s. Worth knowing before it becomes corruption.
        nRxNearFull++;
    }

    while (nRxRead != nWrite)
    {
        CtrlLink_RxByte(aRxDma[nRxRead]);
        nRxRead = (U16)((nRxRead + 1U) % CTRL_RX_RING_BYTES);
    }

    (void)CtrlLink_Poll();
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Gather the bring-up counters and send them.
 */
static void SendDiag(void)
{
    const CTRL_STATS* const pStats = CtrlLink_Stats();
    PROTO_DIAG              tDiag;

    tDiag.nBlocks        = AudioIO_BlockCount();
    tDiag.nFramesOk      = pStats->nFramesOk;
    tDiag.nCrcErrors     = pStats->nCrcErrors;
    tDiag.nResyncs       = pStats->nResyncs;
    tDiag.nRxOverflows   = pStats->nRxOverflows;
    tDiag.nHpClips       = HpBus_ClipCount();

    tDiag.nRecBlocksSent = RecSpi_Stats()->nBlocksSent;
    tDiag.nRecDropped    = RecSpi_Stats()->nBlocksDropped;
    tDiag.nRecErrors     = RecSpi_Stats()->nErrors;

    /* Saturating, not wrapping. A counter that rolls over to a small number
     * reads as healthy, which is the one thing a fault counter must never do. */
    tDiag.nPhaseFaults   = (U16)((AudioIO_PhaseFaults()   > 65535UL) ? 65535UL : AudioIO_PhaseFaults());
    tDiag.nStreamErrors  = (U16)((AudioIO_StreamErrors()  > 65535UL) ? 65535UL : AudioIO_StreamErrors());
    tDiag.nLoopUnderruns = (U16)((LoopMem_Underruns()   > 65535UL) ? 65535UL : LoopMem_Underruns());
    tDiag.nLoopErrors    = (U16)((LoopMem_Errors()      > 65535UL) ? 65535UL : LoopMem_Errors());

    tDiag.nVersion       = (U8)PROTO_VERSION;
    tDiag.bLoopMemOk     = (U8)((bLoopMemHealthy != FALSE) ? 1U : 0U);
    tDiag.aReserved[0]   = 0U;
    tDiag.aReserved[1]   = 0U;

    (void)CtrlLink_SendFrame((U8)PROTO_CMD_DIAG, (const U8*)&tDiag, (U8)sizeof(tDiag));
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT CtrlUart_Init(const BOOLEAN bLoopMemOk)
{
    STD_RESULT eResult;

    nRxRead          = 0U;
    nTxHead          = 0U;
    nTxTail          = 0U;
    nTxInFlight      = 0U;
    nTxDropped       = 0UL;
    nRxNearFull      = 0UL;
    nUartErrors      = 0UL;
    nTelemetryCount  = 0U;
    nLastTelemetryMs = HAL_GetTick();
    bLoopMemHealthy  = bLoopMemOk;
    bBound           = FALSE;

    eResult = CtrlLink_Init(&TxQueue);

    if (eResult == RESULT_OK)
    {
        /* Circular, and never stopped again. The read pointer chases NDTR. */
        if (HAL_UART_Receive_DMA(&huart1, aRxDma, (uint16_t)CTRL_RX_RING_BYTES) != HAL_OK)
        {
            eResult = RESULT_NOT_OK;
        }
        else
        {
            bBound = TRUE;
        }
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

void CtrlUart_Service(void)
{
    U32 nNow;

    if (bBound == FALSE)
    {
        return;
    }

    DrainRx();

    nNow = HAL_GetTick();

    /* Unsigned subtraction, so this stays correct across the tick wrap. */
    if ((nNow - nLastTelemetryMs) >= (U32)TELEMETRY_PERIOD_MS)
    {
        nLastTelemetryMs = nNow;

        (void)CtrlLink_SendTelemetry();

        nTelemetryCount++;
        if (nTelemetryCount >= (U8)TELEMETRY_DIAG_EVERY)
        {
            nTelemetryCount = 0U;
            SendDiag();
        }
    }

    // In case a transfer finished while the ring was empty and something has
    // been queued since without the interrupt having anything to start.
    PumpTx();
}

//--------------------------------------------------------------------------------------------------

BOOLEAN CtrlUart_IsLinked(void)
{
    return (CtrlLink_Stats()->nFramesOk > 0UL) ? TRUE : FALSE;
}

//--------------------------------------------------------------------------------------------------

U32 CtrlUart_TxDropped(void)
{
    return nTxDropped;
}

//--------------------------------------------------------------------------------------------------

U32 CtrlUart_RxNearFull(void)
{
    return nRxNearFull;
}

//--------------------------------------------------------------------------------------------------

U32 CtrlUart_UartErrors(void)
{
    return nUartErrors;
}



/***************************************************************************************************
* HAL callbacks
***************************************************************************************************/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart == &huart1)
    {
        nTxTail     = (U16)((nTxTail + nTxInFlight) % CTRL_TX_RING_BYTES);
        nTxInFlight = 0U;

        PumpTx();
    }
}

//--------------------------------------------------------------------------------------------------

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    if (huart == &huart1)
    {
        nUartErrors++;

        /*
         * An overrun or framing error aborts the receive DMA, and a control link
         * that has silently stopped listening is worse than one that glitches.
         * Restart it and resynchronise the read pointer with the fresh buffer.
         */
        nRxRead = 0U;
        (void)HAL_UART_Receive_DMA(&huart1, aRxDma, (uint16_t)CTRL_RX_RING_BYTES);
    }
}

/****************************************** end of file *******************************************/
