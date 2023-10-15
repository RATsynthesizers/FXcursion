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
#include "sdmmc.h"
#include "fatfs.h"

#include "../../Drivers/WAV/wav.h"
#include "../../Drivers/UI/ui_adapter_reg.h"
#include "../../Drivers/UI/Encoder.hpp"
#include "../../Drivers/UI/Button.hpp"
#include "../../Drivers/UI/Led.hpp"
#include "../../Drivers/AudioAdapter/AudioAdapter.h"
#include "../../Drivers/AudioTransfer/AudioTransfer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
static WAVE_HEADER RecorderWaveHeader = {

		0x52, 0x49, 0x46, 0x46,

		(sizeof(WAVE_HEADER) - 8), //The size of the entire wave file, initial value

		0x57, 0x41, 0x56, 0x45,

		0x66, 0x6d, 0x74, 0x20,

		16,

		1, //Encoding method. Linear PCM encoding

		2, //Stereo

		48000, //Sampling rate is 48k

		192000, //4 bytes per stereo sample

		4, //4 	 bytes per stereo sample

		16, //each sample requires 16 bits

		0x64, 0x61, 0x74, 0x61,

		0 //data length is initialized to 0

		};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile uint8_t recorderState = 0;
volatile unsigned long ulStatsTimerTicks;

extern volatile uint8_t looper_half;
extern TIM_HandleTypeDef htim6;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TouchGFXUpdateTask */
osThreadId_t TouchGFXUpdateTaskHandle;
const osThreadAttr_t TouchGFXUpdateTask_attributes = {
  .name = "TouchGFXUpdateTask",
  .stack_size = 4096 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for VSyncTask */
osThreadId_t VSyncTaskHandle;
const osThreadAttr_t VSyncTask_attributes = {
  .name = "VSyncTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UpdateFromUITask */
osThreadId_t UpdateFromUITaskHandle;
const osThreadAttr_t UpdateFromUITask_attributes = {
  .name = "UpdateFromUITask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for WriteSDTask */
osThreadId_t WriteSDTaskHandle;
const osThreadAttr_t WriteSDTask_attributes = {
  .name = "WriteSDTask",
  .stack_size = 8192 * 4,
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
/* Definitions for WriteSDQueue */
osMessageQueueId_t WriteSDQueueHandle;
const osMessageQueueAttr_t WriteSDQueue_attributes = {
  .name = "WriteSDQueue"
};
/* Definitions for AudioTransferSemaphore */
osSemaphoreId_t AudioTransferSemaphoreHandle;
const osSemaphoreAttr_t AudioTransferSemaphore_attributes = {
  .name = "AudioTransferSemaphore"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void UpdateFromUI(READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg);

extern "C" void touchgfxSignalVSync(void);
extern "C" void MX_USB_DEVICE_Init(void);
extern "C" void MX_USB_DEVICE_DeInit(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTouchGFXUpdateTask(void *argument);
void StartVSyncTask(void *argument);
void StartUpdateFromUITask(void *argument);
void StartWriteSDTask(void *argument);

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

  /* Create the semaphores(s) */
  /* creation of AudioTransferSemaphore */
  AudioTransferSemaphoreHandle = osSemaphoreNew(16, 16, &AudioTransferSemaphore_attributes);

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

  /* creation of WriteSDQueue */
  WriteSDQueueHandle = osMessageQueueNew (16, sizeof(uint8_t), &WriteSDQueue_attributes);

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

  /* creation of UpdateFromUITask */
  UpdateFromUITaskHandle = osThreadNew(StartUpdateFromUITask, NULL, &UpdateFromUITask_attributes);

  /* creation of WriteSDTask */
  WriteSDTaskHandle = osThreadNew(StartWriteSDTask, NULL, &WriteSDTask_attributes);

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
  /* USER CODE BEGIN StartDefaultTask */
	MX_SDMMC1_SD_Init();
	BSP_SD_Init();
	MX_FATFS_Init();
	MX_USB_DEVICE_Init();

//	HAL_SD_CardInfoTypeDef info;
//	HAL_SD_GetCardInfo(&hsd1, &info);

	audioAdapter_Init(&audioAdapter);  // uart interface to audio mcu
	audioTransfer_Init();

	HAL_SPI_TransmitReceive_DMA(&hspi2, UIadapterReg.UIoutputRegs,
			UIadapterReg.UIinputRegs, 3);
	/* Infinite loop */
	for (;;) {
		osThreadYield();
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

/* USER CODE BEGIN Header_StartWriteSDTask */
/**
 * @brief Function implementing the WriteSDTask thread.
 * @param argument: Not used
 * @retval None
 */

uint16_t writingBufTime = 0;
/* USER CODE END Header_StartWriteSDTask */
void StartWriteSDTask(void *argument)
{
  /* USER CODE BEGIN StartWriteSDTask */

	FRESULT res; /* FatFs function common result code */
	uint32_t byteswritten; /* File write/read counts */
	uint8_t record_msg;
	uint32_t recordedAudioSize = 0;
	osStatus_t stat;

	uint32_t temp = 0;

	/* Infinite loop */
	for (;;) {
		switch (recorderState) {
		case 0:
			osMessageQueueGet(WriteSDQueueHandle, &record_msg, 0U,
			osWaitForever);

			f_mount(&SDFatFS, (TCHAR const*) SDPath, 1);

			res = f_open(&SDFile, "wavtest.wav", FA_CREATE_ALWAYS | FA_WRITE);
			if (res != FR_OK)
				Error_Handler();

			MX_USB_DEVICE_DeInit();

			RecorderWaveHeader.DataChunk.Data_Size = 0;
			RecorderWaveHeader.RiffHeader.Riff_Size = (sizeof(WAVE_HEADER) - 8);

			res = f_write(&SDFile, (uint8_t*) &RecorderWaveHeader,
					sizeof(WAVE_HEADER), (UINT*) &byteswritten);
			if ((byteswritten != sizeof(WAVE_HEADER)) || (res != FR_OK))
				Error_Handler();

			recLed.setState(LED_R);

			recorderState++;

			break;

		case 1:

			stat = osMessageQueueGet(WriteSDQueueHandle, &record_msg, 0U, 0U);
			if (stat == osOK && record_msg == 0) {
				recorderState++;
				break;
			} else {
				stat = osSemaphoreAcquire(AudioTransferSemaphoreHandle,
				osWaitForever);

				temp = HAL_GetTick();

				res = f_write(&SDFile,
						(uint8_t*) (writeAudioBuffer)
								+ looper_half * WRITEAUDIOBUF_SIZE / 2,
						WRITEAUDIOBUF_SIZE / 2, (UINT*) &byteswritten);

				if ((byteswritten != WRITEAUDIOBUF_SIZE / 2) || (res != FR_OK))
					Error_Handler();
				else
					recordedAudioSize += byteswritten;

				writingBufTime = HAL_GetTick() - temp;
			}
			break;

		case 2:

			f_lseek(&SDFile, 0);

			RecorderWaveHeader.DataChunk.Data_Size = recordedAudioSize;

			// RIFF size is changed to the size of the entire file
			RecorderWaveHeader.RiffHeader.Riff_Size = (sizeof(WAVE_HEADER) - 8)
					+ recordedAudioSize;

			// Change wav file header information
			res = f_write(&SDFile, (uint8_t*) &RecorderWaveHeader,
					sizeof(WAVE_HEADER), (UINT*) &byteswritten);
			if ((byteswritten != sizeof(WAVE_HEADER)) || (res != FR_OK))
				Error_Handler();

			f_close(&SDFile);

			MX_USB_DEVICE_Init();

			recLed.setState(LED_OFF);

			recorderState = 0;
			recordedAudioSize = 0;
			break;
		default:
			break;
		}
		osThreadYield();
	}
  /* USER CODE END StartWriteSDTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void UpdateFromUI(READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg) {
	uint8_t i = ReadWriteUI_msg.reg;
	uint8_t j = ReadWriteUI_msg.bit;

	//UPDATEAUDIOPARAMS_OBJ_t updateAudioParams_msg;
	UPDATEGUIQUEUE_OBJ_t updateGUI_msg;
	uint8_t record_msg;

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

			osMessageQueuePut(UpdateGUIQueueHandle, &updateGUI_msg, 0U, 0);
			break;
		case 5:
			encs[1]->update();

			updateGUI_msg.uiObject = ENC_GUI_PARAMETER;
			updateGUI_msg.id = 1;
			updateGUI_msg.value = encs[1]->getState();

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
			buttons[2]->update();

			if (buttons[2]->getState() == BTN_PRESSED) {
				if (recorderState == 0)
					record_msg = 1;
				else
					record_msg = 0;
				osMessageQueuePut(WriteSDQueueHandle, &record_msg, 0U, 0);
			}
			break;
		default:
			break;
		}
		break;
	}
}
/* USER CODE END Application */

