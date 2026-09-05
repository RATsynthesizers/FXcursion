/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : mdma.h
  * Description        : This file contains all the function prototypes for
  *                      the mdma.c file
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __mdma_H
#define __mdma_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* MDMA transfer handles -----------------------------------------------------*/
extern MDMA_HandleTypeDef hmdma_mdma_channel0_sw_0;
extern MDMA_HandleTypeDef hmdma_mdma_channel1_sw_0;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_1;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_2;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_3;

/* Routes past what CubeMX emits - the loop transport, the live looper run, and
   headroom. See the note over their definitions in mdma.c for why they live in
   the USER CODE block and are configured from Recorder.c. The chain length is
   whatever the route table asks for; these are a budget, not a limit. */
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_4;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_5;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_6;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_7;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_8;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_9;
extern MDMA_LinkNodeTypeDef node_mdma_channel1_sw_10;

/* USER CODE BEGIN Includes */
#include "common_cfg.h"
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_MDMA_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __mdma_H */

/**
  * @}
  */
