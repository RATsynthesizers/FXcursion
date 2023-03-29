/*
 * ili9341.c
 *
 *  Created on: Aug 3, 2020
 *      Author: romte
 */
// TODO: fuck tft delays

#include "ili9341.h"

uint16_t X_SIZE = 320;
uint16_t Y_SIZE = 240;
uint32_t ili9341_ID  = 0;

extern RNG_HandleTypeDef hrng; // for rand numbers (RNG peripherial)

typedef enum {
	ROTATE_0,
	ROTATE_90,
	ROTATE_180,
	ROTATE_270
} LCD_Horizontal_t;



void ili9341_Delay(uint32_t dly) {
	uint32_t i;
	for (i = 0; i < dly; i++);
}


static inline void ili9341_DelayMicro(volatile uint32_t micros) {
	micros *= (SystemCoreClock / 1000000) / 5 ;
	while (micros--);
}


void ili9341_WriteCmd(unsigned char cmd) {
	ADDR_CMD = cmd;
}


void ili9341_WriteData(unsigned char data) {
	ADDR_DATA = data;
}



uint32_t ili9341_ReadReg(uint8_t reg) {
	uint32_t reading;
	uint8_t tmp8;
	ili9341_WriteCmd(reg);
//	ili9341_DelayMicro(50);
	tmp8 = ADDR_DATA;
	reading = tmp8;
	reading <<= 8;
//	ili9341_DelayMicro(1);
	tmp8 = ADDR_DATA;
	reading |= tmp8;
	reading <<= 8;
//	ili9341_DelayMicro(1);
	tmp8 = ADDR_DATA;
	reading |= tmp8;
	reading <<= 8;
//	ili9341_DelayMicro(1);
	tmp8 = ADDR_DATA;
	reading |= tmp8;
	// 0xEF returns more data
	if (reg == 0xEF) {
		reading <<= 8;
//		ili9341_DelayMicro(5);
		tmp8 = ADDR_DATA;
		reading |= tmp8;
	}
//	ili9341_DelayMicro(150); //stabilization time
	return reading;
}

static void ili9341_Direction(LCD_Horizontal_t direction)
{
	switch (direction) {
	case ROTATE_0:
		ili9341_WriteCmd(0x36);
		ili9341_WriteData(0x48);
		break;
	case ROTATE_90:
		ili9341_WriteCmd(0x36);
		ili9341_WriteData(0x28);
		break;
	case ROTATE_180:
		ili9341_WriteCmd(0x36);
		ili9341_WriteData(0x88);
		break;
	case ROTATE_270:
		ili9341_WriteCmd(0x36);
		ili9341_WriteData(0xE8);
		break;
	}
}

void ili9341_SetRotation(unsigned char r) {
	ili9341_WriteCmd(0x36);
	switch (r) {
	case 0:
		ili9341_WriteData(0x48);
		X_SIZE = 240;
		Y_SIZE = 320;
		break;
	case 1:
		ili9341_WriteData(0x28);
		X_SIZE = 320;
		Y_SIZE = 240;
		break;
	case 2:
		ili9341_WriteData(0x88);
		X_SIZE = 240;
		Y_SIZE = 320;
		break;
	case 3:
		ili9341_WriteData(0xE8);
		X_SIZE = 320;
		Y_SIZE = 240;
		break;
	}
}


void ili9341_Reset(void) {
	RESET_ACTIVE;
	ili9341_DelayMicro(200);
	RESET_IDLE;
	ili9341_WriteCmd(0x01); //Software Reset
	for (uint8_t i = 0; i < 3; i++)
		ili9341_WriteData(0xFF);
	ili9341_DelayMicro(1000);
}


void __attribute__((optimize("O0"))) ili9341_Init(void) {
	RESET_IDLE;
	HAL_Delay(1);
	ili9341_Reset();
	ili9341_ID = ili9341_ReadReg(0xD3);   // read ID4


	ili9341_WriteCmd(0x01);  //Software Reset
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xCB);  //Power Control A
	ili9341_WriteData(0x39);
	ili9341_WriteData(0x2C);
	ili9341_WriteData(0x00);
	ili9341_WriteData(0x34);
	ili9341_WriteData(0x02);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xCF);  //Power Control B
	ili9341_WriteData(0x00);
	ili9341_WriteData(0xC1);
	ili9341_WriteData(0x30);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xE8);  //Driver timing control A
	ili9341_WriteData(0x85);
	ili9341_WriteData(0x00);
	ili9341_WriteData(0x78);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xEA);  //Driver timing control B
	ili9341_WriteData(0x00);
	ili9341_WriteData(0x00);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xED);  //Power on Sequence control
	ili9341_WriteData(0x64);
	ili9341_WriteData(0x03);
	ili9341_WriteData(0x12);
	ili9341_WriteData(0x81);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xF7);  //Pump ratio control
	ili9341_WriteData(0x20);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xC0);  //Power Control 1
	ili9341_WriteData(0x10);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xC1);  //Power Control 2
	ili9341_WriteData(0x10);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xC5);  //VCOM Control 1
	ili9341_WriteData(0x3E);
	ili9341_WriteData(0x28);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xC7);  //VCOM Control 2
	ili9341_WriteData(0x86);
	ili9341_DelayMicro(1);
	ili9341_SetRotation(1);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0x3A);  //Pixel Format Set
	ili9341_WriteData(0x55);     //16bit
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xB1);
	ili9341_WriteData(0x00);
	ili9341_WriteData(0x18);     // FPS 79 ��
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xB6);  //Display Function Control
	ili9341_WriteData(0x08);
	ili9341_WriteData(0x82);
	ili9341_WriteData(0x27);     //320 rows
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xF2);  //Enable 3G (what is it?)
	ili9341_WriteData(0x00);     //Turn it off
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0x26);  //Gamma set
	ili9341_WriteData(0x01);     //Gamma Curve (G2.2)
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xE0);  //Positive Gamma  Correction
	ili9341_WriteData(0x0F);
	ili9341_WriteData(0x31);
	ili9341_WriteData(0x2B);
	ili9341_WriteData(0x0C);
	ili9341_WriteData(0x0E);
	ili9341_WriteData(0x08);
	ili9341_WriteData(0x4E);
	ili9341_WriteData(0xF1);
	ili9341_WriteData(0x37);
	ili9341_WriteData(0x07);
	ili9341_WriteData(0x10);
	ili9341_WriteData(0x03);
	ili9341_WriteData(0x0E);
	ili9341_WriteData(0x09);
	ili9341_WriteData(0x00);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0xE1);  //Negative Gamma  Correction
	ili9341_WriteData(0x00);
	ili9341_WriteData(0x0E);
	ili9341_WriteData(0x14);
	ili9341_WriteData(0x03);
	ili9341_WriteData(0x11);
	ili9341_WriteData(0x07);
	ili9341_WriteData(0x31);
	ili9341_WriteData(0xC1);
	ili9341_WriteData(0x48);
	ili9341_WriteData(0x08);
	ili9341_WriteData(0x0F);
	ili9341_WriteData(0x0C);
	ili9341_WriteData(0x31);
	ili9341_WriteData(0x36);
	ili9341_WriteData(0x0F);
	ili9341_DelayMicro(1);
	ili9341_WriteCmd(0x11);  //Exit sleep mode
	HAL_Delay(120);
	ili9341_WriteCmd(0x29);  //turn on display
	ili9341_WriteData(0x2C);

	ili9341_Direction(ROTATE_90);
	HAL_Delay(150);
}



void ili9341_Flood(uint16_t color, uint32_t len) {
	ili9341_WriteCmd(0x2C);
	uint8_t hi = color >> 8, lo = color;
	for(uint32_t i=0;i<len;i++) {
		ili9341_WriteData(hi);
		ili9341_WriteData(lo);
	}
//	ili9341_WriteData(hi);
//	ili9341_WriteData(lo);
//	len--;
//	uint8_t i;
//	uint16_t blocks;
//	blocks = (uint16_t) (len / 64);  //64 pixels/block
//	while (blocks--)
//	{
//		i = 16;
//		do {
//			ili9341_WriteData(hi);
//			ili9341_WriteData(lo);
//			ili9341_WriteData(hi);
//			ili9341_WriteData(lo);
//			ili9341_WriteData(hi);
//			ili9341_WriteData(lo);
//			ili9341_WriteData(hi);
//			ili9341_WriteData(lo);
//		} while (--i);
//	}
//	//Fill any remaining pixels(1 to 64)
//	for (i = (uint8_t) len & 63; i--;) {
//		ili9341_WriteData(hi);
//		ili9341_WriteData(lo);
//	}
}


void ili9341_SetAddrWindow(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2) {
  ili9341_WriteCmd(0x2A);//Column Addres Set
  ili9341_WriteData(x1 >> 8);
  ili9341_WriteData(x1 & 0x00FF);
  ili9341_WriteData(x2 >> 8);
  ili9341_WriteData(x2 & 0x00FF);
//  ili9341_DelayMicro(1);
  ili9341_WriteCmd(0x2B);//Page Addres Set
  ili9341_WriteData(y1 >> 8);
  ili9341_WriteData(y1 & 0x00FF);
  ili9341_WriteData(y2 >> 8);
  ili9341_WriteData(y2 & 0x00FF);
//        ili9341_DelayMicro(1);
}



void ili9341_FillScreen(uint16_t color) {
        ili9341_SetAddrWindow(0,0,X_SIZE-1,Y_SIZE-1);
        ili9341_Flood(color,(long)X_SIZE*(long)Y_SIZE);
}

// resort for MSB first
static void ConvHL_8bit(uint8_t *pix, int32_t len)
{
	uint8_t v;
	while (len > 0) {
		v = *(pix+1);
		*(pix+1) = *pix;
		*pix = v;
		pix += 2;
		len -= 2;
	}
}

void ili9341_DrawBitmap_8bit(const uint8_t * colors, uint16_t w,uint16_t h) {
	ili9341_WriteCmd(0x2C);
	uint32_t len = w*h*2;
	ConvHL_8bit(colors, len);

	for(uint32_t i=0;i<len;i++) {
		ili9341_WriteData(*(colors+i));
	}
}

//uint16_t ili9341_GetRandColor(void) {
//	return HAL_RNG_GetRandomNumber(&hrng) & 0x0000FFFF;
//}

void ili9341_Test_BlinkRandColor_3000(void) {
	for (int i = 0; i < 3000; i++) {
		ili9341_FillScreen(ili9341_GetRandColor());
		//HAL_Delay(1);
	}
	ili9341_FillScreen(BLACK);
	HAL_Delay(500);
}

void ili9341_FillRectangle(uint16_t color, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	ili9341_SetAddrWindow(x1, y1, x2, y2);
	ili9341_Flood(color, (uint16_t) (x2 - x1 + 1) * (uint16_t) (y2 - y1 + 1));
}


void ili9341_SpeedTest(void) {
	while (1) {
		for (int i = 1; i <= 240; i++)
			ili9341_FillRectangle(WHITE, 0, 0, i, i);
//		for (int i = 240; i > 1; i--)
//			ili9341_FillRectangle(ili9341_GetRandColor(), 0, 0, i, i);
	}
}


void ili9341_DrawPixel(int x, int y, uint16_t color) {
        if((x<0)||(y<0)||(x>=X_SIZE)||(y>=Y_SIZE)) return;
        ili9341_SetAddrWindow(x, y, x, y);
        ili9341_WriteCmd(0x2C);
    ili9341_WriteData(color >> 8);
    ili9341_WriteData(color & 0xFF);
}


//void ili9341_FillNoise(void) {
//	for (int i = 0; i < 15000; i++)
//		ili9341_DrawPixel(HAL_RNG_GenerateRandomNumber(&hrng) % 240,
//				HAL_RNG_GeRandomNumber(&hrng) % 320, ili9341_GetRandColor());
//}


void ili9341_DrawLine(uint16_t color, uint16_t x1, uint16_t y1, uint16_t x2,	uint16_t y2) {
	int steep = abs(y2 - y1) > abs(x2 - x1);
	if (steep) {
		swap(x1, y1);
		swap(x2, y2);
	}
	if (x1 > x2) {
		swap(x1, x2);
		swap(y1, y2);
	}
	int dx, dy;
	dx = x2 - x1;
	dy = abs(y2 - y1);
	int err = dx / 2;
	int ystep;
	if (y1 < y2)
		ystep = 1;
	else
		ystep = -1;
	for (; x1 <= x2; x1++) {
		if (steep)
			ili9341_DrawPixel(y1, x1, color);
		else
			ili9341_DrawPixel(x1, y1, color);
		err -= dy;
		if (err < 0) {
			y1 += ystep;
			err += dx;
		}
	}
}


//void ili9341_DrawRandomLine(void) {
//	ili9341_DrawLine(ili9341_GetRandColor(), HAL_RNG_GetRandomNumber(&hrng) % 240,
//			HAL_RNG_GetRandomNumber(&hrng) % 320,
//			HAL_RNG_GetRandomNumber(&hrng) % 240,
//			HAL_RNG_GetRandomNumber(&hrng) % 320);
//}


void ili9341_DrawRect(uint16_t color, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	ili9341_DrawLine(color, x1, y1, x2, y1);
	ili9341_DrawLine(color, x2, y1, x2, y2);
	ili9341_DrawLine(color, x1, y1, x1, y2);
	ili9341_DrawLine(color, x1, y2, x2, y2);
}


void ili9341_DrawCircle(uint16_t x0, uint16_t y0, int r, uint16_t color) {
	int f = 1 - r;
	int ddF_x = 1;
	int ddF_y = -2 * r;
	int x = 0;
	int y = r;
	ili9341_DrawPixel(x0, y0 + r, color);
	ili9341_DrawPixel(x0, y0 - r, color);
	ili9341_DrawPixel(x0 + r, y0, color);
	ili9341_DrawPixel(x0 - r, y0, color);
	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		ili9341_DrawPixel(x0 + x, y0 + y, color);
		ili9341_DrawPixel(x0 - x, y0 + y, color);
		ili9341_DrawPixel(x0 + x, y0 - y, color);
		ili9341_DrawPixel(x0 - x, y0 - y, color);
		ili9341_DrawPixel(x0 + y, y0 + x, color);
		ili9341_DrawPixel(x0 - y, y0 + x, color);
		ili9341_DrawPixel(x0 + y, y0 - x, color);
		ili9341_DrawPixel(x0 - y, y0 - x, color);
	}
}


//void ili9341_DrawRandomCircle(void) {
//	ili9341_DrawCircle(HAL_RNG_GetRandomNumber(&hrng) % 200 + 20,
//			HAL_RNG_GetRandomNumber(&hrng) % 280 + 20, HAL_RNG_GetRandomNumber(&hrng) % 50, ili9341_GetRandColor());
//}
