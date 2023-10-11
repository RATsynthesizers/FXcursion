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
#include "MemRegions.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef enum {
	NO_OBJECT, BTN_YES_GUI, BTN_NO_GUI, ENC_GUI_SCROLL, ENC_GUI_PARAMETER
} UIObject_Option;

typedef enum {
	MODULE_AMP, MODULE_REVERB
} AudioModule_Option;

typedef enum {
	PARAM_DECAY, PARAM_REVMIX
} ModuleParameter_option;

struct LOOPERQUEUE_OBJ_t{
	uint8_t half;
	uint8_t recording;
};

struct RECORDQUEUE_OBJ_t{
	uint8_t recorderState;
};

struct UPDATEGUIQUEUE_OBJ_t{
	UIObject_Option uiObject;
	uint8_t id;
	int8_t value;
};

struct UPDATEAUDIOPARAMS_OBJ_t{
	AudioModule_Option audioModule;
	ModuleParameter_option parameter;
	float value;
};

struct READWRITEUIQUEUE_OBJ_t{
	uint8_t reg;
	uint8_t bit;
};

struct UPDATEGAUGEVALUES_OBJ_t{
	uint8_t gaugeNum;
	float value;
};

struct ALLGAUGEVALUES_OBJ_t{
	float value[4];
};

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
#define SD_DETECT_Pin GPIO_PIN_0
#define SD_DETECT_GPIO_Port GPIOC
#define USER_LCD_RESET_Pin GPIO_PIN_6
#define USER_LCD_RESET_GPIO_Port GPIOC
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
