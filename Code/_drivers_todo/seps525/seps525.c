/*
 * seps525.c
 *
 *  Created on: Feb 14, 2022
 *      Author: romte
 */


//--------------------------------------------------------------------------
/*
SEPS525-based display demo code by Martin Falatic
Portions (c)2014 Mike LaVine - Newhaven Display International, LLC.
http://www.newhavendisplay.com/NHD_forum/index.php/topic,64.0.html
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "seps525.h"

__IO uint8_t spi2Buf;

void CS_Low()		{SPI2_OLED_CS_GPIO_Port->BSRR = (uint32_t)SPI2_OLED_CS_Pin << 16U;}
void CS_High()		{SPI2_OLED_CS_GPIO_Port->BSRR =  SPI2_OLED_CS_Pin;}
void Set_CMD()		{SPI2_OLED_CMD_GPIO_Port->BSRR = (uint32_t)SPI2_OLED_CMD_Pin << 16U;}
void Set_DAT()		{SPI2_OLED_CMD_GPIO_Port->BSRR = SPI2_OLED_CMD_Pin;}
void RESET_Low()	{SPI2_OLED_RESET_GPIO_Port->BSRR = (uint32_t)SPI2_OLED_RESET_Pin <<16U;}
void RESET_High()	{SPI2_OLED_RESET_GPIO_Port->BSRR = SPI2_OLED_RESET_Pin;}
//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
void displaySend(uint8_t sendType, uint8_t data) {

	CS_Low();
	if (sendType == SEND_CMD)
		// Send a command value
		Set_CMD();
	else if (sendType == SEND_DAT)
		// Send a data value
		Set_DAT();
	spi2Buf = data;
	HAL_SPI_Transmit(&hspi2, &spi2Buf, 1, 2);
	CS_High();
	HAL_Delay(1);
}

void displaySend_(uint8_t sendType, uint8_t data, uint8_t reload) {
	CS_Low();
	if (sendType == SEND_CMD)
		// Send a command value
		Set_CMD();
	else if (sendType == SEND_DAT)
		// Send a data value
		Set_DAT();
	spi2Buf = data;
	HAL_SPI_Transmit(&hspi2, &spi2Buf, 1, 2);
	if(reload) {
		CS_High();
//		HAL_Delay(1);
	}
	HAL_Delay(1);
}

//--------------------------------------------------------------------------
void Set_Column_Address(uint16_t a, uint16_t b)
{
  displaySend(SEND_CMD, 0x17); // Set Column Min
  displaySend(SEND_DAT, a);    //   Default => 0x00
  displaySend(SEND_CMD, 0x18); // Set Column Max
  displaySend(SEND_DAT, b);    //   Default => 0x9F
}

//--------------------------------------------------------------------------
void Set_Row_Address(uint16_t a, uint16_t b)
{
  displaySend(SEND_CMD, 0x19); // Set Row Min
  displaySend(SEND_DAT, a);    //   Default => 0x00
  displaySend(SEND_CMD, 0x1A); // Set Row Max
  displaySend(SEND_DAT, b);    //   Default => 0x7F
}

//--------------------------------------------------------------------------
void Set_Memory_Access_Pointer(uint16_t a, uint16_t b)
{
  displaySend(SEND_CMD, 0x20); // Set Column Ptr
  displaySend(SEND_DAT, a);    //   Default => 0x00
  displaySend(SEND_CMD, 0x21); // Set Row Ptr
  displaySend(SEND_DAT, b);    //   Default => 0x00
}
//--------------------------------------------------------------------------
void Set_Write_RAM()
{
  displaySend_(SEND_CMD, 0x22, CS_NREL); // Enable MCU to Write into RAM
}

//--------------------------------------------------------------------------
void Reset_SEPS525()
{
	 HAL_Delay(100);
	RESET_Low();
	HAL_Delay(2);
	RESET_High();
	HAL_Delay(1);
	 HAL_Delay(100);


//  // Directly from the specs
//  displaySend(SEND_CMD, 0x04); //
//  displaySend(SEND_DAT, 0x03); // = 0x01
//  HAL_Delay(2);
//
//  displaySend(SEND_CMD, 0x04); //
//  displaySend(SEND_DAT, 0x04); // = orig = 0x00 (0x04 saves power)
//  HAL_Delay(2);
//
//  displaySend(SEND_CMD, 0x3B); //
//  displaySend(SEND_DAT, 0x00); // =
//
//  displaySend(SEND_CMD, 0x02); //
//  displaySend(SEND_DAT, 0x01); // =
//
//  displaySend(SEND_CMD, 0x03); //
//  displaySend(SEND_DAT, 0x90); // =
//
//  displaySend(SEND_CMD, 0x80); //
//  displaySend(SEND_DAT, 0x01); // =
//
//  displaySend(SEND_CMD, 0x08); //
//  displaySend(SEND_DAT, 0x04); // =
//  displaySend(SEND_CMD, 0x09); //
//  displaySend(SEND_DAT, 0x05); // =
//  displaySend(SEND_CMD, 0x0A); //
//  displaySend(SEND_DAT, 0x05); // =
//
//  displaySend(SEND_CMD, 0x0B); //
//  displaySend(SEND_DAT, 0x9D); // =
//  displaySend(SEND_CMD, 0x0C); //
//  displaySend(SEND_DAT, 0x8C); // =
//  displaySend(SEND_CMD, 0x0D); //
//  displaySend(SEND_DAT, 0x57); // =
//
//  displaySend(SEND_CMD, 0x10); //
//  displaySend(SEND_DAT, 0x56); // =
//  displaySend(SEND_CMD, 0x11); //
//  displaySend(SEND_DAT, 0x4D); // =
//  displaySend(SEND_CMD, 0x12); //
//  displaySend(SEND_DAT, 0x46); // =
//
//  displaySend(SEND_CMD, 0x13); //
//  displaySend(SEND_DAT, 0x00); // = orig 0x0A - BIG difference!
//
//  displaySend(SEND_CMD, 0x14); //
//  displaySend(SEND_DAT, 0x01); // = orig 0x01 - appears irrelevant in 4-wire mode
//  displaySend(SEND_CMD, 0x16); //
//  displaySend(SEND_DAT, 0x66); // = orig 0x76 = 3-bytes 262K colors, 0x66 = 2-bytes (65K colors)
//
//  displaySend(SEND_CMD, 0x20); //
//  displaySend(SEND_DAT, 0x00); // =
//  displaySend(SEND_CMD, 0x21); //
//  displaySend(SEND_DAT, 0x00); // =
//
//  displaySend(SEND_CMD, 0x28); //
//  displaySend(SEND_DAT, 0x7F); // =
//
//  displaySend(SEND_CMD, 0x29); //
//  displaySend(SEND_DAT, 0x00); // =
//
//  displaySend(SEND_CMD, 0x06); //
//  displaySend(SEND_DAT, 0x01); // =
//
//  displaySend(SEND_CMD, 0x05); //
//  displaySend(SEND_DAT, 0x00); // =
//
//  displaySend(SEND_CMD, 0x15); //
//  displaySend(SEND_DAT, 0x00); // =


	 // display off, analog reset
	displaySend(SEND_CMD,0x04);
	 displaySend(SEND_DAT, 0x01);
		HAL_Delay(1);

	 // normal mode
	 displaySend(SEND_CMD,0x04);
	 displaySend(SEND_DAT, 0x00);
		HAL_Delay(1);

	 // display off
	 displaySend(SEND_CMD,0x06);
	 displaySend(SEND_DAT, 0x00);
		HAL_Delay(1);

	 // turn on internal oscillator using external resistor
	 displaySend(SEND_CMD,0x02);
	 displaySend(SEND_DAT, 0x01);

	 // 90 hz frame rate, divider 0
	 displaySend(SEND_CMD,0x03);
	 displaySend(SEND_DAT, 0x30);

	 // duty cycle 127
	 displaySend(SEND_CMD,0x28);
	 displaySend(SEND_DAT, 0x7F);

	 // start on line 0
	 displaySend(SEND_CMD,0x29);
	 displaySend(SEND_DAT, 0x00);

	 // rgb_if
	 displaySend(SEND_CMD,0x14);
	 displaySend(SEND_DAT, 0x31);

	 // Set Memory Write Mode
	 displaySend(SEND_CMD,0x16);
	 displaySend(SEND_DAT, 0x76);


	 // driving current r g b (uA)
	 displaySend(SEND_CMD,0x10);
	 displaySend(SEND_DAT, 0x45);
	 displaySend(SEND_CMD,0x11);
	 displaySend(SEND_DAT, 0x34);
	 displaySend(SEND_CMD,0x12);
	 displaySend(SEND_DAT, 0x33);

	 // precharge time r g b
	 displaySend(SEND_CMD,0x08);
	 displaySend(SEND_DAT, 0x04);
	 displaySend(SEND_CMD,0x09);
	 displaySend(SEND_DAT, 0x05);
	 displaySend(SEND_CMD,0x0A);
	 displaySend(SEND_DAT, 0x05);

	 // precharge current r g b (uA)
	 displaySend(SEND_CMD,0x0B);
	 displaySend(SEND_DAT, 0x9D);
	 displaySend(SEND_CMD,0x0C);
	 displaySend(SEND_DAT, 0x8C);
	 displaySend(SEND_CMD,0x0D);
	 displaySend(SEND_DAT, 0x57);

	 // Set Reference Voltage Controlled by External Resister
	 displaySend(SEND_CMD,0x80);
	 displaySend(SEND_DAT, 0x00);

	 // mode set
	 displaySend(SEND_CMD,0x13);
	 displaySend(SEND_DAT, 0xA0);

	 Set_Column_Address(0, 159);
	 Set_Row_Address(0, 127);
	 // Display On
	 displaySend(SEND_CMD,0x06);
	 displaySend(SEND_DAT, 0x01);

  HAL_Delay(10);
}


//--------------------------------------------------------------------------
void DrawRandomRect()
{
  uint16_t colMin = random(0,      MAXCOLS);
  uint16_t colMax = random(colMin, MAXCOLS);
  uint16_t rowMin = random(0,      MAXROWS);
  uint16_t rowMax = random(rowMin, MAXROWS);
  uint32_t color  = random(0x3FFFFL+1);
  DrawRect(color, colMin, colMax, rowMin, rowMax);
}

//--------------------------------------------------------------------------
void DrawRandomDot()
{
  uint16_t colMin = random(0,      MAXCOLS);
  uint16_t colMax = colMin;
  uint16_t rowMin = random(0,      MAXROWS);
  uint16_t rowMax = rowMin;
  uint32_t color  = random(0x3FFFFL+1);
  DrawRect(color, colMin, colMax, rowMin, rowMax);
}

//--------------------------------------------------------------------------
void DrawRect(uint32_t color, uint16_t colMin, uint16_t colMax, uint16_t rowMin, uint16_t rowMax)
{
//  Serial.print(colMin);
//  Serial.print(", ");
//  Serial.print(colMax);
//  Serial.print("    ");
//  Serial.print(rowMin);
//  Serial.print(", ");
//  Serial.print(rowMax);
//  Serial.println();

  Set_Column_Address(colMin,colMax);
  Set_Row_Address(rowMin,rowMax);
  Set_Memory_Access_Pointer(colMin,rowMin);
  Set_Write_RAM();

  uint16_t i, j;
  for(i=0;i<=(rowMax-rowMin);i++) // decrementing didn't make a huge difference
  {
    for(j=0;j<=(colMax-colMin);j++)
    {
      if (1) { // color mode selection?
        displaySend_(SEND_DAT, (color&0x0FF00)>> 8, CS_NREL);
        displaySend_(SEND_DAT, (color&0x000FF)>> 0, CS_REL);
      }
      else {
        // none of these works right: // can't seem to do 3-byte colors
        // displaySend(SEND_DAT, color, true, 16);
        // displaySend(SEND_DAT, (color&0x30000)>>16, false, 2);
        // displaySend(SEND_DAT, (color&0x0FF00)>> 8, false);
        // displaySend(SEND_DAT, (color&0x000FF)>> 0, true);
      }
    }
  }
}

//--------------------------------------------------------------------------
//##########################################################################
//--------------------------------------------------------------------------
//void setup()
//{
//  Reset_SEPS525();
//  DrawRect(0x000000L, 0, MAXCOLS-1, 0, MAXROWS-1);
//}
//
////--------------------------------------------------------------------------
//void loop()
//{
//  DrawRect(0x000000L, 0, MAXCOLS-1, 0, MAXROWS-1);
//  delay(500);
//  for (int i = 0; i < 0x00FF; i++) {
//    DrawRandomRect();
//  }
//  delay(1000);
//
//  DrawRect(0x000000L, 0, MAXCOLS-1, 0, MAXROWS-1);
//  delay(500);
//  for (int i = 0; i < 0x1FFF; i++) {
//    DrawRandomDot();
//  }
//  delay(1000);
//
//}

//--------------------------------------------------------------------------

