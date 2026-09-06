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

	/*
	 * DRAIN IT - this is what lets lcdSetWindow drop its osDelay(2).
	 *
	 * osSemaphoreDef expands to { 0, NULL }, so controlblock is NULL and
	 * osSemaphoreCreate takes the vSemaphoreCreateBinary path. That legacy
	 * FreeRTOS macro creates a binary semaphore ALREADY GIVEN, so without this
	 * the first wait below returned before the first transfer had even started,
	 * and every wait afterwards was acknowledging the PREVIOUS transfer instead
	 * of its own. A flush therefore returned while its own MDMA was still
	 * running, and the next lcdSetWindow retargeted the window underneath it -
	 * which is exactly the corruption the 2 ms delay was covering.
	 *
	 * Recorder.c drains its two semaphores for the same reason.
	 */
	osSemaphoreWait(mdmaSemaphoreHandle, 0);

	osSemaphoreDef(vSyncAllowedSem);
	vSyncAllowedSemHandle = osSemaphoreCreate(osSemaphore(vSyncAllowedSem), 1);

	/*
	 * This one is NOT drained, deliberately. It is a gate rather than a
	 * completion signal: HAL_GPIO_EXTI_Callback may only announce a new frame
	 * when it can take this, so it has to start available or TouchGFX would
	 * block in waitForVSync forever and nothing would ever be drawn.
	 */

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

/*
 * The MDMA buffer transfer length is a 7-bit field: the HAL asserts 1..254 and
 * the reference manual allows at most 128 bytes. Passing rect.width * 2 - up to
 * 640 for a full-width rect - overflows TLEN and spills into the bits above it.
 * The assert never fires because USE_FULL_ASSERT is commented out in
 * stm32h7xx_hal_conf.h, so the value is written unmasked.
 *
 * It happens to be tolerated today because the spill lands on PKE, which this
 * channel sets deliberately anyway, and on reserved bits. What is NOT safe is
 * the resulting TLEN: for some widths it is not a multiple of the 4-byte
 * destination data size (width 127 gives 126), which the RM requires.
 *
 * TLEN only sizes the internal buffer, so clamping changes nothing about what
 * is transferred - it just keeps the register legal for every rect width.
 */
static uint32_t mdmaBufferLength(uint16_t widthPixels)
{
    uint32_t nBytes = (uint32_t)widthPixels * 2U;

    if (nBytes > 128U)
    {
        nBytes = 128U;
    }

    /* Must be a whole number of 4-byte destination words. */
    nBytes &= ~3U;

    return (nBytes == 0U) ? 4U : nBytes;
}

/*
 * Grow a flush rect so a WORD-sized MDMA can move it.
 *
 * Channel 0 uses SourceDataSize/DestDataSize = WORD, a 4-byte data size,
 * while the framebuffer is 16bpp. The source byte offset is
 * (rect.y * DISPLAY_WIDTH + rect.x) * 2; DISPLAY_WIDTH * 2 is 640 so the y
 * term is always a multiple of 4, but the x term only is when rect.x is even.
 * An odd x hands the MDMA a source two bytes off a word boundary, and an odd
 * width hands it a block length that is not a whole number of words.
 *
 * A full-screen rect is x=0 width=320 and therefore always safe, which is why
 * changing screens repainted correctly while a widget update did not - the
 * invalidate was never the problem.
 *
 * Growing outwards costs at most one pixel column each side, and those pixels
 * already hold current framebuffer content. The panel window is set from the
 * grown rect too, so it receives exactly what is sent.
 */
static touchgfx::Rect alignRectForWordDma(const touchgfx::Rect& rect)
{
    touchgfx::Rect r = rect;
    int16_t right = (int16_t)(r.x + r.width);

    if ((r.x & 1) != 0)
    {
        r.x--;
    }

    if ((right & 1) != 0)
    {
        right++;
    }

    if (r.x < 0)
    {
        r.x = 0;
    }

    if (right > (int16_t)DISPLAY_WIDTH)
    {
        right = (int16_t)DISPLAY_WIDTH;
    }

    r.width = (int16_t)(right - r.x);

    return r;
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
	/* See alignRectForWordDma: an odd x or width is not transferable
	   by a WORD-sized MDMA, and widget rects are frequently odd. */
	const touchgfx::Rect r = alignRectForWordDma(rect);

	/*
	 * WAIT FOR DMA2D TO FINISH DRAWING BEFORE READING THE FRAMEBUFFER.
	 *
	 * TouchGFXConfiguration installs STM32DMA, so widgets are rendered by
	 * DMA2D and the blit operations complete asynchronously in its interrupt.
	 * Application::draw(Rect&) is, in the linked binary, exactly:
	 *
	 *     currentScreen->draw(rect);          // enqueues blits, does not wait
	 *     ...
	 *     HAL::flushFrameBuffer(rect);        // tail call, nothing in between
	 *
	 * and HAL::flushFrameBuffer(Rect) is two byte stores - bookkeeping, not a
	 * barrier. The framework's own drain, Application::drawCachedAreas' call
	 * to flushDMA, sits on the REFRESH_STRATEGY_OPTIM_SINGLE_BUFFER_TFT_CTRL
	 * path only; we run REFRESH_STRATEGY_DEFAULT, which skips it. That is
	 * correct for an LTDC that rescans continuously, but we PUSH the frame,
	 * so without this call the MDMA reads lines DMA2D has not written yet and
	 * sends whatever SDRAM happens to hold.
	 *
	 * Visible as the first chunk of a heavy frame arriving half-drawn: with
	 * the AddModuleWindow, its full-screen alpha-186 shadeBox is enqueued
	 * first and keeps DMA2D busy for longer than chunk 0 takes to transfer,
	 * so the two option boxes enqueued last landed after their lines had
	 * already gone out. This was the second thing the osDelay(2) in
	 * lcdSetWindow was covering - 2 ms is plenty to drain the queue.
	 *
	 * flushDMA takes the framebuffer semaphore and immediately gives it back,
	 * so it is a pure barrier with no state left behind and is safe to call
	 * on every flush. Nothing can refill the queue while we transfer: the
	 * only producer is this task, and it is here.
	 */
	flushDMA();

    // Split transfer into max 4 chunks (60 lines each for 240px display)
    const uint32_t MAX_CHUNK_HEIGHT = 120;  // Fits in 1.2ms VBlank @ DataSetup=2
    uint32_t chunks = (r.height + MAX_CHUNK_HEIGHT - 1) / MAX_CHUNK_HEIGHT;
    if (chunks > 2) chunks = 2;  // Enforce max 4 chunks

    uint32_t lines_done = 0;

    // 2. Set window for THIS CHUNK ONLY
    lcdSetWindow(r.x,
                 r.y,
                 r.x + r.width - 1,
                 r.y + r.height - 1);

    osSemaphoreWait(vSyncAllowedSemHandle, 0);

    for (uint32_t i = 0; i < chunks; i++)
    {
        uint32_t chunk_height = (lines_done + MAX_CHUNK_HEIGHT <= r.height)
                              ? MAX_CHUNK_HEIGHT
                              : (r.height - lines_done);

        while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) != GPIO_PIN_SET);

        // 3. Start MDMA for THIS CHUNK ONLY
        uint32_t src_addr = (uint32_t)(getClientFrameBuffer() +
                         (r.y + lines_done) * DISPLAY_WIDTH + r.x);

        hmdma_mdma_channel0_sw_0.Init.BufferTransferLength = mdmaBufferLength(r.width);
        hmdma_mdma_channel0_sw_0.Init.SourceBlockAddressOffset = (DISPLAY_WIDTH - r.width) * 2;
        HAL_MDMA_Init(&hmdma_mdma_channel0_sw_0);

        HAL_MDMA_Start_IT(&hmdma_mdma_channel0_sw_0,
                          src_addr,
                          FMC_DATA_ADDRESS,
                          r.width * 2,
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
