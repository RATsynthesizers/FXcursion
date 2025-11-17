
#include "main.h"
#include "UISurvey.hpp"

extern "C" {
// Get RTOS CMSIS-OS interface
#include "cmsis_os.h"
// Get native header
#include "Init.h"
}

#ifdef RELEASE
const char btldr_vtable[] __attribute__ ((section(".btldr_vtable"), used)) =
{
    0xFF
};
const char rom_marker[] __attribute__ ((section(".rom_marker"), used)) =
{
    0xFF
};
const char empty_memory[] __attribute__ ((section(".empty_memory"), used)) =
{
    0xFF
};
const char btldr_code[] __attribute__ ((section(".btldr_code"), used)) =
{
    0xFF
};
#endif

int main(void)
{
    init_all();
    UISurveyInit();

    /* Start scheduler */
    osKernelStart();

    while (1)
    {

    }
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



