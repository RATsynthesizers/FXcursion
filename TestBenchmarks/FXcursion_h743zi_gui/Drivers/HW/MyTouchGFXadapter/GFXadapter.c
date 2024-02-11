/*
 * GFXadapter.c
 *
 *  Created on: Dec 3, 2021
 *      Author: rorka
 */

#include "GFXadapter.h"


uint16_t* framebuffer = (uint16_t*) (SDRAM_BANK1_ADDR);  //16 bpp framebuffer

extern void DisplayDriver_TransferCompleteCallback(void);

static uint8_t isTransmittingData = 0;

int touchgfxDisplayDriverTransmitActive(void) {
	return isTransmittingData;
}

void touchgfxDisplayDriverTransmitBlock(uint8_t* pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
	isTransmittingData = 1;
	lcdSetWindow(x, y, x+w-1, y+h-1);
	lcdDrawBitmap((uint16_t*)pixels, w, h);
	isTransmittingData = 0;
	DisplayDriver_TransferCompleteCallback();
}
