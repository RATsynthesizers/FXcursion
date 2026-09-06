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
/* The UART, its baud rate and its callbacks belong to the transport; nothing
   in this file names a peripheral any more. */
#include "uart_tp.h"

/* A loop transfer ends in an audio block and is reported from here. */
#include "loop_xfer.h"



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
 * The buffers and ring indices that used to be here belong to UART_TP, along
 * with their RAM_D2 placement - DMA1 and DMA2 cannot address DTCM, which is why
 * they could never live with the rest of this module's state. The transport
 * takes that placement from UART_TP_DMA_SECTION in uart_tp_cfg.h.
 *
 * What is left here is the PROTOCOL side: telemetry cadence, diagnostics, and
 * the health flags reported in PROTO_DIAG.
 */

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
/**
 * @brief The CTRL_TX_FN handed to ctrl_link.
 *
 * Copies rather than borrows: the frame it is given lives in DTCM, which the
 * DMA cannot reach. UART_TP_Send does that copy. See ctrl_uart.h.
 *
 * WHAT MOVED. This file used to carry its own transmit ring, its own PumpTx
 * with a critical section around the test-and-start, its own circular receive
 * DMA and both HAL UART callbacks - all of it duplicated, near enough line for
 * line, in the interface controller. It is UART_TP in SystemSW now, which is
 * where a thing both firmwares need belongs.
 *
 * The refusal semantics are unchanged: dropped whole, never truncated, because
 * half a frame on the wire costs the receiver a resynchronisation on top of the
 * lost message. UART_TP counts the drop in its own statistics; nTxDropped is
 * kept here so PROTO_DIAG keeps reporting the same number it always did.
 */
static STD_RESULT TxQueue(const U8* const pData, const U16 nLength)
{
    STD_RESULT eResult;

    if (pData == NULL_PTR)
    {
        return RESULT_INVALID_PARAM_0;
    }

    eResult = UART_TP_Send(pData, nLength);

    if (eResult != RESULT_OK)
    {
        nTxDropped++;
        eResult = RESULT_NOT_OK;
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Feed everything the DMA has received since the last call to the parser.
 */
static void DrainRx(void)
{
    U8      aChunk[64];
    BOOLEAN bOverrun = FALSE;
    U16     nQty;

    /* Not yet a loss, but the super-loop is close to being lapped. Worth
     * knowing before it becomes corruption - the same warning this used to
     * compute from NDTR, now asked of the transport. */
    if (UART_TP_RxPending() > (U16)CTRL_RX_WARN_BYTES)
    {
        nRxNearFull++;
    }

    /* Drain in chunks until the transport has nothing left. The ring walk and
       the NDTR arithmetic that used to be here are UART_TP's now. */
    do
    {
        U16 i;

        nQty = UART_TP_Read(aChunk, (U16)sizeof(aChunk), &bOverrun);

        /*
         * Bytes were lost BEFORE these ones. The parser is holding the front of
         * a frame whose middle is missing, and the bytes now arriving look like
         * a continuation of it - so tell it to give up on that frame rather
         * than let it splice two together and rely on the CRC noticing.
         */
        if (bOverrun == TRUE)
        {
            CtrlLink_Resync();
        }

        for (i = 0U; i < nQty; i++)
        {
            CtrlLink_RxByte(aChunk[i]);
        }

    } while (nQty == (U16)sizeof(aChunk));

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

    /* The ring indices that used to be reset here are UART_TP's, cleared in
       UART_TP_Init below. */
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
        /* Takes the peripheral, applies UART_TP_BAUDRATE and arms the circular
           receive - which is never stopped again.

           WHICH UART IS NO LONGER NAMED HERE. It moved from USART1 to USART2 so
           that both controllers use the same peripheral for the control link;
           that choice now lives in uart_tp_cfg.h, and USART1 is free for a
           per-board debug console. */
        if (UART_TP_Init() != RESULT_OK)
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

    /*
     * A loop transfer that ended in an audio block. The block could not send a
     * UART frame, so it latched the result and this collects it - once, which
     * is why LoopXfer_TakeCompletion clears the latch rather than exposing a
     * state to poll.
     *
     * This frame is how the CRC crosses. The interface checksums what arrived;
     * comparing it needs the value this side computed over what it sent, and
     * without this the comparison has nothing to compare against.
     */
    {
        PROTO_LOOP_STAT tStat;

        if (LoopXfer_TakeCompletion(&tStat) == TRUE)
        {
            (void)CtrlLink_SendFrame((U8)PROTO_CMD_LOOP_STAT,
                                     (const U8*)&tStat, (U8)sizeof(tStat));
        }
    }

    // In case a transfer finished while the queue was empty and something has
    // been queued since without the interrupt having anything to start.
    UART_TP_Poll();
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
    /* From the transport, which owns the error callback now. Kept as an
       accessor so PROTO_DIAG reports the same field it always did. */
    UART_TP_STATS tTp;

    UART_TP_GetStats(&tTp);

    return nUartErrors + tTp.nUartErrors;
}



/***************************************************************************************************
* HAL callbacks
***************************************************************************************************/

/*
 * The two HAL UART callbacks that used to live here - transmit-complete and
 * error - belong to UART_TP now, along with the restart-after-error that kept a
 * control link from silently going deaf.
 *
 * The one behaviour that improved in the move: the restart used to reset the
 * read pointer and say nothing, so the parser carried on mid-frame into a fresh
 * buffer. UART_TP reports it instead, DrainRx sees the overrun flag and calls
 * CtrlLink_Resync, and the frame is abandoned deliberately.
 *
 * nUartErrors is still reported in PROTO_DIAG - CtrlUart_Errors reads it from
 * the transport's own counters.
 */

/****************************************** end of file *******************************************/
