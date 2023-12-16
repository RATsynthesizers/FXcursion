/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#define SPI2_FromAudio 1
#define SPI1_optionalDisplay 1
#define SPI5_UI 1
#define I2C2_oled 1
#define I2C4_Codec 1
#define MY_UI_RESET PC15
#define I2C4_EEPROM 1
#define MY_LCD_RESET PC6
#define MY_SD_DETECT PA10
#define UART4_debug 1
#define USER_UI_RESET_Pin GPIO_PIN_13
#define USER_UI_RESET_GPIO_Port GPIOC
#define USER_GUI_PB2_Pin GPIO_PIN_2
#define USER_GUI_PB2_GPIO_Port GPIOB
#define USER_LCD_RESET_Pin GPIO_PIN_6
#define USER_LCD_RESET_GPIO_Port GPIOC
#define SD_DETECT_Pin GPIO_PIN_10
#define SD_DETECT_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
