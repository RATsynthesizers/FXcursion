/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.25.0. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
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

#include <TouchGFXHAL.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */
#include "ili9341.h"
#include "mdma.h"
#include "cmsis_os.h"

#define DISPLAY_WIDTH   (HAL::DISPLAY_WIDTH)
#define FMC_DATA_ADDRESS 0x60800000UL


extern MDMA_HandleTypeDef hmdma_mdma_channel0_sw_0;
osSemaphoreId mdmaSemaphoreHandle;

using namespace touchgfx;

static void HAL_MDMA_XferCpltCallback(MDMA_HandleTypeDef *hmdma);
static void HAL_MDMA_XferErrorCallback(MDMA_HandleTypeDef *hmdma);



void TouchGFXHAL::initialize()
{
	// Calling parent implementation of initialize().
	//
	// To overwrite the generated implementation, omit the call to the parent function
	// and implement the needed functionality here.
	// Please note, HAL::initialize() must be called to initialize the framework.

	osSemaphoreDef(mdmaSemaphore);
	mdmaSemaphoreHandle = osSemaphoreCreate(osSemaphore(mdmaSemaphore), 1);

	HAL_MDMA_RegisterCallback(&hmdma_mdma_channel0_sw_0, HAL_MDMA_XFER_CPLT_CB_ID, HAL_MDMA_XferCpltCallback);
	HAL_MDMA_RegisterCallback(&hmdma_mdma_channel0_sw_0, HAL_MDMA_XFER_ERROR_CB_ID, HAL_MDMA_XferErrorCallback);

	TouchGFXGeneratedHAL::initialize();
}

/**
 * Gets the frame buffer address used by the TFT controller.
 *
 * @return The address of the frame buffer currently being displayed on the TFT.
 */
uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    // Calling parent implementation of getTFTFrameBuffer().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    return TouchGFXGeneratedHAL::getTFTFrameBuffer();
}

/**
 * Sets the frame buffer address used by the TFT controller.
 *
 * @param [in] address New frame buffer address.
 */
void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    // Calling parent implementation of setTFTFrameBuffer(uint16_t* address).
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::setTFTFrameBuffer(address);
}

/**
 * This function is called whenever the framework has performed a partial draw.
 *
 * @param rect The area of the screen that has been drawn, expressed in absolute coordinates.
 *
 * @see flushFrameBuffer().
 */
void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    HAL_StatusTypeDef status;

    // 1. Calculate the starting address of the dirty rectangle in SDRAM (Source)
    // Address in bytes:
    uint32_t src_addr = (uint32_t)(getClientFrameBuffer() + (rect.y * DISPLAY_WIDTH + rect.x)); // * 2 for 16-bit pixels

    // 2. Set the window commands (MANDATORY CPU STEP)
    lcdSetWindow(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);

    // 3. Configure the 2D parameters (Offsets and Counts) in the MDMA handle
    hmdma_mdma_channel0_sw_0.Init.BufferTransferLength = rect.width * 2;
    // Source Block Offset (Pitch): Skips the padding bytes in SDRAM
    hmdma_mdma_channel0_sw_0.Init.SourceBlockAddressOffset = (DISPLAY_WIDTH - rect.width) * 2;

    // ******* CRUCIAL: Re-initialize to apply offsets to the TCB *******
    if (HAL_MDMA_Init(&hmdma_mdma_channel0_sw_0) != HAL_OK)
    {
    	// Handle error: MDMA failed to init
		__NOP();
    }

    // We pass the total pixel count as BlockDataLength and 1 as BlockCount,
    // BUT the MDMA uses the pitch/offset settings configured above
    // combined with the internal TCB mechanism to execute the 2D transfer.
    status = HAL_MDMA_Start_IT(&hmdma_mdma_channel0_sw_0,
                               src_addr,
                               FMC_DATA_ADDRESS,
                               rect.width * 2,      // BlockDataLength (TDC, width)
                               rect.height);		// BlockCount (TBC, height)

    if (status != HAL_OK)
    {
        // Handle error: MDMA failed to start
        __NOP();
    }

    osSemaphoreWait(mdmaSemaphoreHandle, osWaitForever);

    // 6. Notify TouchGFX
    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
    return TouchGFXGeneratedHAL::blockCopy(dest, src, numBytes);
}

/**
 * Configures the interrupts relevant for TouchGFX. This primarily entails setting
 * the interrupt priorities for the DMA and LCD interrupts.
 */
void TouchGFXHAL::configureInterrupts()
{
    // Calling parent implementation of configureInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::configureInterrupts();
}

/**
 * Used for enabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::enableInterrupts()
{
    // Calling parent implementation of enableInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::enableInterrupts();
}

/**
 * Used for disabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::disableInterrupts()
{
    // Calling parent implementation of disableInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::disableInterrupts();
}

/**
 * Configure the LCD controller to fire interrupts at VSYNC. Called automatically
 * once TouchGFX initialization has completed.
 */
void TouchGFXHAL::enableLCDControllerInterrupt()
{
    // Calling parent implementation of enableLCDControllerInterrupt().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::enableLCDControllerInterrupt();
}

bool TouchGFXHAL::beginFrame()
{
    return TouchGFXGeneratedHAL::beginFrame();
}

void TouchGFXHAL::endFrame()
{
    TouchGFXGeneratedHAL::endFrame();
}

// Called when the MDMA transfer is fully complete
void HAL_MDMA_XferCpltCallback(MDMA_HandleTypeDef *hmdma)
{
    if (hmdma->Instance == MDMA_Channel0)
    {
        // Release the semaphore to unblock the waiting TouchGFX thread
        osSemaphoreRelease(mdmaSemaphoreHandle);
    }
}

// Optional: Implement the Error callback in case of an issue
void HAL_MDMA_XferErrorCallback(MDMA_HandleTypeDef *hmdma)
{
    if (hmdma->Instance == MDMA_Channel0)
    {
        // Handle error: abort the MDMA and still release the semaphore
        HAL_MDMA_Abort(hmdma);
        osSemaphoreRelease(mdmaSemaphoreHandle);
    }
}

/* USER CODE END TouchGFXHAL.cpp */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
