/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : mdma.c
  * Description        : This file provides code for the configuration
  *                      of all the requested global MDMA transfers.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* USER CODE BEGIN Includes */
/* For the recorder stream geometry: REC_RX_WORDS, RECORD_BUF_SAMPLES,
   REC_RX_HALF_FRAMES. */
#include "common_cfg.h"
/* USER CODE END Includes */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure MDMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */
extern int32_t audioRxBuffer[REC_RX_WORDS];

/* Planar de-interleaved audio - one ring per channel. The node initialisers
   below are only ever placeholders: MDMA_Trigger_Deinterleave rewrites CSAR,
   CDAR, CBNDTR and CBRUR on every transfer, so these values need to be valid
   rather than right. */
extern int32_t recorder[REC_SLOTS_PER_FRAME][RECORD_BUF_SAMPLES];

/* USER CODE END 1 */
MDMA_HandleTypeDef hmdma_mdma_channel0_sw_0;
MDMA_HandleTypeDef hmdma_mdma_channel1_sw_0;
/* USER CODE BEGIN MDMA_Nodes */
/*
 * .dma_buffers - RAM_D2, NON-CACHEABLE via MPU region 0. See MPU_Config in
 * Init.c.
 *
 * These are not data buffers, they are DESCRIPTORS, and the MDMA engine
 * fetches them from RAM itself. The CPU rewrites CSAR/CDAR/CBNDTR/CBRUR on
 * every block in Recorder.c, so with the D-cache on and these in cacheable
 * RAM_D1 the engine would read whatever RAM held before those writes - a
 * de-interleave pointing at the wrong addresses, which records real audio
 * into the wrong files and reports nothing.
 *
 * NOLOAD is safe: MX_MDMA_Init fills every field below with
 * HAL_MDMA_LinkedList_CreateNode, so no flash initialiser is relied on.
 * 32-byte aligned for the cache line, which also satisfies the 64-bit
 * alignment the MDMA requires of a node.
 *
 * WARNING: this file is CubeMX generated. Regenerating from the .ioc drops
 * this attribute and reintroduces the fault - which is why Recorder.c also
 * cleans the nodes explicitly before firing.
 */
MDMA_LinkNodeTypeDef node_mdma_channel1_sw_1 __attribute__((section(".dma_buffers"), aligned(32)));
MDMA_LinkNodeTypeDef node_mdma_channel1_sw_2 __attribute__((section(".dma_buffers"), aligned(32)));
MDMA_LinkNodeTypeDef node_mdma_channel1_sw_3 __attribute__((section(".dma_buffers"), aligned(32)));

/*
 * A FOURTH NODE, for the loop transport.
 *
 * The de-interleave needs one route per destination. Four recorder planes use
 * the channel itself plus nodes 1..3, which is the whole of what CubeMX
 * generates - so a loop transfer, whose slots are contiguous on the wire and
 * therefore ONE route, had nowhere to go.
 *
 * Declared here rather than in MX_MDMA_Init because this block is inside the
 * USER CODE section and survives regeneration; MX_MDMA_Init does not. Nothing
 * initialises it here either: Recorder.c copies node 3's static configuration
 * into it once at start-up and rewrites the dynamic registers every block, the
 * same as it already does for the other three. That keeps the whole
 * arrangement independent of what CubeMX decides to emit.
 */
MDMA_LinkNodeTypeDef node_mdma_channel1_sw_4 __attribute__((section(".dma_buffers"), aligned(32)));
/* USER CODE END MDMA_Nodes */

/**
  * Enable MDMA controller clock
  * Configure MDMA for global transfers
  *   hmdma_mdma_channel0_sw_0
  *   hmdma_mdma_channel1_sw_0
  *   node_mdma_channel1_sw_1
  *   node_mdma_channel1_sw_2
  *   node_mdma_channel1_sw_3
  */
void MX_MDMA_Init(void)
{

  /* MDMA controller clock enable */
  __HAL_RCC_MDMA_CLK_ENABLE();
  /* Local variables */
  MDMA_LinkNodeConfTypeDef nodeConfig;

  /* Configure MDMA channel MDMA_Channel0 */
  /* Configure MDMA request hmdma_mdma_channel0_sw_0 on MDMA_Channel0 */
  hmdma_mdma_channel0_sw_0.Instance = MDMA_Channel0;
  hmdma_mdma_channel0_sw_0.Init.Request = MDMA_REQUEST_SW;
  hmdma_mdma_channel0_sw_0.Init.TransferTriggerMode = MDMA_FULL_TRANSFER;
  hmdma_mdma_channel0_sw_0.Init.Priority = MDMA_PRIORITY_HIGH;
  hmdma_mdma_channel0_sw_0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  hmdma_mdma_channel0_sw_0.Init.SourceInc = MDMA_SRC_INC_WORD;
  hmdma_mdma_channel0_sw_0.Init.DestinationInc = MDMA_DEST_INC_DISABLE;
  hmdma_mdma_channel0_sw_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
  hmdma_mdma_channel0_sw_0.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  hmdma_mdma_channel0_sw_0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  /* USER CODE BEGIN MDMA0_TLEN */
  /* 128, not 320*2. This field is 7 bits and the HAL asserts 1..254, but
     USE_FULL_ASSERT is off so 640 was written unmasked and overflowed into the
     bits above TLEN. flushFrameBuffer rewrites this per rect anyway - see
     mdmaBufferLength in TouchGFXHAL.cpp - so this is only the boot value. */
  hmdma_mdma_channel0_sw_0.Init.BufferTransferLength = 128;
  /* USER CODE END MDMA0_TLEN */
  hmdma_mdma_channel0_sw_0.Init.SourceBurst = MDMA_SOURCE_BURST_128BEATS;
  hmdma_mdma_channel0_sw_0.Init.DestBurst = MDMA_DEST_BURST_128BEATS;
  hmdma_mdma_channel0_sw_0.Init.SourceBlockAddressOffset = 0;
  hmdma_mdma_channel0_sw_0.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_mdma_channel0_sw_0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure MDMA channel MDMA_Channel1 */
  /* Configure MDMA request hmdma_mdma_channel1_sw_0 on MDMA_Channel1 */
  hmdma_mdma_channel1_sw_0.Instance = MDMA_Channel1;
  hmdma_mdma_channel1_sw_0.Init.Request = MDMA_REQUEST_SW;
  hmdma_mdma_channel1_sw_0.Init.TransferTriggerMode = MDMA_FULL_TRANSFER;
  hmdma_mdma_channel1_sw_0.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
  hmdma_mdma_channel1_sw_0.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  hmdma_mdma_channel1_sw_0.Init.SourceInc = MDMA_SRC_INC_WORD;
  hmdma_mdma_channel1_sw_0.Init.DestinationInc = MDMA_DEST_INC_WORD;
  hmdma_mdma_channel1_sw_0.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
  hmdma_mdma_channel1_sw_0.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  hmdma_mdma_channel1_sw_0.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  hmdma_mdma_channel1_sw_0.Init.BufferTransferLength = 4;
  hmdma_mdma_channel1_sw_0.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
  hmdma_mdma_channel1_sw_0.Init.DestBurst = MDMA_DEST_BURST_SINGLE;
  hmdma_mdma_channel1_sw_0.Init.SourceBlockAddressOffset = 12;
  hmdma_mdma_channel1_sw_0.Init.DestBlockAddressOffset = 0;
  if (HAL_MDMA_Init(&hmdma_mdma_channel1_sw_0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Initialize MDMA link node according to specified parameters */
  nodeConfig.Init.Request = MDMA_REQUEST_SW;
  nodeConfig.Init.TransferTriggerMode = MDMA_FULL_TRANSFER;
  nodeConfig.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
  nodeConfig.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  nodeConfig.Init.SourceInc = MDMA_SRC_INC_WORD;
  nodeConfig.Init.DestinationInc = MDMA_DEST_INC_WORD;
  nodeConfig.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
  nodeConfig.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  nodeConfig.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  nodeConfig.Init.BufferTransferLength = 4;
  nodeConfig.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
  nodeConfig.Init.DestBurst = MDMA_DEST_BURST_SINGLE;
  nodeConfig.Init.SourceBlockAddressOffset = 12;
  nodeConfig.Init.DestBlockAddressOffset = 0;
  nodeConfig.PostRequestMaskAddress = 0;
  nodeConfig.PostRequestMaskData = 0;
  nodeConfig.SrcAddress = (uint32_t)  &audioRxBuffer[1];
  nodeConfig.DstAddress = (uint32_t)  &recorder[1][0];
  nodeConfig.BlockDataLength = 4;
  nodeConfig.BlockCount = REC_RX_HALF_FRAMES;
  if (HAL_MDMA_LinkedList_CreateNode(&node_mdma_channel1_sw_1, &nodeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN mdma_channel1_sw_1 */

  /* USER CODE END mdma_channel1_sw_1 */

  /* Connect a node to the linked list */
  if (HAL_MDMA_LinkedList_AddNode(&hmdma_mdma_channel1_sw_0, &node_mdma_channel1_sw_1, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Initialize MDMA link node according to specified parameters */
  nodeConfig.Init.Request = MDMA_REQUEST_SW;
  nodeConfig.Init.TransferTriggerMode = MDMA_FULL_TRANSFER;
  nodeConfig.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
  nodeConfig.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  nodeConfig.Init.SourceInc = MDMA_SRC_INC_WORD;
  nodeConfig.Init.DestinationInc = MDMA_DEST_INC_WORD;
  nodeConfig.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
  nodeConfig.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  nodeConfig.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  nodeConfig.Init.BufferTransferLength = 4;
  nodeConfig.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
  nodeConfig.Init.DestBurst = MDMA_DEST_BURST_SINGLE;
  nodeConfig.Init.SourceBlockAddressOffset = 12;
  nodeConfig.Init.DestBlockAddressOffset = 0;
  nodeConfig.PostRequestMaskAddress = 0;
  nodeConfig.PostRequestMaskData = 0;
  nodeConfig.SrcAddress = (uint32_t)  &audioRxBuffer[2];
  nodeConfig.DstAddress = (uint32_t)  &recorder[2][0];
  nodeConfig.BlockDataLength = 4;
  nodeConfig.BlockCount = REC_RX_HALF_FRAMES;
  if (HAL_MDMA_LinkedList_CreateNode(&node_mdma_channel1_sw_2, &nodeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN mdma_channel1_sw_2 */

  /* USER CODE END mdma_channel1_sw_2 */

  /* Connect a node to the linked list */
  if (HAL_MDMA_LinkedList_AddNode(&hmdma_mdma_channel1_sw_0, &node_mdma_channel1_sw_2, 0) != HAL_OK)
  {
    Error_Handler();
  }

  /* Initialize MDMA link node according to specified parameters */
  nodeConfig.Init.Request = MDMA_REQUEST_SW;
  nodeConfig.Init.TransferTriggerMode = MDMA_FULL_TRANSFER;
  nodeConfig.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
  nodeConfig.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
  nodeConfig.Init.SourceInc = MDMA_SRC_INC_WORD;
  nodeConfig.Init.DestinationInc = MDMA_DEST_INC_WORD;
  nodeConfig.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
  nodeConfig.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
  nodeConfig.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
  nodeConfig.Init.BufferTransferLength = 4;
  nodeConfig.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
  nodeConfig.Init.DestBurst = MDMA_DEST_BURST_SINGLE;
  nodeConfig.Init.SourceBlockAddressOffset = 12;
  nodeConfig.Init.DestBlockAddressOffset = 0;
  nodeConfig.PostRequestMaskAddress = 0;
  nodeConfig.PostRequestMaskData = 0;
  nodeConfig.SrcAddress = (uint32_t)  &audioRxBuffer[3];
  nodeConfig.DstAddress = (uint32_t)  &recorder[3][0];
  nodeConfig.BlockDataLength = 4;
  nodeConfig.BlockCount = REC_RX_HALF_FRAMES;
  if (HAL_MDMA_LinkedList_CreateNode(&node_mdma_channel1_sw_3, &nodeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN mdma_channel1_sw_3 */

  /* USER CODE END mdma_channel1_sw_3 */

  /* MDMA interrupt initialization */
  /* MDMA_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(MDMA_IRQn, 5, 0);
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

