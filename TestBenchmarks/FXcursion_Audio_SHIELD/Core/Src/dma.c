/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   This file provides code for the configuration
  *          of all the requested memory to memory DMA transfers.
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
#include "dma.h"

/* USER CODE BEGIN 0 */
#include "irq_cfg.h"
/* USER CODE END 0 */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /*
   * DMA interrupt init
   *
   * Priorities come from Config/irq_cfg.h rather than being 0 for everything as
   * generated. With no RTOS and a 1333 us deadline, "everything is equally
   * urgent" means the debug UART can delay the audio block.
   *
   * DMA1 stream map:
   *   0  SAI1_A RX   the audio block interrupt - the only one that does work
   *   1  SAI1_B TX
   *   2  USART2 RX   debug
   *   3  USART2 TX   debug
   *   4  SPI1 RX     recorder samples
   *   5  SPI1 TX     recorder samples
   *   6  SAI2_A RX   added: channels 2 and 3 in
   *   7  SAI2_B TX   added: channels 2 and 3 out
   *
   * DMA2 stream map:
   *   0  SPI3 TX     added: the headphone monitor
   *   1  USART1 RX   added: control link in, circular
   *   2  USART1 TX   added: control link out
   */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, IRQ_PRIO_AUDIO, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, IRQ_PRIO_AUDIO_STREAM, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* DMA1_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, IRQ_PRIO_DEBUG, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, IRQ_PRIO_DEBUG, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, IRQ_PRIO_RECORDER, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, IRQ_PRIO_RECORDER, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, IRQ_PRIO_AUDIO_STREAM, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA1_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, IRQ_PRIO_AUDIO_STREAM, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, IRQ_PRIO_AUDIO_STREAM, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, IRQ_PRIO_CTRL_LINK, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, IRQ_PRIO_CTRL_LINK, IRQ_SUBPRIO);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */

