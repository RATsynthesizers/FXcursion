/*
 * ili9341.h
 *
 *  Created on: Aug 3, 2020
 *      Author: romte
 */

// datasheet pdf
// https://cdn-shop.adafruit.com/datasheets/ILI9341.pdf
#ifndef INC_ILI9341_H_
#define INC_ILI9341_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
//#include "stm32f4xx_hal.h"
#include "main.h"

#define ADDR_CMD        *(uint8_t*)0x60000000
#define ADDR_DATA       *(uint8_t*)0x60400000

#define swap_(a,b)    {int16_t t=a;a=b;b=t;}
#define RESET_ACTIVE HAL_GPIO_WritePin(USER_LCD_RESET_GPIO_Port, USER_LCD_RESET_Pin,   GPIO_PIN_RESET);
#define RESET_IDLE   HAL_GPIO_WritePin(USER_LCD_RESET_GPIO_Port, USER_LCD_RESET_Pin,   GPIO_PIN_SET);
#define BLACK        0x0000
#define BLUE         0x001F
#define RED          0x0F800
#define GREEN        0x07E0
#define CYAN         0x07FF
#define MAGENTA      0xF81F
#define YELLOW       0xFFE0
#define WHITE        0xFFFF


//void tftDelay(uint32_t dly);
//__STATIC_INLINE void tftDelayMicro(volatile uint32_t micros);

//void tftWriteCmd(unsigned char cmd);
//void tftWriteData(unsigned char data);
uint32_t ili9341_ReadReg(uint8_t reg);

uint16_t ili9341_GetRandColor(void);

void ili9341_Reset(void);
void ili9341_Init(void);

void ili9341_SetRotation(unsigned char r);
void ili9341_Flood(uint16_t color, uint32_t len);
void ili9341_SetAddrWindow(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2);

void ili9341_DrawBitmap_8bit(const uint8_t * colors, uint16_t w,uint16_t h);
void ili9341_FillScreen(uint16_t color);
void ili9341_FillRectangle(uint16_t color, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void ili9341_DrawPixel(int x, int y, uint16_t color);
void ili9341_DrawLine(uint16_t color, uint16_t x1, uint16_t y1, uint16_t x2,	uint16_t y2);
void ili9341_DrawRect(uint16_t color, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void ili9341_DrawCircle(uint16_t x0, uint16_t y0, int r, uint16_t color);

void ili9341_FillNoise(void);
void ili9341_Test_BlinkRandColor_3000(void);
void ili9341_SpeedTest(void);
void ili9341_DrawRandomLine(void);
void ili9341_DrawRandomCircle(void);

#ifdef __cplusplus
}
#endif


#endif /* INC_ILI9341_H_ */
