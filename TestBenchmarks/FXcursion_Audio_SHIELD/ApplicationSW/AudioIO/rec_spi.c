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

#include "spi.h"



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Hand one staged half to the SPI DMA.
 *
 * SPI1 is configured SPI_DATASIZE_32BIT, so HAL_SPI_Transmit_DMA counts 32-bit
 * data frames, not bytes - the word count from the staging layer goes straight
 * in.
 *
 * No cache maintenance: the buffer is in RAM_D2, which MPU region 0 maps
 * non-cacheable precisely so that the audio DMA buffers never need it. See
 * mem_map.h.
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

    /* Cast away const for the HAL, which takes a non-const pointer even for a
       transmit. Nothing writes through it. */
    if (HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)(void*)pBuf, nWords) != HAL_OK)
    {
        /* The state machine believes a transfer is running. Tell it otherwise,
           or it waits for a completion interrupt that will never arrive and the
           stream stops for good. */
        RecStream_Error();
    }
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT RecSpi_Init(void)
{
    return RecStream_Init();
}

//--------------------------------------------------------------------------------------------------

void RecSpi_PushBlock(void)
{
    U16        nFrames = 0U;
    const S32* pSrc;
    U8         nStart;
    U32        nPrimask;

    if (RecStream_IsEnabled() == FALSE)
    {
        return;
    }

    pSrc = Recorder_GetStream(&nFrames);

    nPrimask = __get_PRIMASK();
    __disable_irq();

    nStart = RecStream_Stage(pSrc, nFrames);

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

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    if (hspi == &hspi1)
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
}

//--------------------------------------------------------------------------------------------------

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef* hspi)
{
    if (hspi == &hspi1)
    {
        /*
         * An underrun or a bus error means the interface has just received an
         * unknown number of words, so its de-interleave is now of unknown
         * phase. There is no way to resynchronise by content - the stream
         * carries no framing - so the honest thing is to stop and let the
         * interface notice nStreamErrors and re-arm.
         *
         * RecStream_Error clears the machine; staging stays enabled, so the
         * next block starts a fresh transfer on a block boundary. The interface
         * will have a discontinuity in the recording either way, and a
         * discontinuity it can see beats a rotation it cannot.
         */
        RecStream_Error();
    }
}

/****************************************** end of file *******************************************/
