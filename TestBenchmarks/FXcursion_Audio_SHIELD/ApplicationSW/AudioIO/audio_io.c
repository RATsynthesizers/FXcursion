/**
 * @file      audio_io.c
 *
 * @details   Audio timebase implementation. See audio_io.h for the model.
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

#include "audio_io.h"

#include "rec_spi.h"

#include "chan_map.h"
#include "hp_bus.h"

#include "mem_map.h"
#include "audio_sys.h"

#include "main.h"
#include "sai.h"
#include "i2s.h"



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * DMA buffers.
 *
 * RAM_D2, not DTCM and not AXI SRAM. DMA1 and DMA2 live in domain D2 and CANNOT
 * reach the tightly coupled memories at all - a buffer in DTCM would simply
 * never transfer - and reaching AXI SRAM costs a bus bridge that D2 SRAM does
 * not. The MPU maps this region non-cacheable, so no maintenance is needed
 * around a transfer. See mem_map.h.
 */
static S32 aSai1Rx[AIO_MAIN_WORDS] IN_DMA_BUF MEM_ALIGN(32);
static S32 aSai1Tx[AIO_MAIN_WORDS] IN_DMA_BUF MEM_ALIGN(32);
static S32 aSai2Rx[AIO_MAIN_WORDS] IN_DMA_BUF MEM_ALIGN(32);
static S32 aSai2Tx[AIO_MAIN_WORDS] IN_DMA_BUF MEM_ALIGN(32);
static S32 aHpTx  [AIO_HP_WORDS]   IN_DMA_BUF MEM_ALIGN(32);

/*
 * The engine's block, four channels interleaved. DTCM: the CPU touches every
 * word of it twice per block and nothing else does.
 */
static S32 aBlockIn [AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY] IN_DTCM MEM_ALIGN(32);
static S32 aBlockOut[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY] IN_DTCM MEM_ALIGN(32);

static U32 nBudgetCycles;
static U32 nBlocks;
static U32 nPhaseFaults;
static U32 nStreamErrors;

static BOOLEAN bRunning;
static volatile BOOLEAN bRestartRequest;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Which half of a two-half circular buffer a DMA stream is filling.
 *
 * NDTR counts DOWN from the full item count, so more than half remaining means
 * it is still working through the first half.
 */
static U8 HalfInFlight(const DMA_HandleTypeDef* const pDma)
{
    U8 nHalf = 0U;

    if (pDma != NULL_PTR)
    {
        nHalf = (__HAL_DMA_GET_COUNTER(pDma) > (U32)AIO_BLOCK_WORDS) ? 0U : 1U;
    }

    return nHalf;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Write the monitor mix into the headphone ring, ahead of the DMA.
 *
 * The headphone converter has its own frame sync, so its DMA is at an arbitrary
 * but fixed phase relative to this interrupt. Writing "the other half" the way
 * the main streams do would therefore be either always right or always wrong.
 *
 * Instead the slot being read is measured and the mix is written half a ring
 * ahead of it. That leaves a block of margin on either side whatever the phase
 * turns out to be.
 */
static void HpWrite(const S32* const pBlock, const U16 nFrames)
{
    if (hi2s3.hdmatx != NULL_PTR)
    {
        const U32 nRemain = __HAL_DMA_GET_COUNTER(hi2s3.hdmatx);
        U32       nBusy   = ((U32)AIO_HP_WORDS - nRemain) / (U32)AIO_BLOCK_WORDS;
        U32       nWrite;

        if (nBusy >= (U32)AIO_HP_BUF_SLOTS)
        {
            nBusy = (U32)AIO_HP_BUF_SLOTS - 1UL;    // NDTR reads 0 mid-reload
        }

        nWrite = (nBusy + (U32)AIO_HP_WRITE_LEAD) % (U32)AIO_HP_BUF_SLOTS;

        HpBus_Process(pBlock, &aHpTx[nWrite * (U32)AIO_BLOCK_WORDS], nFrames);
    }
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief One audio block. This is the whole real-time system.
 */
static void ProcessHalf(const U8 nHalf)
{
    const U32 nOff   = (U32)nHalf * (U32)AIO_BLOCK_WORDS;
    const U32 nStart = DWT->CYCCNT;
    U32       nSpent;

    ChanMap_Gather(&aSai1Rx[nOff], &aSai2Rx[nOff], aBlockIn, (U16)AUDIO_BLOCK_FRAMES);

    AudioSys_ProcessBlock(aBlockIn, aBlockOut, (U16)AUDIO_BLOCK_FRAMES);

    /* After the chain, because the recorder's post-effect taps are written
       during Grid_Process - and inside the load measurement below, because the
       staging copy is real work spent in the block budget. */
    RecSpi_PushBlock();

    ChanMap_Scatter(aBlockOut, &aSai1Tx[nOff], &aSai2Tx[nOff], (U16)AUDIO_BLOCK_FRAMES);

    HpWrite(aBlockOut, (U16)AUDIO_BLOCK_FRAMES);

    /*
     * SAI2 shares SAI1's frame sync, so its DMA must be in the same half. If it
     * ever is not, channels 2 and 3 are being read from the wrong place and no
     * amount of DSP downstream will fix it - see audio_io.h on start order.
     */
    if (HalfInFlight(hsai_BlockA1.hdmarx) != HalfInFlight(hsai_BlockA2.hdmarx))
    {
        nPhaseFaults++;
    }

    nSpent = DWT->CYCCNT - nStart;              // wraps correctly, unsigned
    AudioSys_ReportLoad(nSpent, nBudgetCycles);

    if (nSpent > nBudgetCycles)
    {
        AudioSys_NotifyOverrun();
    }

    nBlocks++;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Flush denormal floats to zero in hardware.
 *
 * A reverb tail decays smoothly towards silence, so it spends its last second
 * in the range where denormals live - and a denormal operation can cost ten to
 * a hundred times a normal one. A tail that gets progressively more expensive
 * as it fades is the kind of overrun that only shows up after the player stops
 * playing.
 *
 * FPSCR.FZ is per-core and set once. It costs nothing and there is no audio in
 * this system that wants denormal precision.
 */
static void DenormalsOff(void)
{
    __set_FPSCR(__get_FPSCR() | (1UL << 24U));
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Enable the cycle counter used for the load figure.
 */
static void CycleCounterStart(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT       = 0UL;
    DWT->CTRL        |= DWT_CTRL_CYCCNTENA_Msk;
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT AudioIO_Init(void)
{
    // Silence first. Whatever RAM_D2 held at reset must not reach a converter,
    // and the transmit buffers are read by DMA from the first frame onward.
    ChanMap_Silence(aSai1Tx, (U32)AIO_MAIN_WORDS);
    ChanMap_Silence(aSai2Tx, (U32)AIO_MAIN_WORDS);
    ChanMap_Silence(aHpTx,   (U32)AIO_HP_WORDS);
    ChanMap_Silence(aSai1Rx, (U32)AIO_MAIN_WORDS);
    ChanMap_Silence(aSai2Rx, (U32)AIO_MAIN_WORDS);

    ChanMap_Silence(aBlockIn,  (U32)AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY);
    ChanMap_Silence(aBlockOut, (U32)AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY);

    (void)HpBus_Init();

    /* The recorder stream stays OFF until the interface asks for it - see
       fx_protocol.h on why it cannot just start. This only clears the staging
       buffers, which .dtcm being NOLOAD makes mandatory. */
    (void)RecSpi_Init();

    DenormalsOff();
    CycleCounterStart();

    // DWT counts CPU cycles, so this is the core clock and not HCLK.
    nBudgetCycles   = (SystemCoreClock / 1000000UL) * (U32)AUDIO_BLOCK_PERIOD_US;

    nBlocks         = 0UL;
    nPhaseFaults    = 0UL;
    nStreamErrors   = 0UL;
    bRunning        = FALSE;
    bRestartRequest = FALSE;

    return RESULT_OK;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT AudioIO_Start(void)
{
    STD_RESULT eResult = RESULT_OK;

    if (bRunning != FALSE)
    {
        return RESULT_OK;
    }

    /* THE MONITOR MUST BE ARMED FIRST, and this is no longer a free choice.
     *
     * I2S3 is a SLAVE now - its CK and WS come from SAI1 block A. A slave I2S
     * latches its frame alignment from the WS edge it sees when it is enabled,
     * so it has to be waiting BEFORE any clock exists. Start it after the
     * master and it joins mid-frame: left and right swap, and it stays swapped
     * until the next restart.
     *
     * That is why the master is started last, below. The order reads the same
     * as it did when this was an independent master and the ordering did not
     * matter; it matters now.
     *
     * The HAL takes the item count as U16 and the buffer as uint16_t*; both are
     * legacy signatures - the transfer width comes from the DMA configuration,
     * which is 32-bit for our 24-bit samples. */
    if (HAL_I2S_Transmit_DMA(&hi2s3, (const uint16_t*)aHpTx, (uint16_t)AIO_HP_WORDS) != HAL_OK)
    {
        eResult = RESULT_NOT_OK;
    }

    /* Now the main streams, dependents first. See audio_io.h - this order is
     * the difference between four aligned channels and an intermittent swap. */
    if ((eResult == RESULT_OK) &&
        (HAL_SAI_Transmit_DMA(&hsai_BlockB2, (uint8_t*)aSai2Tx, (uint16_t)AIO_MAIN_WORDS) != HAL_OK))
    {
        eResult = RESULT_NOT_OK;
    }

    if ((eResult == RESULT_OK) &&
        (HAL_SAI_Receive_DMA(&hsai_BlockA2, (uint8_t*)aSai2Rx, (uint16_t)AIO_MAIN_WORDS) != HAL_OK))
    {
        eResult = RESULT_NOT_OK;
    }

    if ((eResult == RESULT_OK) &&
        (HAL_SAI_Transmit_DMA(&hsai_BlockB1, (uint8_t*)aSai1Tx, (uint16_t)AIO_MAIN_WORDS) != HAL_OK))
    {
        eResult = RESULT_NOT_OK;
    }

    /* The master, and therefore the clock, last of all. */
    if ((eResult == RESULT_OK) &&
        (HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t*)aSai1Rx, (uint16_t)AIO_MAIN_WORDS) != HAL_OK))
    {
        eResult = RESULT_NOT_OK;
    }

    if (eResult == RESULT_OK)
    {
        bRunning = TRUE;
    }
    else
    {
        // Never leave half the streams running: a partially started SAI group
        // produces plausible-looking audio on some channels and silence on
        // others, which is far harder to diagnose than nothing at all.
        (void)AudioIO_Stop();
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT AudioIO_Stop(void)
{
    STD_RESULT eResult = RESULT_OK;

    // Master first: with the clock stopped the synchronous blocks stop moving
    // data, so the rest come down quietly.
    if (HAL_SAI_DMAStop(&hsai_BlockA1) != HAL_OK) { eResult = RESULT_NOT_OK; }
    if (HAL_SAI_DMAStop(&hsai_BlockB1) != HAL_OK) { eResult = RESULT_NOT_OK; }
    if (HAL_SAI_DMAStop(&hsai_BlockA2) != HAL_OK) { eResult = RESULT_NOT_OK; }
    if (HAL_SAI_DMAStop(&hsai_BlockB2) != HAL_OK) { eResult = RESULT_NOT_OK; }
    if (HAL_I2S_DMAStop(&hi2s3)        != HAL_OK) { eResult = RESULT_NOT_OK; }

    bRunning = FALSE;

    return eResult;
}

//--------------------------------------------------------------------------------------------------

void AudioIO_Service(void)
{
    if (bRestartRequest != FALSE)
    {
        bRestartRequest = FALSE;

        (void)AudioIO_Stop();
        (void)AudioIO_Start();
    }
}

//--------------------------------------------------------------------------------------------------

BOOLEAN AudioIO_IsRunning(void)
{
    return bRunning;
}

//--------------------------------------------------------------------------------------------------

U32 AudioIO_BlockCount(void)
{
    return nBlocks;
}

//--------------------------------------------------------------------------------------------------

U32 AudioIO_PhaseFaults(void)
{
    return nPhaseFaults;
}

//--------------------------------------------------------------------------------------------------

U32 AudioIO_StreamErrors(void)
{
    return nStreamErrors;
}



/***************************************************************************************************
* HAL callbacks
***************************************************************************************************/

/*
 * Only SAI1 block A does any work. The other four streams free-run in circular
 * mode and their callbacks exist solely because the HAL requires the interrupt
 * to be serviced - see audio_io.h.
 */

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef* hsai)
{
    if (hsai == &hsai_BlockA1)
    {
        ProcessHalf(0U);
    }
}

//--------------------------------------------------------------------------------------------------

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef* hsai)
{
    if (hsai == &hsai_BlockA1)
    {
        ProcessHalf(1U);
    }
}

//--------------------------------------------------------------------------------------------------

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef* hsai)
{
    UNUSED(hsai);
}

//--------------------------------------------------------------------------------------------------

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef* hsai)
{
    UNUSED(hsai);
}

//--------------------------------------------------------------------------------------------------

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef* hsai)
{
    UNUSED(hsai);

    nStreamErrors++;

    // Recovery is five DMA restarts and is not interrupt work. Ask the super
    // loop to do it.
    bRestartRequest = TRUE;
}

//--------------------------------------------------------------------------------------------------

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
}

//--------------------------------------------------------------------------------------------------

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);
}

//--------------------------------------------------------------------------------------------------

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef* hi2s)
{
    UNUSED(hi2s);

    // The monitor failing is not a reason to interrupt the main outputs, so
    // this is counted and left alone.
    nStreamErrors++;
}

/****************************************** end of file *******************************************/
