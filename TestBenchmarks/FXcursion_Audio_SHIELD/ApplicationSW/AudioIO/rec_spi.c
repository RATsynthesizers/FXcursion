/**
 * @file      rec_spi.c
 *
 * @details   Recorder stream transport. See rec_spi.h for the model and the
 *            bandwidth arithmetic.
 *
 *            Excluded from the host build: this is the DMA and interrupt
 *            plumbing, and everything worth testing was deliberately put in
 *            rec_stream.c instead.
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

#include "rec_spi.h"

#include "recorder.h"

/* The SPI peripheral, its clock and its callbacks belong to the transport now.
   spi.h is no longer needed here: nothing in this file touches hspi1. main.h
   still is - it is what pulls in the CMSIS core intrinsics for __get_PRIMASK,
   which spi.h used to provide transitively. */
#include "main.h"
#include "spi_tp.h"

/* The loop transport rides in the same frame as the recorder stream: it says
   how wide the frame is this block, and fills the slots past REC_SLOT_QTY. */
#include "loop_xfer.h"

/* FX_FRAME_SLOT_QTY, FX_FRAME_LOOP_SLOT_BASE - the frame is fixed and shared,
 * not negotiated per transfer. */
#include "fx_frame.h"



/***************************************************************************************************
* Declarations of local (private) functions
***************************************************************************************************/

/* Defined below, registered with the transport in RecSpi_Init above them. */
static void RecSpi_OnSent(void);
static void RecSpi_OnError(void);



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Hand one staged half to the transport.
 *
 * SPI1 is configured SPI_DATASIZE_32BIT, so the transport counts 32-bit data
 * frames, not bytes - the word count from the staging layer goes straight in.
 *
 * No cache maintenance: the buffer is in RAM_D2, which MPU region 0 maps
 * non-cacheable precisely so that the audio DMA buffers never need it. See
 * mem_map.h.
 *
 * WHAT MOVED. This used to call HAL_SPI_Transmit_DMA on hspi1 directly and own
 * HAL_SPI_TxCpltCallback. Both now belong to SPI_TP, which also applies the
 * configured bit clock - so the link speed lives in spi_tp_cfg.h rather than in
 * whatever CubeMX last generated.
 */
static void StartTx(const U8 nHalf)
{
    const S32* const pBuf   = RecStream_Buffer(nHalf);
    const U16        nWords = RecStream_Words(nHalf);

    if ((pBuf == NULL_PTR) || (nWords == 0U))
    {
        RecStream_Error();
        return;
    }

    if (SPI_TP_SendFrame(pBuf, nWords) != RESULT_OK)
    {
        /* The state machine believes a transfer is running. Tell it otherwise,
           or it waits for a completion that will never arrive and the stream
           stops for good.
           A RESULT_BUSY here means the previous frame had not finished, which
           SPI_TP counts as a dropped frame - the number that says out loud that
           the frame no longer fits the block at this clock. */
        RecStream_Error();
    }
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT RecSpi_Init(void)
{
    STD_RESULT eResult = RecStream_Init();

    if (eResult == RESULT_OK)
    {
        /* Applies the configured bit clock and takes the peripheral. */
        eResult = SPI_TP_Init();
    }

    if (eResult == RESULT_OK)
    {
        eResult = SPI_TP_RegisterSentCb(&RecSpi_OnSent);
    }

    if (eResult == RESULT_OK)
    {
        eResult = SPI_TP_RegisterErrorCb(&RecSpi_OnError);
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

void RecSpi_PushBlock(void)
{
    U16        nFrames = 0U;
    const S32* pSrc;
    U8         nStart;
    U8         nSlots;
    U32        nPrimask;

    if (RecStream_IsEnabled() == FALSE)
    {
        return;
    }

    pSrc = Recorder_GetStream(&nFrames);

    /*
     * THE FRAME NEVER WIDENS. It is FX_FRAME_SLOT_QTY slots, always.
     *
     * This used to be LoopXfer_StreamWidth() - REC_SLOT_QTY when idle, wider
     * while a session ran - and both ends had to change width on precisely the
     * same frame. They cannot disagree about a constant.
     */
    nSlots = (U8)FX_FRAME_SLOT_QTY;

    nPrimask = __get_PRIMASK();
    __disable_irq();

    nStart = RecStream_Stage(pSrc, nFrames, nSlots);

    /*
     * Fill the loop slots of the half just staged, before it is handed to the
     * transport. Inside the critical section with the staging: until StartTx
     * runs, the completion callback could otherwise hand this half out again.
     *
     * Called unconditionally now. LoopXfer_Block writes nothing when no session
     * is running, and RecStream_Stage has already zeroed those slots - so an
     * idle link sends 27 slots of zeros rather than narrowing the frame.
     */
    if (nStart != (U8)REC_STAGE_NONE)
    {
        S32* const pStage = RecStream_StageSlots(nStart);

        if (pStage != NULL_PTR)
        {
            (void)LoopXfer_Block(&pStage[FX_FRAME_LOOP_SLOT_BASE],
                                 (U32)nFrames, nSlots);
        }
    }

    __set_PRIMASK(nPrimask);

    /* Outside the critical section: the HAL call is long, and by this point the
       half is committed to us - the completion callback cannot hand it out
       again, so nothing else can touch it. */
    if (nStart != (U8)REC_STAGE_NONE)
    {
        StartTx(nStart);
    }
}

//--------------------------------------------------------------------------------------------------

BOOLEAN RecSpi_IsStreaming(void)
{
    return RecStream_IsEnabled();
}

//--------------------------------------------------------------------------------------------------

const REC_STREAM_STATS* RecSpi_Stats(void)
{
    return RecStream_Stats();
}



/***************************************************************************************************
* HAL callbacks
***************************************************************************************************/

/**
 * @brief One frame has gone out. Registered with SPI_TP at init.
 *
 * Was HAL_SPI_TxCpltCallback with a handle check. SPI_TP owns that weak symbol
 * now - it has to, because it also owns the error callback and the receive side
 * on the other board - and hands the event on through this.
 */
static void RecSpi_OnSent(void)
{
    U8  nNext;
    U32 nPrimask = __get_PRIMASK();

    __disable_irq();

    nNext = RecStream_Complete();

    __set_PRIMASK(nPrimask);

    if (nNext != (U8)REC_STAGE_NONE)
    {
        StartTx(nNext);
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief A transfer failed. Registered with SPI_TP at init.
 *
 * Was HAL_SPI_ErrorCallback with a handle check; SPI_TP owns that weak symbol
 * now and calls this.
 */
static void RecSpi_OnError(void)
{
    /*
     * An underrun or a bus error means the interface has just received an
     * unknown number of words, so its de-interleave is now of unknown phase.
     * There is no way to resynchronise by content - the stream carries no
     * framing - so the honest thing is to stop and let the interface notice
     * nStreamErrors and re-arm.
     *
     * RecStream_Error clears the machine; staging stays enabled, so the next
     * block starts a fresh transfer on a block boundary. The interface will
     * have a discontinuity in the recording either way, and a discontinuity it
     * can see beats a rotation it cannot.
     */
    RecStream_Error();
}

/****************************************** end of file *******************************************/
