/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : mdma.c
  * Description        : This file provides code for the configuration
  *                      of all the requested global MDMA transfers.
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
#include "mdma.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure MDMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

#include "irq_cfg.h"

/* USER CODE END 1 */
MDMA_HandleTypeDef hmdma_quadspi;

/**
  * Enable MDMA controller clock
  * Configure MDMA for global transfers
  *   hmdma_mdma_channel0_sw_0
  */
void MX_MDMA_Init(void)
{

  /* MDMA controller clock enable */
  __HAL_RCC_MDMA_CLK_ENABLE();
  /* Local variables */

  /*
   * Channel 0 carries the QUADSPI, which means it carries loop audio.
   *
   * CubeMX generated this channel as a software-triggered halfword mover with a
   * 24-byte buffer length - a placeholder that nothing referenced. It is now
   * configured for what this board actually needs.
   *
   * BufferTransferLength matches the QUADSPI FIFO threshold set in
   * MX_QUADSPI_Init: one MDMA burst per FIFO trigger, no more.
   *
   * BYTE data size on both ends. HAL_QSPI_Receive_DMA and HAL_QSPI_Transmit_DMA
   * derive the increment from these fields, and loop windows split on 3-byte
   * sample boundaries, so neither the address nor the length can be assumed
   * word aligned.
   */
  hmdma_quadspi.Instance = MDMA_Channel0;
  hmdma_quadspi.Init.Request = MDMA_REQUEST_QUADSPI_FIFO_TH;
  hmdma_quadspi.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;
  hmdma_quadspi.Init.Priority = MDMA_PRIORITY_HIGH;
  hmdma_quadspi.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  hmdma_quadspi.Init.SourceInc = MDMA_SRC_INC_BYTE;
  hmdma_quadspi.Init.DestinationInc = MDMA_DEST_INC_BYTE;
  hmdma_quadspi.Init.SourceDataSize = MDMA_SRC_DATASIZE_BYTE;
  hmdma_quadspi.Init.DestDataSize = MDMA_DEST_DATASIZE_BYTE;
  hmdma_quadspi.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  hmdma_quadspi.Init.BufferTransferLength = 4;
  hmdma_quadspi.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
  hmdma_quadspi.Init.DestBurst = MDMA_DEST_BURST_SINGLE;
  hmdma_quadspi.Init.SourceBlockAddressOffset = 0;
  hmdma_quadspi.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_quadspi) != HAL_OK)
  {
    Error_Handler();
  }

  /* MDMA interrupt initialization */
  /* MDMA_IRQn interrupt configuration */
  /* Below the audio block interrupt on purpose - see irq_cfg.h. */
  HAL_NVIC_SetPriority(MDMA_IRQn, IRQ_PRIO_LOOP_MDMA, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(MDMA_IRQn);

}
/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

/**
  * @}
  */

/**
  * @}
  */

