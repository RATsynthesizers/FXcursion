/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2s.c
  * @brief   This file provides code for the configuration
  *          of the I2S instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "i2s.h"

/* USER CODE BEGIN 0 */

#include "irq_cfg.h"

/*
 * The headphone converter had no DMA stream at all as generated, so the monitor
 * output had no data path. DMA2, because DMA1 is full: streams 0-7 are taken by
 * the four SAI streams, the debug UART and the recorder link.
 */
DMA_HandleTypeDef hdma_spi3_tx;

/* USER CODE END 0 */

I2S_HandleTypeDef hi2s3;

/* I2S3 init function */
void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  /* ==========================================================================
   * I2S3 - codec 3, the HEADPHONE MONITOR. Output only.
   *
   * Codec 3's ADC inputs are permanently unused for now; the board does route
   * I2S3_SDI, so full duplex remains possible later if an aux return or
   * re-amp loop is ever wanted. PC11 (I2S3_SDI) can be freed meanwhile.
   *
   * ---- SLAVE, clocked from SAI1 -------------------------------------------
   *
   * This used to be a second MASTER, generating its own MCLK, CK and WS from
   * the SPI123 kernel. It worked because that kernel was the same 24.576 MHz
   * crystal SAI1 uses, so the two were frequency-locked and only the frame
   * PHASE differed - which HpWrite absorbs by writing half an elastic ring
   * ahead of wherever the DMA actually is.
   *
   * WHY IT MOVED. I2S3 is SPI3, and D2CCIP1R.SPI123SEL selects the kernel for
   * SPI1, SPI2 and SPI3 TOGETHER. So a second master here pinned SPI1 to the
   * 24.576 MHz pin clock, and with the H7's minimum /2 prescaler that capped
   * the board-to-board link at 12.288 Mbit/s - half of which the recorder
   * stream alone consumes, leaving no room for loop transport.
   *
   * As a slave it needs no kernel of its own, which frees SPI123 to run from a
   * PLL. See spi_tp_cfg.h.
   *
   * WHAT THE BOARD MUST PROVIDE. CK and WS are now INPUTS, driven by SAI1
   * block A - the same MCLK/SCK/FS that already clock codec 1, which has no
   * clock pins of its own either:
   *
   *     SAI1_SCK_A  PE5  ->  I2S3_CK  PC10   and the codec
   *     SAI1_FS_A   PE4  ->  I2S3_WS  PA15   and the codec
   *     SAI1_MCLK_A PE2  ->  codec MCLK only - a slave I2S cannot emit MCLK,
   *                          and does not need one
   *     PC7 (was I2S3_MCK) is now free.
   *
   * The formats already agree: SAI1 is I2S standard, 24-bit in 2 slots, so
   * BCLK is 64 x Fs = 3.072 MHz, and I2S_DATAFORMAT_24B frames identically.
   *
   * The elastic ring in HpWrite is kept. It costs nothing and it no longer has
   * to work: with one clock domain the phase is fixed by construction rather
   * than merely stable.
   * ======================================================================== */
  hi2s3.Init.Mode = I2S_MODE_SLAVE_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_24B;
  /* A slave cannot drive MCLK - the codec takes SAI1_MCLK_A directly. */
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  /* Ignored in slave mode: the rate arrives on CK. Left at the real value so
     the number is not misleading to a reader. */
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.FirstBit = I2S_FIRSTBIT_MSB;
  hi2s3.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
  hi2s3.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
  hi2s3.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

}

void HAL_I2S_MspInit(I2S_HandleTypeDef* i2sHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2sHandle->Instance==SPI3)
  {
  /* USER CODE BEGIN SPI3_MspInit 0 */

  /* USER CODE END SPI3_MspInit 0 */
    /* I2S3 clock enable */
    __HAL_RCC_SPI3_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**I2S3 GPIO Configuration
    PC7     ------> I2S3_MCK
    PA15 (JTDI)     ------> I2S3_WS
    PC10     ------> I2S3_CK
    PC11     ------> I2S3_SDI
    PC12     ------> I2S3_SDO
    */
    /* PC7 (I2S3_MCK) is NOT configured any more: a slave emits no MCLK, and
       leaving it as an alternate function would drive a clock into a net that
       is now fed by SAI1_MCLK_A. It is free for other use.

       PC10 (CK) and PC11/PC12 (data) stay alternate-function. CK is an INPUT in
       slave mode; the alternate function is still correct - the peripheral
       reads the pin rather than driving it. */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    /* CK and WS arrive at 3.072 MHz from SAI1. LOW speed is a slew-rate limit
       on OUTPUTS and does not affect these inputs, but SDO is an output at the
       same rate, so the setting is left as generated. */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral DMA init*/

    /* Headphone monitor, transmit only. WORD transfers: the I2S carries 24-bit
     * samples right aligned in 32-bit slots, exactly like the SAI. */
    hdma_spi3_tx.Instance = DMA2_Stream0;
    hdma_spi3_tx.Init.Request = DMA_REQUEST_SPI3_TX;
    hdma_spi3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi3_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_spi3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_spi3_tx.Init.Mode = DMA_CIRCULAR;
    hdma_spi3_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_spi3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi3_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(i2sHandle,hdmatx,hdma_spi3_tx);

  /* USER CODE BEGIN SPI3_MspInit 1 */

  /* USER CODE END SPI3_MspInit 1 */
  }
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef* i2sHandle)
{

  if(i2sHandle->Instance==SPI3)
  {
  /* USER CODE BEGIN SPI3_MspDeInit 0 */

  /* USER CODE END SPI3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();

    /**I2S3 GPIO Configuration
    PC7     ------> I2S3_MCK
    PA15 (JTDI)     ------> I2S3_WS
    PC10     ------> I2S3_CK
    PC11     ------> I2S3_SDI
    PC12     ------> I2S3_SDO
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_7|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_15);

  /* USER CODE BEGIN SPI3_MspDeInit 1 */

  /* USER CODE END SPI3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
