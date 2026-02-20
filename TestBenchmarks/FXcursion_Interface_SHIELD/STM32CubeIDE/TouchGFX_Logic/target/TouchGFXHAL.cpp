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
#include "touchgfx_wrapper.h"

#define DISPLAY_WIDTH   (HAL::DISPLAY_WIDTH)
#define FMC_DATA_ADDRESS 0x60800000UL


extern MDMA_HandleTypeDef hmdma_mdma_channel0_sw_0;
osSemaphoreId mdmaSemaphoreHandle;
osSemaphoreId vSyncAllowedSemHandle;

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

	osSemaphoreDef(vSyncAllowedSem);
	vSyncAllowedSemHandle = osSemaphoreCreate(osSemaphore(vSyncAllowedSem), 1);

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
	static uint16_t framesWithoutChunking = 178;

	// CRITICAL: First transfer = full white background fill
	// Skip chunking to avoid TE instability during ILI9341 startup
	if (framesWithoutChunking > 0) {
		framesWithoutChunking--;

		// SINGLE TRANSFER - no chunking, no TE waits
		uint32_t src_addr = (uint32_t)(getClientFrameBuffer() +
						 (rect.y * DISPLAY_WIDTH + rect.x));

        while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) != GPIO_PIN_SET);

		lcdSetWindow(rect.x, rect.y, rect.x + rect.width - 1, rect.y + rect.height - 1);

		hmdma_mdma_channel0_sw_0.Init.BufferTransferLength = rect.width * 2;
		hmdma_mdma_channel0_sw_0.Init.SourceBlockAddressOffset =
			(DISPLAY_WIDTH - rect.width) * 2;
		HAL_MDMA_Init(&hmdma_mdma_channel0_sw_0);

		HAL_MDMA_Start_IT(&hmdma_mdma_channel0_sw_0,
						  src_addr,
						  FMC_DATA_ADDRESS,
						  rect.width * 2,
						  rect.height);

		osSemaphoreWait(mdmaSemaphoreHandle, osWaitForever);
		TouchGFXGeneratedHAL::flushFrameBuffer(rect);
		return;
	}

    // Split transfer into max 4 chunks (60 lines each for 240px display)
    const uint32_t MAX_CHUNK_HEIGHT = 120;  // Fits in 1.2ms VBlank @ DataSetup=2
    uint32_t chunks = (rect.height + MAX_CHUNK_HEIGHT - 1) / MAX_CHUNK_HEIGHT;
    if (chunks > 2) chunks = 2;  // Enforce max 4 chunks

    uint32_t lines_done = 0;

    // 2. Set window for THIS CHUNK ONLY
    lcdSetWindow(rect.x,
                 rect.y,
                 rect.x + rect.width - 1,
                 rect.y + rect.height - 1);

    osSemaphoreWait(vSyncAllowedSemHandle, 0);

    for (uint32_t i = 0; i < chunks; i++)
    {
        uint32_t chunk_height = (lines_done + MAX_CHUNK_HEIGHT <= rect.height)
                              ? MAX_CHUNK_HEIGHT
                              : (rect.height - lines_done);

        while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) != GPIO_PIN_SET);

        // 3. Start MDMA for THIS CHUNK ONLY
        uint32_t src_addr = (uint32_t)(getClientFrameBuffer() +
                         (rect.y + lines_done) * DISPLAY_WIDTH + rect.x);

        hmdma_mdma_channel0_sw_0.Init.BufferTransferLength = rect.width * 2;
        hmdma_mdma_channel0_sw_0.Init.SourceBlockAddressOffset = (DISPLAY_WIDTH - rect.width) * 2;
        HAL_MDMA_Init(&hmdma_mdma_channel0_sw_0);

        HAL_MDMA_Start_IT(&hmdma_mdma_channel0_sw_0,
                          src_addr,
                          FMC_DATA_ADDRESS,
                          rect.width * 2,
                          chunk_height);

        osSemaphoreWait(mdmaSemaphoreHandle, osWaitForever);

        lines_done += chunk_height;
    }

    osSemaphoreRelease(vSyncAllowedSemHandle);

    // 4. Notify TouchGFX after ALL chunks complete
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
