/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file   fatfs.c
 * @brief  Code for fatfs applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
#include "fatfs.h"
#include "MemRegions.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
IN_RAM_D1_DMA FATFS SDFatFS;    /* File system object for SD logical drive */
IN_RAM_D1_DMA FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */
IN_RAM_D1_DMA uint8_t workBuffer[_MAX_SS];
/* USER CODE END Variables */

void MX_FATFS_Init(void)
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
	/* additional user code for init */

//	retSD = f_mkfs((TCHAR const*) SDPath, FM_FAT, 0, workBuffer,
//			sizeof(workBuffer));
//	if (retSD != FR_OK)
//		Error_Handler();
//	retSD = f_mount(&SDFatFS, (TCHAR const*) SDPath, 0);
//	if (retSD != FR_OK)
//		Error_Handler();
	__NOP();
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
	return 0;
  /* USER CODE END get_fattime */
}

/* USER CODE BEGIN Application */

/* USER CODE END Application */
