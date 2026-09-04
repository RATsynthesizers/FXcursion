/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MPU_D3_NONCACHABLE_0x38__ 1
#define FAFA 0xFAFA
/* What each peripheral is for. These used to be written the other way round -
 * "#define SAI1 Codec_0" - which shadowed the CMSIS peripheral macros and broke
 * the build in i2c.c, spi.c, quadspi.c and usart.c. Role on the left, hardware
 * on the right. */
#define CODEC_0_SAI SAI1            /* channels 0,1 - and the audio timebase   */
#define CODEC_1_SAI SAI2            /* channels 2,3 - data slave to SAI1       */
#define CODEC_HP_I2S SPI3           /* headphone monitor, TX only              */
#define CODEC_CTRL_I2C I2C1         /* all three codecs, SCL gated per group   */
#define SAMPLES_SPI SPI1            /* 24-bit sample stream to the interface   */
#define FILTER_CTRL_SPI SPI2        /* analogue filter control                 */
#define PSRAM_QSPI QUADSPI          /* loop audio store                        */
#define CTRL_UART USART1            /* control link to the interface           */
#define DEBUG_UART USART2           /* debug                                   */
#define USER_CODEC_GPIO_Pin GPIO_PIN_13
#define USER_CODEC_GPIO_GPIO_Port GPIOC
#define USER_DBG2_Pin GPIO_PIN_11
#define USER_DBG2_GPIO_Port GPIOA
#define USER_DBG1_Pin GPIO_PIN_12
#define USER_DBG1_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
