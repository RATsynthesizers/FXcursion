/*
 * ssd1305_i2c.h
 *
 *  Created on: Aug 21, 2022
 *      Author: romte
 */

#ifndef HW_SSD1305_I2C_SSD1305_I2C_H_
#define HW_SSD1305_I2C_SSD1305_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>  // uintXX_t support
#include "main.h"
//#include "spi.h"

/////////////  my S S D 1 3 0 5    settings   ////////////////
#define SSD1305_HEIGHT 32
#define SSD1305_WIDTH 128
#define SSD1305_ADDR_ (0x78>>1) // 0x7A  shift for hal i2c
#define SSD1305_I2C		hi2c2	// i2c instance to use
//////////////////////////////////////////////////////////////

#define SSD1305_ADDRESS 		(SSD1305_ADDR_ << 1) // for hal i2c
#define SSD1305_CONTINUE_BIT 	(1<<7)
#define SSD1305_NOCONTINUE_BIT 	(0<<7)
#define SSD1305_DATA_BIT		(1<<6) // set -> data,
#define SSD1305_CMD_BIT			(0<<6) // reset -> cmd

/////////////  S S D 1 3 0 5    R E G S   ////////////////
#define BLACK 0
#define WHITE 1

#define SSD1305_SETLOWCOLUMN 0x00
#define SSD1305_SETHIGHCOLUMN 0x10
#define SSD1305_MEMORYMODE 0x20
#define SSD1305_SETCOLADDR 0x21
#define SSD1305_SETPAGEADDR 0x22
#define SSD1305_SETSTARTLINE 0x40

#define SSD1305_SETCONTRAST 0x81
#define SSD1305_SETBRIGHTNESS 0x82

#define SSD1305_SETLUT 0x91

#define SSD1305_SEGREMAP 0xA0
#define SSD1305_DISPLAYALLON_RESUME 0xA4
#define SSD1305_DISPLAYALLON 0xA5
#define SSD1305_NORMALDISPLAY 0xA6
#define SSD1305_INVERTDISPLAY 0xA7
#define SSD1305_SETMULTIPLEX 0xA8
#define SSD1305_DISPLAYDIM 0xAC
#define SSD1305_MASTERCONFIG 0xAD
#define SSD1305_DISPLAYOFF 0xAE
#define SSD1305_DISPLAYON 0xAF

#define SSD1305_SETPAGESTART 0xB0

#define SSD1305_COMSCANINC 0xC0
#define SSD1305_COMSCANDEC 0xC8
#define SSD1305_SETDISPLAYOFFSET 0xD3
#define SSD1305_SETDISPLAYCLOCKDIV 0xD5
#define SSD1305_SETAREACOLOR 0xD8
#define SSD1305_SETPRECHARGE 0xD9
#define SSD1305_SETCOMPINS 0xDA
#define SSD1305_SETVCOMLEVEL 0xDB

/////////////  M A I N    F U N C   /////////////

void oled_command(uint8_t c);
void oled_commandList(uint8_t *c, uint8_t n);
void oled_data(uint8_t* pData, uint16_t size);

void ssd1305_Init(void);

#define grayoled_swap(a, b)                                                    \
  (((a) ^= (b)), ((b) ^= (a)), ((a) ^= (b))) ///< No-temp-var swap operation

void drawPixel(int16_t x, int16_t y, uint16_t color);
void drawBitmap();
void getPixel(int16_t x, int16_t y);

void Set_Page_Address(uint8_t add);
void Set_Column_Address(uint8_t add);
void Display_Picture(uint8_t* pic);



#ifdef __cplusplus
}
#endif

#endif /* HW_SSD1305_I2C_SSD1305_I2C_H_ */




