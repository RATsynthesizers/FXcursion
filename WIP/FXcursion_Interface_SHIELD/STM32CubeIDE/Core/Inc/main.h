/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#define I2C4_EEPROM 1
#define MY_SD_DETECT PA10
#define I2C2_oled 1
#define MY_LCD_RESET PC6
#define MY_UI_RESET PC15
#define SPI1_optionalDisplay 1
#define SPI2_FromAudio 1
#define SPI5_UI 1
#define UART4_debug 1
#define ENC_MENU_A_Pin GPIO_PIN_3
#define ENC_MENU_A_GPIO_Port GPIOE
#define ENC_MENU_B_Pin GPIO_PIN_4
#define ENC_MENU_B_GPIO_Port GPIOE
#define BTN_REC_Pin GPIO_PIN_13
#define BTN_REC_GPIO_Port GPIOC
#define ADC_VUSB_Pin GPIO_PIN_1
#define ADC_VUSB_GPIO_Port GPIOC
#define ADC_VBAT_Pin GPIO_PIN_3
#define ADC_VBAT_GPIO_Port GPIOA
#define ADC3_Pin GPIO_PIN_4
#define ADC3_GPIO_Port GPIOA
#define ADC4_Pin GPIO_PIN_5
#define ADC4_GPIO_Port GPIOA
#define ADC_VOL_Pin GPIO_PIN_6
#define ADC_VOL_GPIO_Port GPIOA
#define ADC5_Pin GPIO_PIN_4
#define ADC5_GPIO_Port GPIOC
#define ADC6_Pin GPIO_PIN_5
#define ADC6_GPIO_Port GPIOC
#define RGB_LEDS_Pin GPIO_PIN_1
#define RGB_LEDS_GPIO_Port GPIOB
#define MY_LCD_TE_EXTI2_Pin GPIO_PIN_2
#define MY_LCD_TE_EXTI2_GPIO_Port GPIOB
#define MY_LCD_TE_EXTI2_EXTI_IRQn EXTI2_IRQn
#define ENC_PARAM_B_Pin GPIO_PIN_2
#define ENC_PARAM_B_GPIO_Port GPIOG
#define ENC_PARAM_A_Pin GPIO_PIN_3
#define ENC_PARAM_A_GPIO_Port GPIOG
#define BTN_FUNC_Pin GPIO_PIN_7
#define BTN_FUNC_GPIO_Port GPIOG
#define MY_LCD_RESET_Pin GPIO_PIN_6
#define MY_LCD_RESET_GPIO_Port GPIOC
#define MY_SD_DETECT_Pin GPIO_PIN_10
#define MY_SD_DETECT_GPIO_Port GPIOA
#define MY_LED_REAR_Pin GPIO_PIN_15
#define MY_LED_REAR_GPIO_Port GPIOA
#define BTN_DOWN_Pin GPIO_PIN_13
#define BTN_DOWN_GPIO_Port GPIOG
#define BTN_UP_Pin GPIO_PIN_14
#define BTN_UP_GPIO_Port GPIOG
#define BTN_YES_Pin GPIO_PIN_4
#define BTN_YES_GPIO_Port GPIOB
#define BTN_NO_Pin GPIO_PIN_7
#define BTN_NO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
