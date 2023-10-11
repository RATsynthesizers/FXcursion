/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    quadspi.h
  * @brief   This file contains all the function prototypes for
  *          the quadspi.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#ifndef __QUADSPI_H__
#define __QUADSPI_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern QSPI_HandleTypeDef hqspi;

/* USER CODE BEGIN Private defines */
//////////////////////////////////////////////////////////////////////////////////
#include "n25q128a.h"
#define QUADSPI_EXTERNAL_LOADER_NAME "MY_N25Q128A_STM32H743VI-PedalBreadBoard_V0"
#define MEMORY_FLASH_SIZE N25Q128A_FLASH_SIZE
#define MEMORY_PAGE_SIZE N25Q128A_PAGE_SIZE
#define MEMORY_SECTOR_SIZE N25Q128A_SECTOR_SIZE

uint8_t CSP_QUADSPI_Init(void);
uint8_t CSP_QSPI_EraseSector(uint32_t EraseStartAddress, uint32_t EraseEndAddress);
uint8_t CSP_QSPI_WriteMemory(uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
uint8_t CSP_QSPI_ReadMemory(uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
uint8_t CSP_QSPI_Erase_Block(uint32_t BlockAddress);
uint8_t CSP_QSPI_EnableMemoryMappedMode(void);
uint8_t CSP_QSPI_Erase_Chip (void);
/////////////////////////////////////////////////////////////////////////////////
/* USER CODE END Private defines */

void MX_QUADSPI_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __QUADSPI_H__ */

