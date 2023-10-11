/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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



// 32-sample sine wave.
// (Use this macro to adjust aplitude/volume.)
#define _AMP(x) ( x / 2 )
const size_t SINE_SAMPLES = 32;
const uint16_t SINE_WAVE[] = {
  _AMP(2048), _AMP(2447), _AMP(2831), _AMP(3185),
  _AMP(3495), _AMP(3750), _AMP(3939), _AMP(4056),
  _AMP(4095), _AMP(4056), _AMP(3939), _AMP(3750),
  _AMP(3495), _AMP(3185), _AMP(2831), _AMP(2447),
  _AMP(2048), _AMP(1649), _AMP(1265), _AMP(911),
  _AMP(601),  _AMP(346),  _AMP(157),  _AMP(40),
  _AMP(0),    _AMP(40),   _AMP(157),  _AMP(346),
  _AMP(601),  _AMP(911),  _AMP(1265), _AMP(1649)
};
// Global variable to hold the core clock speed in Hertz.
//uint32_t SystemCoreClock = 8000000;
// Simple imprecise delay method.
void __attribute__( ( optimize( "O0" ) ) )
delay_cycles( uint32_t cyc ) {
  for ( uint32_t d_i = 0; d_i < cyc; ++d_i ) { asm( "NOP" ); }
}



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */


  // Enable peripherals clocks: GPIOA, DMA, DAC, TIM6, SYSCFG.
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
  RCC->APB1LENR |= RCC_APB1LENR_DAC12EN | RCC_APB1LENR_TIM6EN;
  RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;

  // Pin A4: analog mode. (PA4 = DAC1, Channel 1)
  GPIOA->MODER    &= ~( 0x3 << ( 4 * 2 ) );
  GPIOA->MODER    |=  ( 0x3 << ( 4 * 2 ) );

  // enable & config DMA:
  DMA1_Stream0 -> PAR = (uint32_t) &(DAC1->DHR12R1);
  DMA1_Stream0 -> M0AR = ( uint32_t )&SINE_WAVE;
  DMA1_Stream0 -> NDTR = ( uint16_t )SINE_SAMPLES;
  DMA1_Stream0 -> CR = 0U;				// CLEAR
  DMA1_Stream0 -> CR |= DMA_SxCR_DIR_0 | DMA_SxCR_MINC | DMA_SxCR_PL; 	// mem to periph, mem increment, medium PrioLevel
  // Enable DMA1 Stream 0.
  // Note: the transfer won't actually start here, because
  // the DAC peripheral is not sending DMA requests yet and
  // DMAMUX is not routed from DAC req -> channel_0 req input (starts DMA stream 0).
  DMA1_Stream0 -> CR |= DMA_SxCR_EN;
  // Use DMAMux1 to route a DMA request line to the DMA channel.
  // PAGE 465 RM
  //DMAMUX1 channels 0 to 7 are connected to DMA1 channels 0 to 7
  //DMAMUX1 channels 8 to 15 are connected to DMA2 channels 0 to 7
  //DMAMUX2 channels 0 to 7 are connected to BDMA channels 0 to 7
  DMAMUX1_Channel0->CCR  = (67 << DMAMUX_CxCR_DMAREQ_ID_Pos);  	// dac_ch1_dma

  // TIM6 configuration. This timer will set the frequency
  // at which the DAC peripheral requests DMA transfers.
  // Set prescaler and autoreload for a 440Hz sine wave.
  TIM6 -> PSC = 0u;
  TIM6 -> ARR = ( SystemCoreClock / ( 440 * SINE_SAMPLES ) );
  TIM6->CR2 &= ~( TIM_CR2_MMS ); // master mode selection page 1607 RM
  TIM6->CR2 |=  ( 0x2 << TIM_CR2_MMS_Pos ); // trigger output = on Update (to trigger DAC or other things)
  TIM6->CR1 |=  ( TIM_CR1_CEN ); // Start the timer.


  // DAC configuration.
  // Set trigger sources to TIM6 TRGO (TRiGger Output).
  DAC1 -> CR &= ~(DAC_CR_TSEL1); // PAGE 1074 rm
  DAC1 -> CR |=	5; // dac_chx_trg5 for timer 6
  // Enable DAC DMA requests for dac_channel 1.
  DAC1 -> CR |= DAC_CR_DMAEN1;
  // Enable DAC channel 1.
  DAC1->CR  |=  ( DAC_CR_EN1 );
  delay_cycles( 1000 );
  // Enable DAC channel trigger.
  // The DMA stream and timer are both already on, so the
  // DMA transfer will start as soon as the DAC peripheral
  // starts making requests. The DAC peripheral will make a
  // request every time that TIM6 ticks over, but only after
  // this 'trigger enable' bit is set.
  DAC1 -> CR |= DAC_CR_TEN1;	// trigger enable


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 80;
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

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
