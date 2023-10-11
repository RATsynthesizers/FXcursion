/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_touchgfx.h"

#include "../../Drivers/UI/ui_adapter_reg.h"
#include "../../Drivers/UI/Encoder.hpp"
#include "../../Drivers/UI/Button.hpp"
#include "../../Drivers/UI/Led.hpp"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile unsigned long ulStatsTimerTicks;
extern TIM_HandleTypeDef htim6;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TouchGFXUpdateTask */
osThreadId_t TouchGFXUpdateTaskHandle;
const osThreadAttr_t TouchGFXUpdateTask_attributes = {
  .name = "TouchGFXUpdateTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for VSyncTask */
osThreadId_t VSyncTaskHandle;
const osThreadAttr_t VSyncTask_attributes = {
  .name = "VSyncTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ReadWriteUITask */
osThreadId_t ReadWriteUITaskHandle;
const osThreadAttr_t ReadWriteUITask_attributes = {
  .name = "ReadWriteUITask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UpdateFromUITask */
osThreadId_t UpdateFromUITaskHandle;
const osThreadAttr_t UpdateFromUITask_attributes = {
  .name = "UpdateFromUITask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UpdateGUIQueue */
osMessageQueueId_t UpdateGUIQueueHandle;
const osMessageQueueAttr_t UpdateGUIQueue_attributes = {
  .name = "UpdateGUIQueue"
};
/* Definitions for ReadWriteUIQueue */
osMessageQueueId_t ReadWriteUIQueueHandle;
const osMessageQueueAttr_t ReadWriteUIQueue_attributes = {
  .name = "ReadWriteUIQueue"
};
/* Definitions for UpdateAudioParamsQueue */
osMessageQueueId_t UpdateAudioParamsQueueHandle;
const osMessageQueueAttr_t UpdateAudioParamsQueue_attributes = {
  .name = "UpdateAudioParamsQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void UpdateFromUI(READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg);

extern "C" void touchgfxSignalVSync(void);
extern "C" void MX_USB_DEVICE_Init(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTouchGFXUpdateTask(void *argument);
void StartVSyncTask(void *argument);
void StartReadWriteUITask(void *argument);
void StartUpdateFromUITask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void) {
	ulStatsTimerTicks = 0;
	HAL_TIM_Base_Start_IT(&htim6);
}

__weak unsigned long getRunTimeCounterValue(void) {
	return ulStatsTimerTicks;
}
/* USER CODE END 1 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of UpdateGUIQueue */
  UpdateGUIQueueHandle = osMessageQueueNew (16, sizeof(struct UPDATEGUIQUEUE_OBJ_t), &UpdateGUIQueue_attributes);

  /* creation of ReadWriteUIQueue */
  ReadWriteUIQueueHandle = osMessageQueueNew (16, sizeof(struct READWRITEUIQUEUE_OBJ_t), &ReadWriteUIQueue_attributes);

  /* creation of UpdateAudioParamsQueue */
  UpdateAudioParamsQueueHandle = osMessageQueueNew (16, sizeof(struct UPDATEAUDIOPARAMS_OBJ_t), &UpdateAudioParamsQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of TouchGFXUpdateTask */
  TouchGFXUpdateTaskHandle = osThreadNew(StartTouchGFXUpdateTask, NULL, &TouchGFXUpdateTask_attributes);

  /* creation of VSyncTask */
  VSyncTaskHandle = osThreadNew(StartVSyncTask, NULL, &VSyncTask_attributes);

  /* creation of ReadWriteUITask */
  ReadWriteUITaskHandle = osThreadNew(StartReadWriteUITask, NULL, &ReadWriteUITask_attributes);

  /* creation of UpdateFromUITask */
  UpdateFromUITaskHandle = osThreadNew(StartUpdateFromUITask, NULL, &UpdateFromUITask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
	/* Infinite loop */
	for (;;) {
		osThreadTerminate(osThreadGetId());
	}
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTouchGFXUpdateTask */
/**
 * @brief Function implementing the TouchGFXUpdateTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTouchGFXUpdateTask */
void StartTouchGFXUpdateTask(void *argument)
{
  /* USER CODE BEGIN StartTouchGFXUpdateTask */
	/* Infinite loop */
	MX_TouchGFX_Process();
	for (;;) {
		osThreadTerminate(osThreadGetId());
	}
  /* USER CODE END StartTouchGFXUpdateTask */
}

/* USER CODE BEGIN Header_StartVSyncTask */
/**
 * @brief Function implementing the VSyncTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartVSyncTask */
void StartVSyncTask(void *argument)
{
  /* USER CODE BEGIN StartVSyncTask */
	/* Infinite loop */
	for (;;) {
		touchgfxSignalVSync();
		osThreadYield();
	}
  /* USER CODE END StartVSyncTask */
}

/* USER CODE BEGIN Header_StartReadWriteUITask */
/**
 * @brief Function implementing the ReadWriteUITask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartReadWriteUITask */
void StartReadWriteUITask(void *argument)
{
  /* USER CODE BEGIN StartReadWriteUITask */
	/* Infinite loop */
	for (;;) {
		UIadapter_ReadWriteUI(&UIadapterReg);
		osThreadYield();
	}
  /* USER CODE END StartReadWriteUITask */
}

/* USER CODE BEGIN Header_StartUpdateFromUITask */
/**
 * @brief Function implementing the UpdateFromUITask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartUpdateFromUITask */
void StartUpdateFromUITask(void *argument)
{
  /* USER CODE BEGIN StartUpdateFromUITask */
	READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg;
	/* Infinite loop */
	for (;;) {
		osMessageQueueGet(ReadWriteUIQueueHandle, &ReadWriteUI_msg, 0U,
		osWaitForever);
		UpdateFromUI(ReadWriteUI_msg);
		osThreadYield();
	}
  /* USER CODE END StartUpdateFromUITask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void UpdateFromUI(READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg) {
	uint8_t i = ReadWriteUI_msg.reg;
	uint8_t j = ReadWriteUI_msg.bit;

	UPDATEAUDIOPARAMS_OBJ_t updateAudioParams_msg;
	UPDATEGUIQUEUE_OBJ_t updateGUI_msg;
	RECORDQUEUE_OBJ_t record_msg;

	switch (i) {
	case 0:
		switch (j) {
		case 0:
			buttons[1]->update();
			if (buttons[1]->getState() == BTN_PRESSED) {
				updateGUI_msg.uiObject = BTN_YES_GUI;
				updateGUI_msg.value = 1;

				osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			}
			break;
		case 7:
			buttons[0]->update();
			if (buttons[0]->getState() == BTN_PRESSED) {
				updateGUI_msg.uiObject = BTN_NO_GUI;
				updateGUI_msg.value = 1;

				osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			}
			break;
		case 2:
			encs[0]->update();

			updateGUI_msg.uiObject = ENC_GUI_PARAMETER;
			updateGUI_msg.id = 0;
			updateGUI_msg.value = encs[0]->getState();

//			updateAudioParams_msg.audioModule = MODULE_REVERB;
//			updateAudioParams_msg.parameter = PARAM_DECAY;
//			updateAudioParams_msg.value = (float) (encs[0]->getState()) / 20.0f;
//
//			osMessageQueuePut(UpdateAudioParamsQueueHandle,
//					&updateAudioParams_msg, 0U, 0);
			osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			break;
		case 5:
			encs[1]->update();

			updateGUI_msg.uiObject = ENC_GUI_PARAMETER;
			updateGUI_msg.id = 1;
			updateGUI_msg.value = encs[1]->getState();

//			updateAudioParams_msg.audioModule = MODULE_REVERB;
//			updateAudioParams_msg.parameter = PARAM_REVMIX;
//			updateAudioParams_msg.value = (float) (encs[1]->getState()) / 20.0f;
//
//			osMessageQueuePut(UpdateAudioParamsQueueHandle,
//					&updateAudioParams_msg, 0U, 0);
			osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			break;
		default:
			break;
		}
		break;
	case 1:
		switch (j) {
		case 2:
			encs[2]->update();

			updateGUI_msg.uiObject = ENC_GUI_PARAMETER;
			updateGUI_msg.id = 2;
			updateGUI_msg.value = encs[2]->getState();

			osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			break;
		case 5:
			encs[3]->update();
			updateGUI_msg.uiObject = ENC_GUI_PARAMETER;
			updateGUI_msg.id = 3;
			updateGUI_msg.value = encs[3]->getState();

			osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			break;
		default:
			break;
		}
		break;
	case 2:
		switch (j) {
		case 2:
			encs[4]->update();
			updateGUI_msg.uiObject = ENC_GUI_SCROLL;
			updateGUI_msg.value = encs[4]->getState();

			osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			break;
		case 6:
//			buttons[2]->update();
//
//			if (buttons[2]->getState() == BTN_PRESSED) {
//				osSemaphoreRelease(writeSDSemHandle);
//				record_msg.recorderState = 1;
//				osMessageQueuePut(UpdateGUIQueueHandle, &record_msg, 0U, 0);
//			}
			break;
		default:
			break;
		}
		break;
	}
}
/* USER CODE END Application */

