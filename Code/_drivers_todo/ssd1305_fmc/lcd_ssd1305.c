/*
 * lcd_ssd1305.c
 *
 *  Created on: Jan 16, 2022
 *      Author: rorka
 */

#include "lcd_ssd1305.h"
#include "ssd1305_splash.h"

  uint8_t page_offset = 0;
  uint8_t column_offset = 0;

//  void oled_command(uint8_t c) {
//      uint8_t buf[2] = {0x00, c}; // Co = 0, D/C = 0
//      i2c_dev->write(buf, 2);
//  }

//  oled_commandList(const uint8_t *c, uint8_t n) {
//	    uint8_t dc_byte = 0x00; // Co = 0, D/C = 0
//	    i2c_dev->write((uint8_t *)c, n, true, &dc_byte, 1);
//  }

  void CS_Low()		{SPI2_OLED_CS_GPIO_Port->BSRR = (uint32_t)SPI2_OLED_CS_Pin << 16U;}
  void CS_High()		{SPI2_OLED_CS_GPIO_Port->BSRR =  SPI2_OLED_CS_Pin;}
  void Set_CMD()		{SPI2_OLED_CMD_GPIO_Port->BSRR = (uint32_t)SPI2_OLED_CMD_Pin << 16U;}
  void Set_DAT()		{SPI2_OLED_CMD_GPIO_Port->BSRR = SPI2_OLED_CMD_Pin;}
  void RESET_Low()	{SPI2_OLED_RESET_GPIO_Port->BSRR = (uint32_t)SPI2_OLED_RESET_Pin <<16U;}
  void RESET_High()	{SPI2_OLED_RESET_GPIO_Port->BSRR = SPI2_OLED_RESET_Pin;}

void LCD_CmdWrite(const uint8_t Command) {
//	  LCD->LCD_REG = Command;
//	  HAL_Delay(1);

	Set_CMD();
	CS_Low();
	HAL_SPI_Transmit(&hspi2, &Command, 1, 1);
	CS_High();
}

void LCD_CmdWriteCmdArray(const uint8_t* CommandArray, uint8_t size) {
	for(uint8_t i = 0; i < size; i++) {
		LCD_CmdWrite(*(CommandArray + i));
//		LCD->LCD_REG = *(CommandArray + i);
//		HAL_Delay(1);
	}
}


void LCD_DataWrite(const uint8_t Data) {
//	LCD->LCD_RAM = Data;
	Set_DAT();
	CS_Low();
	HAL_SPI_Transmit(&hspi2, &Data, 1, 1);
	CS_High();
}


void LCD_WriteReg(const uint8_t LCD_Reg, const uint16_t LCD_RegValue)
{
	LCD->LCD_REG = LCD_Reg;
	LCD->LCD_RAM = LCD_RegValue;
}


void ssd1305_Init(void) {
  // Init sequence, make sure its under 32 bytes, or split into multiples!
  static const uint8_t init_128x32[] = {
      // Init sequence for 128x32 OLED module
      SSD1305_DISPLAYOFF,          // 0xAE
      SSD1305_SETDISPLAYCLOCKDIV,
      0x10,//0xA8,//0xF0,
      SSD1305_SETMULTIPLEX,
	  0x1F,//0x3F,
      SSD1305_SETDISPLAYOFFSET,
      0x00,//0x40,
      SSD1305_SETSTARTLINE | 0x0,  // line #0
      SSD1305_MASTERCONFIG,
      0x8E,
      SSD1305_SETAREACOLOR,
      0x05,
      SSD1305_SEGREMAP | 0x01,
      SSD1305_COMSCANDEC,
      SSD1305_SETCOMPINS,
      0x12, 						// 0xDA, 0x12
      SSD1305_SETLUT,
      0x3F,
      0x3F,
      0x3F,
      0x3F,
      SSD1305_SETCONTRAST,
      0xBF,//0x32, 						// 0x81, 0x32
      SSD1305_SETPRECHARGE,
      0xD2,//0xF1, 						// 0xd9, 0xF1
	  SSD1305_SETVCOMLEVEL,
	  0x08,
	  SSD1305_DISPLAYALLON_RESUME,
      SSD1305_NORMALDISPLAY, 		// 0xA6

	  SSD1305_MEMORYMODE,
	  0x00

//      SSD1305_SETLOWCOLUMN | 0x0,  // low col = 0
//      SSD1305_SETHIGHCOLUMN | 0x0, // hi col = 0
//      0x2E,                        // ??
//      SSD1305_SETBRIGHTNESS,
//      0x10, 						// 0x82, 0x10


};


  static const uint8_t init_128x64[] = {
      // Init sequence for 128x64 OLED module
      SSD1305_DISPLAYOFF,          // 0xAE
      SSD1305_SETLOWCOLUMN | 0x4,  // low col = 0
      SSD1305_SETHIGHCOLUMN | 0x4, // hi col = 0
      SSD1305_SETSTARTLINE | 0x0,  // line #0
      0x2E,                        //??
      SSD1305_SETCONTRAST,
      0x32, // 0x81, 0x32
      SSD1305_SETBRIGHTNESS,
      0x80, // 0x82, 0x80
      SSD1305_SEGREMAP | 0x01,
      SSD1305_NORMALDISPLAY, // 0xA6
      SSD1305_SETMULTIPLEX,
      0x3F, // 0xA8, 0x3F (1/64)
      SSD1305_MASTERCONFIG,
      0x8E, /* external vcc supply */
      SSD1305_COMSCANDEC,
      SSD1305_SETDISPLAYOFFSET,
      0x40, // 0xD3, 0x40
      SSD1305_SETDISPLAYCLOCKDIV,
      0xF0, // 0xD5, 0xF0
      SSD1305_SETAREACOLOR,
      0x05,
      SSD1305_SETPRECHARGE,
      0xF1, // 0xd9, 0xF1
      SSD1305_SETCOMPINS,
      0x12, // 0xDA, 0x12
      SSD1305_SETLUT,
      0x3F,
      0x3F,
      0x3F,
      0x3F};

  if (SSD1305_HEIGHT == 32) {
    page_offset = 4;
    column_offset = 4;
    LCD_CmdWriteCmdArray(init_128x32, sizeof(init_128x32));
  } else {
    // 128x64 high
    page_offset = 0;
    LCD_CmdWriteCmdArray(init_128x64, sizeof(init_128x64));
    }
//  LCD_WriteReg(SSD1305_SETCONTRAST, 0x2F);

  HAL_Delay(100);                      		// 100ms delay recommended
  LCD_CmdWrite(SSD1305_DISPLAYON); 			// 0xaf

  LCD_CmdWrite(SSD1305_DISPLAYALLON);

//  setContrast(0x2F);
  for(int i = 0; i < (132*64); i++)
	  LCD_DataWrite(0x00);
  //drawBitmap((SSD1305_HEIGHT - splash2_width) / 2, (SSD1305_HEIGHT - splash2_height) / 2, splash2_data, splash2_width, splash2_height, 1);
}



//void drawPixel(int16_t x, int16_t y, uint16_t color) {
//  if ((x >= 0) && (x < width()) && (y >= 0) && (y < height())) {
//    // Pixel is in-bounds. Rotate coordinates if needed.
////    switch (getRotation()) {
////    case 1:
////      grayoled_swap(x, y);
////      x = WIDTH - x - 1;
////      break;
////    case 2:
//      x = SSD1305_WIDTH - x - 1;
//      y = SSD1305_HEIGHT - y - 1;
////      break;
////    case 3:
////      grayoled_swap(x, y);
////      y = HEIGHT - y - 1;
////      break;
////    }
//
//    // adjust dirty window
////    window_x1 = min(window_x1, x);
////    window_y1 = min(window_y1, y);
////    window_x2 = max(window_x2, x);
////    window_y2 = max(window_y2, y);
//
////    if (_bpp == 1) {
////      switch (color) {
////      case MONOOLED_WHITE:
////        buffer[x + (y / 8) * WIDTH] |= (1 << (y & 7));
////        break;
////      case MONOOLED_BLACK:
////        buffer[x + (y / 8) * WIDTH] &= ~(1 << (y & 7));
////        break;
////      case MONOOLED_INVERSE:
////        buffer[x + (y / 8) * WIDTH] ^= (1 << (y & 7));
////        break;
////      }
////    }
////    if (_bpp == 4) {
//      uint8_t *pixelptr = &buffer[x / 2 + (y * WIDTH / 2)];
//      // Serial.printf("(%d, %d) -> offset %d\n", x, y, x/2 + (y * WIDTH / 2));
//      if (x % 2 == 0) { // even, left nibble
//        uint8_t t = pixelptr[0] & 0x0F;
//        t |= (color & 0xF) << 4;
//        pixelptr[0] = t;
//      } else { // odd, right lower nibble
//        uint8_t t = pixelptr[0] & 0xF0;
//        t |= color & 0xF;
//        pixelptr[0] = t;
//      }
////    }
//  }
//}

