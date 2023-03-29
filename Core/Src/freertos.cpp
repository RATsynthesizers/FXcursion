/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb_device.h"
#include "../../Drivers/WAV/wav.h"
#include "../../Drivers/UI/ui_adapter_reg.h"
#include "../../Drivers/UI/Encoder.hpp"
#include "../../Drivers/UI/Button.hpp"
#include "../../Drivers/UI/Led.hpp"
#include "../../Modules/AudioSysObjects.hpp"
#include "string.h"
#include "fatfs.h"
#include "app_touchgfx.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
/* USER CODE BEGIN PTD */
extern WAVE_HEADER RecorderWaveHeader;
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
extern TIM_HandleTypeDef htim3;
extern "C" void touchgfxSignalVSync(void);

#ifdef _MAX_SS
FRESULT res; /* FatFs function common result code */
uint32_t byteswritten, bytesread; /* File write/read counts */
uint16_t writeAudioBufferHead;
int16_t writeAudioBuffer[_MAX_SS]; /* File write buffer */
uint16_t readAudioBuffer[_MAX_SS]; /* File read buffer */
#endif // _MAX_SS

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
uint32_t defaultTaskBuffer[400];
osStaticThreadDef_t defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = { .name = "defaultTask", .cb_mem =
		&defaultTaskControlBlock, .cb_size = sizeof(defaultTaskControlBlock),
		.stack_mem = &defaultTaskBuffer[0], .stack_size =
				sizeof(defaultTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };
/* Definitions for TouchGFXUpdateTask */
osThreadId_t TouchGFXUpdateTaskHandle;
uint32_t TouchGFXUpdateTaskBuffer[700];
osStaticThreadDef_t TouchGFXUpdateTaskControlBlock;
const osThreadAttr_t TouchGFXUpdateTask_attributes = { .name =
		"TouchGFXUpdateTask", .cb_mem = &TouchGFXUpdateTaskControlBlock,
		.cb_size = sizeof(TouchGFXUpdateTaskControlBlock), .stack_mem =
				&TouchGFXUpdateTaskBuffer[0], .stack_size =
				sizeof(TouchGFXUpdateTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };
/* Definitions for ReadWriteUITask */
osThreadId_t ReadWriteUITaskHandle;
uint32_t ReadWriteUITaskBuffer[400];
osStaticThreadDef_t ReadWriteUITaskControlBlock;
const osThreadAttr_t ReadWriteUITask_attributes = { .name = "ReadWriteUITask",
		.cb_mem = &ReadWriteUITaskControlBlock, .cb_size =
				sizeof(ReadWriteUITaskControlBlock), .stack_mem =
				&ReadWriteUITaskBuffer[0], .stack_size =
				sizeof(ReadWriteUITaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };
/* Definitions for vSyncTask */
osThreadId_t vSyncTaskHandle;
uint32_t vSyncTaskBuffer[300];
osStaticThreadDef_t vSyncTaskControlBlock;
const osThreadAttr_t vSyncTask_attributes = { .name = "vSyncTask", .cb_mem =
		&vSyncTaskControlBlock, .cb_size = sizeof(vSyncTaskControlBlock),
		.stack_mem = &vSyncTaskBuffer[0], .stack_size = sizeof(vSyncTaskBuffer),
		.priority = (osPriority_t) osPriorityNormal, };
/* Definitions for WriteSDTask */
osThreadId_t WriteSDTaskHandle;
uint32_t WriteSDTaskBuffer[1000];
osStaticThreadDef_t WriteSDTaskControlBlock;
const osThreadAttr_t WriteSDTask_attributes = { .name = "WriteSDTask", .cb_mem =
		&WriteSDTaskControlBlock, .cb_size = sizeof(WriteSDTaskControlBlock),
		.stack_mem = &WriteSDTaskBuffer[0], .stack_size =
				sizeof(WriteSDTaskBuffer), .priority =
				(osPriority_t) osPriorityHigh, };
/* Definitions for UpdateFromUITask */
osThreadId_t UpdateFromUITaskHandle;
uint32_t UpdateFromUITaskBuffer[500];
osStaticThreadDef_t UpdateFromUITaskControlBlock;
const osThreadAttr_t UpdateFromUITask_attributes = { .name = "UpdateFromUITask",
		.cb_mem = &UpdateFromUITaskControlBlock, .cb_size =
				sizeof(UpdateFromUITaskControlBlock), .stack_mem =
				&UpdateFromUITaskBuffer[0], .stack_size =
				sizeof(UpdateFromUITaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };
/* Definitions for RecordInputTask */
osThreadId_t RecordInputTaskHandle;
uint32_t RecordInputTaskBuffer[1000];
osStaticThreadDef_t RecordInputTaskControlBlock;
const osThreadAttr_t RecordInputTask_attributes = { .name = "RecordInputTask",
		.cb_mem = &RecordInputTaskControlBlock, .cb_size =
				sizeof(RecordInputTaskControlBlock), .stack_mem =
				&RecordInputTaskBuffer[0], .stack_size =
				sizeof(RecordInputTaskBuffer), .priority =
				(osPriority_t) osPriorityNormal, };
/* Definitions for UpdateGUIQueue */
osMessageQueueId_t UpdateGUIQueueHandle;
uint8_t UpdateGUIQueueBuffer[12 * sizeof(struct UPDATEGUIQUEUE_OBJ_t)];
osStaticMessageQDef_t UpdateGUIQueueControlBlock;
const osMessageQueueAttr_t UpdateGUIQueue_attributes = { .name =
		"UpdateGUIQueue", .cb_mem = &UpdateGUIQueueControlBlock, .cb_size =
		sizeof(UpdateGUIQueueControlBlock), .mq_mem = &UpdateGUIQueueBuffer,
		.mq_size = sizeof(UpdateGUIQueueBuffer) };
/* Definitions for ReadWriteUIQueue */
osMessageQueueId_t ReadWriteUIQueueHandle;
uint8_t ReadWriteUIQueueBuffer[12 * sizeof(struct READWRITEUIQUEUE_OBJ_t)];
osStaticMessageQDef_t ReadWriteUIQueueControlBlock;
const osMessageQueueAttr_t ReadWriteUIQueue_attributes = { .name =
		"ReadWriteUIQueue", .cb_mem = &ReadWriteUIQueueControlBlock, .cb_size =
		sizeof(ReadWriteUIQueueControlBlock), .mq_mem = &ReadWriteUIQueueBuffer,
		.mq_size = sizeof(ReadWriteUIQueueBuffer) };
/* Definitions for UpdateAudioParamsQueue */
osMessageQueueId_t UpdateAudioParamsQueueHandle;
uint8_t UpdateAudioParamsQueueBuffer[12 * sizeof(struct UPDATEAUDIOPARAMS_OBJ_t)];
osStaticMessageQDef_t UpdateAudioParamsQueueControlBlock;
const osMessageQueueAttr_t UpdateAudioParamsQueue_attributes = { .name =
		"UpdateAudioParamsQueue", .cb_mem = &UpdateAudioParamsQueueControlBlock,
		.cb_size = sizeof(UpdateAudioParamsQueueControlBlock), .mq_mem =
				&UpdateAudioParamsQueueBuffer, .mq_size =
				sizeof(UpdateAudioParamsQueueBuffer) };
/* Definitions for GaugeValuesQueue */
osMessageQueueId_t GaugeValuesQueueHandle;
uint8_t GaugeValuesQueueBuffer[12 * sizeof(struct UPDATEGAUGEVALUES_OBJ_t)];
osStaticMessageQDef_t GaugeValuesQueueControlBlock;
const osMessageQueueAttr_t GaugeValuesQueue_attributes = { .name =
		"GaugeValuesQueue", .cb_mem = &GaugeValuesQueueControlBlock, .cb_size =
		sizeof(GaugeValuesQueueControlBlock), .mq_mem = &GaugeValuesQueueBuffer,
		.mq_size = sizeof(GaugeValuesQueueBuffer) };
/* Definitions for AllGaugeValuesQueue */
osMessageQueueId_t AllGaugeValuesQueueHandle;
uint8_t AllGaugeValuesQueueBuffer[12 * sizeof(struct ALLGAUGEVALUES_OBJ_t)];
osStaticMessageQDef_t AllGaugeValuesQueueControlBlock;
const osMessageQueueAttr_t AllGaugeValuesQueue_attributes = { .name =
		"AllGaugeValuesQueue", .cb_mem = &AllGaugeValuesQueueControlBlock,
		.cb_size = sizeof(AllGaugeValuesQueueControlBlock), .mq_mem =
				&AllGaugeValuesQueueBuffer, .mq_size =
				sizeof(AllGaugeValuesQueueBuffer) };
/* Definitions for RecordInputQueue */
osMessageQueueId_t RecordInputQueueHandle;
uint8_t RecordInputQueueBuffer[12 * sizeof(struct RECORDQUEUE_OBJ_t)];
osStaticMessageQDef_t RecordInputQueueControlBlock;
const osMessageQueueAttr_t RecordInputQueue_attributes = { .name =
		"RecordInputQueue", .cb_mem = &RecordInputQueueControlBlock, .cb_size =
		sizeof(RecordInputQueueControlBlock), .mq_mem = &RecordInputQueueBuffer,
		.mq_size = sizeof(RecordInputQueueBuffer) };
/* Definitions for LooperQueue */
osMessageQueueId_t LooperQueueHandle;
uint8_t looperQueueBuffer[12 * sizeof(struct LOOPERQUEUE_OBJ_t)];
osStaticMessageQDef_t looperQueueControlBlock;
const osMessageQueueAttr_t LooperQueue_attributes = { .name = "LooperQueue",
		.cb_mem = &looperQueueControlBlock, .cb_size =
				sizeof(looperQueueControlBlock), .mq_mem = &looperQueueBuffer,
		.mq_size = sizeof(looperQueueBuffer) };
/* Definitions for SDMutex */
osMutexId_t SDMutexHandle;
osStaticMutexDef_t SDMutexControlBlock;
const osMutexAttr_t SDMutex_attributes = { .name = "SDMutex", .cb_mem =
		&SDMutexControlBlock, .cb_size = sizeof(SDMutexControlBlock), };
/* Definitions for requestParamValuesSem */
osSemaphoreId_t requestParamValuesSemHandle;
osStaticSemaphoreDef_t requestParamValuesSemControlBlock;
const osSemaphoreAttr_t requestParamValuesSem_attributes = { .name =
		"requestParamValuesSem", .cb_mem = &requestParamValuesSemControlBlock,
		.cb_size = sizeof(requestParamValuesSemControlBlock), };
/* Definitions for writeSDSem */
osSemaphoreId_t writeSDSemHandle;
osStaticSemaphoreDef_t writeSDSemControlBlock;
const osSemaphoreAttr_t writeSDSem_attributes = { .name = "writeSDSem",
		.cb_mem = &writeSDSemControlBlock, .cb_size =
				sizeof(writeSDSemControlBlock), };

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void UpdateFromUI(READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg);
void WriteSDAudio(uint8_t *recorderState, uint32_t *recordedAudioSize);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTouchGFXUpdateTask(void *argument);
void StartReadWriteUITask(void *argument);
void StartVSyncTask(void *argument);
void StartWriteSDTask(void *argument);
void StartUpdateFromUITask(void *argument);
void StartRecordInputTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void) {
	ulStatsTimerTicks = 0;
	HAL_TIM_Base_Start_IT(&htim3);
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
	/* Create the mutex(es) */
	/* creation of SDMutex */
	SDMutexHandle = osMutexNew(&SDMutex_attributes);

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* Create the semaphores(s) */
	/* creation of requestParamValuesSem */
	requestParamValuesSemHandle = osSemaphoreNew(1, 1,
			&requestParamValuesSem_attributes);

	/* creation of writeSDSem */
	writeSDSemHandle = osSemaphoreNew(1, 1, &writeSDSem_attributes);

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	osSemaphoreAcquire(requestParamValuesSemHandle, 0);
	osSemaphoreAcquire(writeSDSemHandle, 0);
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	/* USER CODE END RTOS_TIMERS */

	/* Create the queue(s) */
	/* creation of UpdateGUIQueue */
	UpdateGUIQueueHandle = osMessageQueueNew(12,
			sizeof(struct UPDATEGUIQUEUE_OBJ_t), &UpdateGUIQueue_attributes);

	/* creation of ReadWriteUIQueue */
	ReadWriteUIQueueHandle = osMessageQueueNew(12,
			sizeof(struct READWRITEUIQUEUE_OBJ_t),
			&ReadWriteUIQueue_attributes);

	/* creation of UpdateAudioParamsQueue */
	UpdateAudioParamsQueueHandle = osMessageQueueNew(12,
			sizeof(struct UPDATEAUDIOPARAMS_OBJ_t),
			&UpdateAudioParamsQueue_attributes);

	/* creation of GaugeValuesQueue */
	GaugeValuesQueueHandle = osMessageQueueNew(12,
			sizeof(struct UPDATEGAUGEVALUES_OBJ_t),
			&GaugeValuesQueue_attributes);

	/* creation of AllGaugeValuesQueue */
	AllGaugeValuesQueueHandle = osMessageQueueNew(12,
			sizeof(struct ALLGAUGEVALUES_OBJ_t),
			&AllGaugeValuesQueue_attributes);

	/* creation of RecordInputQueue */
	RecordInputQueueHandle = osMessageQueueNew(12,
			sizeof(struct RECORDQUEUE_OBJ_t), &RecordInputQueue_attributes);

	/* creation of LooperQueue */
	LooperQueueHandle = osMessageQueueNew(12, sizeof(struct LOOPERQUEUE_OBJ_t),
			&LooperQueue_attributes);

/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	/* USER CODE END RTOS_QUEUES */

	/* Create the thread(s) */
	/* creation of defaultTask */
	defaultTaskHandle = osThreadNew(StartDefaultTask, NULL,
			&defaultTask_attributes);

	/* creation of TouchGFXUpdateTask */
	TouchGFXUpdateTaskHandle = osThreadNew(StartTouchGFXUpdateTask, NULL,
			&TouchGFXUpdateTask_attributes);

	/* creation of ReadWriteUITask */
	ReadWriteUITaskHandle = osThreadNew(StartReadWriteUITask, NULL,
			&ReadWriteUITask_attributes);

	/* creation of vSyncTask */
	vSyncTaskHandle = osThreadNew(StartVSyncTask, NULL, &vSyncTask_attributes);

	/* creation of WriteSDTask */
	WriteSDTaskHandle = osThreadNew(StartWriteSDTask, NULL,
			&WriteSDTask_attributes);

	/* creation of UpdateFromUITask */
	UpdateFromUITaskHandle = osThreadNew(StartUpdateFromUITask, NULL,
			&UpdateFromUITask_attributes);

	/* creation of RecordInputTask */
	RecordInputTaskHandle = osThreadNew(StartRecordInputTask, NULL,
			&RecordInputTask_attributes);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

	/* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
	//osEventFlagsSet(audioUpdateEventHandle, 1);
	/* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
	/* init code for USB_DEVICE */
	MX_USB_DEVICE_Init();
	/* USER CODE BEGIN StartDefaultTask */
	/* Infinite loop */
	for (;;) {
		osDelay(1);
	}
	/* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTouchGFXUpdateTask */
/**
 * @brief Function implementing the TouchGFXUpdate thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartTouchGFXUpdateTask */
void StartTouchGFXUpdateTask(void *argument) {
	/* USER CODE BEGIN StartTouchGFXUpdateTask */
	MX_TouchGFX_Process();
	/* USER CODE END StartTouchGFXUpdateTask */
}

/* USER CODE BEGIN Header_StartReadWriteUITask */
/**
 * @brief Function implementing the ReadWriteUI thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartReadWriteUITask */
void StartReadWriteUITask(void *argument) {
	/* USER CODE BEGIN StartReadWriteUITask */
	/* Infinite loop */
	for (;;) {
		UIadapter_ReadWriteUI(&UIadapterReg);
		osThreadYield();
	}
	/* USER CODE END StartReadWriteUITask */
}

/* USER CODE BEGIN Header_StartVSyncTask */
/**
 * @brief Function implementing the vSyncTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartVSyncTask */
void StartVSyncTask(void *argument) {
	/* USER CODE BEGIN StartVSyncTask */

	/* Infinite loop */
	for (;;) {
		touchgfxSignalVSync();
		osThreadYield();
	}
	/* USER CODE END StartVSyncTask */
}

/* USER CODE BEGIN Header_StartWriteSDTask */
/**
 * @brief Function implementing the WriteSDTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartWriteSDTask */
void StartWriteSDTask(void *argument) {
	/* USER CODE BEGIN StartWriteSDTask */
	uint8_t recorderState = 0;
	uint32_t recordedAudioSize = 0;
	/* Infinite loop */
	for (;;) {
		WriteSDAudio(&recorderState, &recordedAudioSize);
	}
	/* USER CODE END StartWriteSDTask */
}

/* USER CODE BEGIN Header_StartUpdateFromUITask */
/**
 * @brief  Function implementing the UpdateFromUITask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartUpdateFromUITask */
void StartUpdateFromUITask(void *argument) {
	/* USER CODE BEGIN StartUpdateFromUITask */
	/* Infinite loop */
	READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg;
	for (;;) {
		UpdateFromUI(ReadWriteUI_msg);
	}
	/* USER CODE END StartUpdateFromUITask */
}

/* USER CODE BEGIN Header_StartRecordInputTask */
/**
 * @brief Function implementing the RecordInputTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartRecordInputTask */
void StartRecordInputTask(void *argument) {
	/* USER CODE BEGIN StartRecordInputTask */
	/* Infinite loop */
	for (;;) {
		osDelay(1);
	}
	/* USER CODE END StartRecordInputTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void UpdateFromUI(READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg) {

	/*
	 * Receive QueueMsg from UI, decipher from which place update occured.
	 * Adjust software parameters accordingly.
	 */

	osMessageQueueGet(ReadWriteUIQueueHandle, &ReadWriteUI_msg, 0U, osWaitForever);

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
				updateGUI_msg.id = 0;
				updateGUI_msg.value = 1;

				osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			}
			break;
		case 7:
			buttons[0]->update();
			if (buttons[0]->getState() == BTN_PRESSED) {
				updateGUI_msg.uiObject = BTN_NO_GUI;
				updateGUI_msg.id = 0;
				updateGUI_msg.value = 1;

				osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			}
			break;
		case 2:
			encs[0]->update();

			updateGUI_msg.uiObject = ENC_GUI_PARAMETER;
			updateGUI_msg.id = 0;
			updateGUI_msg.value = encs[0]->getState();

			updateAudioParams_msg.audioModule = MODULE_REVERB;
			updateAudioParams_msg.parameter = PARAM_DECAY;
			updateAudioParams_msg.value = (float) (encs[0]->getState()) / 20.0f;

			osMessageQueuePut(UpdateAudioParamsQueueHandle,
					&updateAudioParams_msg, 0U, 0);
			osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			break;
		case 5:
			encs[1]->update();

			updateGUI_msg.uiObject = ENC_GUI_PARAMETER;
			updateGUI_msg.id = 1;
			updateGUI_msg.value = encs[1]->getState();

			updateAudioParams_msg.audioModule = MODULE_REVERB;
			updateAudioParams_msg.parameter = PARAM_REVMIX;
			updateAudioParams_msg.value = (float) (encs[1]->getState()) / 20.0f;

			osMessageQueuePut(UpdateAudioParamsQueueHandle,
					&updateAudioParams_msg, 0U, 0);
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
			updateGUI_msg.id = 0;
			updateGUI_msg.value = encs[4]->getState();

			osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			break;
		case 6:
			buttons[2]->update();

			if (buttons[2]->getState() == BTN_PRESSED) {
				osSemaphoreRelease(writeSDSemHandle);
				record_msg.recorderState = 1;
				osMessageQueuePut(UpdateGUIQueueHandle, &record_msg, 0U, 0);
			}
			break;
		default:
			break;
		}
		break;
	}

	osThreadYield();
}

void WriteSDAudio(uint8_t *recorderState, uint32_t *recordedAudioSize) {

	/* recorderState values:
	 * 0. ready
	 * 1. write wav header
	 * 2. data recording
	 * 3. rewrite wav header
	 */

	RECORDQUEUE_OBJ_t record_msg;
	LOOPERQUEUE_OBJ_t looper_msg;

	switch (*recorderState) {
	case 0:
		osMessageQueueGet(UpdateGUIQueueHandle, &record_msg, 0U, osWaitForever);
		*recorderState = record_msg.recorderState;
		break;
	case 1:
		osMutexWait(SDMutexHandle, osWaitForever);
		MX_USB_DEVICE_DeInit();
		if (f_mount(&SDFatFS, (TCHAR const*) SDPath, 0) != FR_OK) {
			Error_Handler();
		} else {
			//Open file for writing (Create)
			if (f_open(&SDFile, "wavtest.wav", FA_CREATE_ALWAYS | FA_WRITE)
					!= FR_OK) {
				Error_Handler();
			} else {
				res = f_write(&SDFile, (uint8_t*) &RecorderWaveHeader,
						sizeof(WAVE_HEADER), (UINT*) &byteswritten);
				if ((byteswritten == 0) || (res != FR_OK)) {
					Error_Handler();
				} else {
					*recorderState = 2;
				}
			}
		}
		break;
	case 2:
		osMessageQueueGet(LooperQueueHandle, &looper_msg, 0U, osWaitForever);
		if (looper_msg.recording == 1) {
			res = f_write(&SDFile,
					(uint8_t*) (&writeAudioBuffer)
							+ looper_msg.half * sizeof(writeAudioBuffer) / 2,
					sizeof(writeAudioBuffer) / 2, (UINT*) &byteswritten);
		} else {
			*recorderState = 3;
			break;
		}

		if (res != FR_OK) {
			Error_Handler();
		} else {
			recordedAudioSize += byteswritten;
			byteswritten = sizeof(writeAudioBuffer) / 2;
		}
		break;
	case 3:
		f_lseek(&SDFile, 0);

		RecorderWaveHeader.DataChunk.Data_Size = *recordedAudioSize;

		// RIFF size is changed to the size of the entire file
		RecorderWaveHeader.RiffHeader.Riff_Size = (sizeof(WAVE_HEADER) - 8)
				+ *recordedAudioSize;

		// Change wav file header information
		f_write(&SDFile, (uint8_t*) &RecorderWaveHeader, sizeof(WAVE_HEADER),
				(UINT*) &byteswritten);

		f_close(&SDFile);
		f_mount(&SDFatFS, (TCHAR const*) NULL, 0);
		MX_USB_DEVICE_Init();
		osMutexRelease(SDMutexHandle);

		*recorderState = 0;
		*recordedAudioSize = 0;
		break;
	default:
		break;
	}
}

/* USER CODE END Application */

