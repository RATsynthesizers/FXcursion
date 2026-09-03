/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "mdma.h"
#include "quadspi.h"
#include "sai.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "audio_sys.h"
#include "audio_io.h"

#include "w9812g6jh.h"
#include "wm8731.h"

#include "ctrl_uart.h"

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  BOOLEAN bLoopMemOk;


  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_MDMA_Init();
  MX_FMC_Init();
  MX_SAI1_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* No MX_QUADSPI_Init: there is no QSPI part on this board any more. Loop
   * audio moved into the two SDRAM banks - see loop_mem.h. Turn QUADSPI off in
   * the .ioc so a regeneration does not put this call back. */
  MX_I2C1_Init();
  MX_I2S3_Init();
  MX_SAI2_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* ==========================================================================
   * AUDIO BRING-UP. THE ORDER HERE IS NOT COSMETIC.
   *
   * 1. Both SDRAM banks, and both BEFORE AudioSys_Init.
   *
   *    AudioSys_Init resets every effect, and those resets write into the delay
   *    lines in bank 1 and the reverb memory in bank 2. Running it against
   *    uninitialised SDRAM writes into a controller that has not been through
   *    its power-up sequence.
   *
   *    Each bank is initialised separately on purpose - see w9812g6jh.h on why
   *    a Load Mode Register command issued to both at once uses bank 1 timings.
   *
   *    Both banks now also carry a looper, so this ordering matters more than
   *    it used to: LoopMem_Init clears its windows during AudioSys_Init, and
   *    the buffers those windows read from live in these banks.
   *
   * 2. Codecs before AudioIO_Start. They are clock SLAVES: they need to be
   *    configured and out of reset before SAI1 starts driving MCLK and FS at
   *    them, or the first frames land on a device still in its default state.
   *
   * 3. AudioIO_Start absolutely last. It starts the clock, and from that moment
   *    the audio interrupt is live and the engine must already be initialised.
   * ======================================================================== */

  W9812G6JH_InitBank(&hsdram1, FMC_SDRAM_CMD_TARGET_BANK1);
  W9812G6JH_InitBank(&hsdram2, FMC_SDRAM_CMD_TARGET_BANK2);

  /* Loop audio lives in those two banks now, so prove they answer before the
   * engine starts writing takes into them. Destructive, which is why it runs
   * here and nowhere else: after this point the buffers hold real audio.
   *
   * Not fatal. Everything except the looper still works, and PROTO_DIAG carries
   * bLoopMemOk so the interface can say so rather than leaving the player to
   * discover it by pressing record. */
  bLoopMemOk = (W9812G6JH_SelfTest(SDRAM_BANK_ADDR)  == RESULT_OK) &&
               (W9812G6JH_SelfTest(SDRAM_BANK2_ADDR) == RESULT_OK);

  (void)WM8731_Init(&hi2c1, USER_CODEC_GPIO_GPIO_Port, USER_CODEC_GPIO_Pin);

  if (WM8731_Start() != RESULT_OK)
  {
    Error_Handler();
  }

  if (AudioSys_Init() != RESULT_OK)
  {
    Error_Handler();
  }

  (void)AudioIO_Init();

  if (AudioIO_Start() != RESULT_OK)
  {
    Error_Handler();
  }

  /* Last, and deliberately after the audio is already running: the interface
   * may send a configuration the instant the link comes up, and Grid_Apply has
   * to have a live engine to hand it to. */
  if (CtrlUart_Init(bLoopMemOk) != RESULT_OK)
  {
    Error_Handler();
  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* Restarts the streams if a converter reported an error. Everything else
     * the audio path needs happens in the SAI1_A interrupt. */
    AudioIO_Service();

    /* The entire control link: drain the receiver, dispatch whatever arrived,
     * and send telemetry when it is due. None of it runs in the audio ISR. */
    CtrlUart_Service();

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

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
  /* --------------------------------------------------------------------------
   * CORE CLOCK SOURCE - HSI, deliberately.
   *
   * There is no HSE on this board: PH0/PH1 are unconnected and the only
   * oscillator, Y1 (24.576 MHz), goes to I2S_CKIN for the audio path.
   *
   * HSI is +/-1%, which affects the core clock, UART baud (inside budget at
   * 115200) and cycle-based timing. It does NOT affect audio: every audio clock
   * comes from the crystal - see PeriphCommonClock_Config.
   * ------------------------------------------------------------------------ */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV4;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 120;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
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

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  /* --------------------------------------------------------------------------
   * EVERY AUDIO CLOCK COMES FROM THE 24.576 MHz CRYSTAL (Y1 -> I2S_CKIN, PC9).
   *
   * This is not a preference, it is required. The MCU has no HSE on this board,
   * so the PLLs run from HSI, which is +/-1% and drifts with temperature. A SAI
   * clocked from an HSI-derived PLL would be 48 kHz +/-480 ppm and would slide
   * against the two codecs that ARE on the crystal - audible drift, then
   * periodic glitches.
   *
   * 24.576 MHz = 512 x 48 kHz, so every divider below is exact:
   *     MCLK  = 24.576 / 2  = 12.288 MHz  = 256 x Fs   (what the WM8731 wants)
   *     BCLK  = 64 x Fs     =  3.072 MHz               (2 slots x 32-bit)
   *
   * SPI123 NO LONGER NEEDS THE CRYSTAL.
   *
   * SPI1, SPI2 and SPI3/I2S3 share one kernel selection (D2CCIP1R.SPI123SEL),
   * and this note used to read "I2S3 must have the crystal, so SPI1 gets it
   * too" - capping the board-to-board link at 24.576/2 = 12.288 Mbit/s, half of
   * which the recorder stream alone consumes.
   *
   * I2S3 is now a SLAVE, taking CK and WS from SAI1 block A (see i2s.c). It
   * derives nothing from the kernel, so the whole SPI123 domain is free to run
   * from a PLL, and SPI1 can carry loop transport alongside the recorder.
   *
   * The audio path is unaffected: SAI1 and SAI23 keep _PIN, so every sample
   * clock in the system still comes from Y1. The headphone codec is now on the
   * same clock EDGE as the other two rather than merely the same frequency.
   *
   * PLL3P is 192 MHz so SPI1's minimum /2 prescaler lands exactly on 96 MHz -
   * see spi_tp_cfg.h for why 96 and not 48.
   *
   *     HSI 64 MHz / PLL3M 8 = 8 MHz  x PLL3N 96 = 768 MHz VCO  / PLL3P 4
   *
   * HSI-derived is fine HERE and nowhere else on this board: the SPI link is
   * synchronous - the slave samples the clock it is given - so the +/-1% of an
   * untrimmed HSI costs a little throughput and nothing else. It would still be
   * unacceptable for anything that has to stay in step with the converters.
   * ------------------------------------------------------------------------ */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1|RCC_PERIPHCLK_SAI2
                              |RCC_PERIPHCLK_SPI3|RCC_PERIPHCLK_SPI2
                              |RCC_PERIPHCLK_SPI1;
  PeriphClkInitStruct.PLL3.PLL3M = 8;
  PeriphClkInitStruct.PLL3.PLL3N = 96;
  PeriphClkInitStruct.PLL3.PLL3P = 4;
  PeriphClkInitStruct.PLL3.PLL3Q = 4;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.Sai1ClockSelection  = RCC_SAI1CLKSOURCE_PIN;
  PeriphClkInitStruct.Sai23ClockSelection = RCC_SAI23CLKSOURCE_PIN;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL3;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /* ==========================================================================
   * Region 0 - RAM_D2, the audio DMA buffers. NON-CACHEABLE.
   *
   * As generated this region covered 8 KiB at 0x24000000, which was wrong twice
   * over: nothing DMA related was placed there, and the 8 KiB window landed in
   * the middle of the modulation delay lines, leaving part of them cacheable
   * and part not.
   *
   * The audio buffers live in RAM_D2 because DMA1 and DMA2 are D2 masters and
   * cannot reach the TCMs at all. Nothing else is placed in RAM_D2, so the
   * whole region can be marked non-cacheable and no cache maintenance is needed
   * around any transfer.
   * ======================================================================== */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* ==========================================================================
   * Region 1 - BOTH SDRAM banks. Write-through, no write allocate.
   *
   * 512 MiB, not the generated 256 MiB. 256 MiB reaches 0xCFFFFFFF and stops,
   * which covers the delay lines in bank 1 and leaves the reverb in bank 2 at
   * 0xD0000000 on the default write-back write-allocate mapping - a different
   * cache policy for the two halves of the same design, by accident.
   *
   * Write-through with no write allocate is deliberate: delay and reverb lines
   * are streamed with almost no reuse, so write-allocate would buy a
   * read-for-ownership on data that is never read back.
   * ======================================================================== */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512MB;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x38000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
