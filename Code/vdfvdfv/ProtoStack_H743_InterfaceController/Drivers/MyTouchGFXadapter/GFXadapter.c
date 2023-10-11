/*
 * GFXadapter.c
 *
 *  Created on: Dec 3, 2021
 *      Author: rorka
 */

#include "../ili9341/ili9341.h"
#include "GFXadapter.h"
#include "main.h"

extern void DisplayDriver_TransferCompleteCallback(void);

static uint8_t isTransmittingData = 0;

int touchgfxDisplayDriverTransmitActive(void) {
	return isTransmittingData;
}

void touchgfxDisplayDriverTransmitBlock(const uint8_t* pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	isTransmittingData = 1;
	ili9341_SetAddrWindow(x, y, x+w-1, y+h-1);
	ili9341_DrawBitmap_8bit(pixels, w, h);
	isTransmittingData = 0;
	DisplayDriver_TransferCompleteCallback();
}
