/**
 * @file      Init.c
 *
 * @details   Initializing sequence
 *
 * @version   1.0.0
 *
 * @authors   Predtechenskii Dmitrii (predtech4@yandex.ru)
 *
 * \date      12.08.2025 - First release
 *
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get native header
#include "Init.h"

// Get common cfg parameters
#include "common_cfg.h"

// Get custom std_lib interface
#include "std_lib.h"

// Get display interface
#include "ili9341.h"

#include "touchgfx_wrapper.h"

/*

// Get pixel driver interface
#include "pixel_drv.h"

// Get pub-sub interface
#include "pubsub.h"

// Get settings module interface
#include "ecuid.h"

// Get settings component interface
#include "Settings.h"

// Get JSON library parser interface
#include "jsmn_api.h"

*/


#include "adc.h"
#include "crc.h"
#include "dma.h"
#include "dma2d.h"
#include "fatfs.h"
#include "i2c.h"
#include "jpeg.h"
#include "quadspi.h"
#include "rtc.h"
#include "sdmmc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "fmc.h"
#include "app_touchgfx.h"
#include "w9812g6jh.h"


/***************************************************************************************************
* Definitions of global (public) variables
***************************************************************************************************/

/// None.


/***************************************************************************************************
* Definitions of module constants
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of local (private) data types
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Definitions of static global (private) variables
***************************************************************************************************/

/* Init task handle */
static osThreadId initThreadHandle;
/* TouchGFX task handle */
static osThreadId TouchGFXTaskHandle;
#ifdef DEBUG
/* Monitoring task handle */
static osThreadId monitoringThreadHandle;
#endif
/* IDLE task control block */
static StaticTask_t xIdleTaskTCBBuffer;
/* IDLE task stack buffer */
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
/* Timer task control block */
static StaticTask_t xTimerTaskTCBBuffer;
/* Timer task stack buffer */
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

/***************************************************************************************************
* Declarations of local (private) functions
***************************************************************************************************/

/// System Clock Configuration
static void SystemClock_Config(void);
/// MPU Configuration
static void MPU_Config(void);
/// Creates initialization thread */
static void CreateInitThread(void);
/// Init thread
static void InitThread(void const *argument);
#ifdef DEBUG
/// Os monitoring thread
static void MonitoringThread(void const *argument);
#endif

/// TouchGFX thread
extern void TouchGFX_Task(void const *argument);


/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

/* Enables SWO printf */
int __io_putchar(int ch)
{
	// Write character to ITM ch.0
	ITM_SendChar(ch);
	return(ch);
}

/**
 * @fn    void vApplicationGetIdleTaskMemory(StaticTask_t**, StackType_t**, uint32_t*)
 *
 * @brief This function is called on the IDLE task creation event.
 *
 * @param[in] ppxTimerTaskTCBBuffer - pointer to store timer task control block.
 * @param[in] ppxTimerTaskStackBuffer - pointer to store timer task stack buffer.
 * @param[in] pulTimerTaskStackSize - pointer to store timer task stack size.
 *
 * @return    None.
 */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
	*ppxIdleTaskStackBuffer = &xIdleStack[0];
	*pulIdleTaskStackSize = 70;
}



/**
 * @fn    void vApplicationGetTimerTaskMemory(StaticTask_t**, StackType_t**, uint32_t*)
 *
 * @brief This function is called on the Timer task creation event.
 *
 * @param[in] ppxTimerTaskTCBBuffer - pointer to store timer task control block.
 * @param[in] ppxTimerTaskStackBuffer - pointer to store timer task stack buffer.
 * @param[in] pulTimerTaskStackSize - pointer to store timer task stack size.
 *
 * @return    None.
 */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
	*ppxTimerTaskStackBuffer = &xTimerStack[0];
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}



/**
 * @fn    void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
 *
 * @brief This function is called on the stack overflow event.
 *
 * @param[in] xTask - task handle.
 * @param[in] pcTaskName - task name.
 *
 * @return    None.
 */
#ifdef DEBUG
void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char* pcTaskName)
{
    printf("Task \"%s\" stack overflow!\n", pcTaskName);
}
#endif



/**
 * @fn    void vApplicationMallocFailedHook(void)
 *
 * @brief This function is called on the memory allocation failed event.
 *
 * @param[in] None.
 *
 * @return    None.
 */
#ifdef DEBUG
void vApplicationMallocFailedHook(void)
{
    printf("Application mem.alloc failed!\n");
}
#endif



#if (configGENERATE_RUN_TIME_STATS == 1)
/**
 * @fn    void vConfigureTimerForRunTimeStats(void)
 *
 * @brief Configures run time counter.
 *
 * @param[in] None.
 *
 * @return    None.
 */
void vConfigureTimerForRunTimeStats(void)
{
    /* Run time timer handler */
    static TIM_HandleTypeDef htim5;

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    __HAL_RCC_TIM5_CLK_ENABLE();
    /* Configure timer to 20kHz: Fclk = 84MHz -> set prescaler to 4199 to get 50KHz -> 1 tick - 20us */
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = (84U * 20U) - 1U;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 0xFFFFFFFFUL;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_TIM_Base_Start(&htim5);
}



/**
 * @fn    uint32_t vGetRunTimeCounterValue(void)
 *
 * @brief Returns run time counter value.
 *
 * @param[in] None.
 *
 * @return    None.
 */
uint32_t vGetRunTimeCounterValue(void)
{
    return TIM5->CNT;
}
#endif



/**
 * @fn    void init_all(void)
 *
 * @brief All initialization function.
 *
 * @param[in] None.
 *
 * @return    None.
 */
void init_all(void)
{
	/* MPU Configuration--------------------------------------------------------*/
	MPU_Config();

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C4_Init();
    MX_QUADSPI_Init();
    MX_SDMMC1_SD_Init();
    MX_SPI1_Init();
    MX_SPI2_Init();
    MX_USART2_UART_Init();
    MX_I2C2_Init();
    MX_SPI5_Init();
    MX_UART4_Init();
    MX_ADC1_Init();
    MX_TIM8_Init();
    MX_FMC_Init();
    MX_CRC_Init();
    MX_DMA2D_Init();
    MX_FATFS_Init();
    MX_JPEG_Init();
    MX_RTC_Init();


    /* Initialize display */

    W9812G6JH_Init(&hsdram1);

    HAL_Delay(10);

    lcdInit();
    lcdSetWindow(0, 0, 320-1, 240-1); // force whole framebuffer drawing

//    uint32_t b = 0xFFBFAFFF;
//    _RAM_WRITE32(b, 0 );
//    uint32_t a = _RAM_READ32(0);

//    memset((U8*) 0xC0000000, 0xA9, 0x1000000);

    memset((U8*) 0xC0000000, 0x00, 0x10000);
//    memcpy(&a, (U8*) 0xC0000004, 4);

    /* Initialize TouchGFX */
    MX_TouchGFX_Init();
    /* Create initialization thread */
    CreateInitThread();
}



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @fn    void CreateInitThread(void)
 *
 * @brief Creates initialization thread.
 *
 * @param[in] None.
 *
 * @return    None.
 */
static void CreateInitThread(void)
{
    osThreadDef(InitThread, InitThread, osPriorityNormal, 0, 1000U);
    initThreadHandle = osThreadCreate(osThread(InitThread), NULL);
    //osThreadSuspend(initThreadHandle);
}



/**
 * @fn    void InitThread(void)
 *
 * @brief Init thread.
 *
 * @param[in] argument - pointer to input arguments.
 *
 * @return    None.
 */

uint8_t flag = 0;

static void InitThread(void const *argument)
{
//    /* Initialize pubsub service */
//    if (RESULT_NOT_OK == PUBSUB_Init())
//    {
//        printf("PUBSUB init failed!\n");
//    }
//
//    /* Initialize pixels */
//    if (RESULT_OK != PIXEL_Init())
//    {
//        printf("PIXEL initialize failed!\n");
//    }

	char test_txt[] = "> HELLO WORLD!";
	  lcdTest();
	  lcdSetCursor(50,100);
	  lcdSetTextFont(&Font12);
	  lcdPrintf(test_txt);
	//memset((uint8_t*) LCD_BASE1, 0, 0x5000);
	flag = 1;

	/* definition and creation of TouchGFXTask */
	osThreadDef(TouchGFXTask, TouchGFX_Task, osPriorityBelowNormal, 0, 4096U);
	TouchGFXTaskHandle = osThreadCreate(osThread(TouchGFXTask), NULL);


#ifdef DEBUG
    osThreadDef(MonitoringThread, MonitoringThread, osPriorityBelowNormal, 0, 200U);
    monitoringThreadHandle = osThreadCreate(osThread(MonitoringThread), NULL);
#endif





    for(;;)
    {
        /* Delete the Init Thread */
        osThreadTerminate(initThreadHandle);
    }
}



#ifdef DEBUG
/**
 * @fn    void MonitoringThread(void)
 *
 * @brief Os monitoring thread.
 *
 * @param[in] argument - pointer to input arguments.
 *
 * @return    None.
 */
void MonitoringThread(void const *argument)
{
    (void)argument;

    static const S8 aTaskStateChars[] =
    {
        /* eRunning (executed) */
        'X',
        /* eReady */
        'R',
        /* eBlocked */
        'B',
        /* eSuspended */
        'S',
        /* eDeleted */
        'D'
    };
    #define STAT_LINE_STR    "|-----------------------------------------------------------|\n"
    #define STAT_EMPTY_STR   "|                                                           |\n"
    static char stat_str[sizeof(STAT_EMPTY_STR) + 1U];
    stat_str[SIZE_OF_ARRAY(stat_str) - 1U] = 0;

    osDelay(1000);

    for(;;)
    {
        /* Get current tasks count */
        UBaseType_t nTaskCount = uxTaskGetNumberOfTasks();
        /* Variable to store total run time */
        U32 nTotalRuntime;

        /* Allocate a TaskStatus_t structure for each task */
        TaskStatus_t* pxTaskStatusArray = (TaskStatus_t*)pvPortMalloc(nTaskCount * sizeof(TaskStatus_t));
        if (NULL_PTR != pxTaskStatusArray)
        {
            /* Generate raw status information about each task */
            nTaskCount = uxTaskGetSystemState(pxTaskStatusArray, nTaskCount, &nTotalRuntime);

            printf("\n\n");
            printf("######################## RUN TIME STATS #####################\n"STAT_LINE_STR);
            printf("|TaskName            |State|Priority|Min.stack|   Runtime   |\n"STAT_LINE_STR);
            U8 nTaskNum;
            S8 nTaskStateChar;
            for (nTaskNum = 0U; nTaskNum < nTaskCount; nTaskNum++)
            {
                if (pxTaskStatusArray[nTaskNum].eCurrentState <= SIZE_OF_ARRAY(aTaskStateChars))
                {
                    nTaskStateChar = aTaskStateChars[(U8)pxTaskStatusArray[nTaskNum].eCurrentState];
                }
                else
                {
                    nTaskStateChar = 'I';
                }

                char aTaskName[21U];
                strncpy (aTaskName, pxTaskStatusArray[nTaskNum].pcTaskName, SIZE_OF_ARRAY(aTaskName));
                U8 nTaskNameLen = strlen(aTaskName);
                if (nTaskNameLen < SIZE_OF_ARRAY(aTaskName))
                {
                    memset(&aTaskName[nTaskNameLen], ' ', SIZE_OF_ARRAY(aTaskName) - nTaskNameLen);
                }
                aTaskName[20U] = 0U;

                memcpy(stat_str, STAT_EMPTY_STR, sizeof(STAT_EMPTY_STR));
                std_sprintf(&stat_str[1U],
                            "%s|  %c  |   %u    |  %4u   | %u",
                            aTaskName,
                            nTaskStateChar,
                            pxTaskStatusArray[nTaskNum].uxCurrentPriority,
                            pxTaskStatusArray[nTaskNum].usStackHighWaterMark,
                            pxTaskStatusArray[nTaskNum].ulRunTimeCounter);
                printf(stat_str);
            }

            /* The array is no longer needed, free the memory it consumes */
            vPortFree( pxTaskStatusArray );
        }

        /* Print line */
        printf(STAT_LINE_STR);

        /* Print heap stats */
        memcpy(stat_str, STAT_EMPTY_STR, sizeof(STAT_EMPTY_STR));
        std_sprintf(&stat_str[1U],
                    "Heap free size: %u, min.heap free size: %u",
                    xPortGetFreeHeapSize(),
                    xPortGetMinimumEverFreeHeapSize());
        printf(stat_str);

        /* Print uptime stats */
        memcpy(stat_str, STAT_EMPTY_STR, sizeof(STAT_EMPTY_STR));
        std_sprintf(&stat_str[1U],
                    "Total runtime: %u, uptime: %u ms",
                    nTotalRuntime,
                    xTaskGetTickCount() * portTICK_PERIOD_MS);
        printf(stat_str);

        /* Print line */
        printf(STAT_LINE_STR);
        osDelay(60000);
    }
}
#endif



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 120;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}


/* MPU Configuration */
static void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct = {0};

	/* Disables the MPU */
	HAL_MPU_Disable();

	/** Initializes and configures the Region and the memory to be protected
	*/
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER2;
	MPU_InitStruct.BaseAddress = 0x60000000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_64MB;
	MPU_InitStruct.SubRegionDisable = 0x0;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);


	/** Initializes and configures the Region and the memory to be protected
	*/
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER4;
	MPU_InitStruct.BaseAddress = 0xC0000000;
	MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
	MPU_InitStruct.SubRegionDisable = 0x0;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Enables the MPU */
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}



/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM17) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

