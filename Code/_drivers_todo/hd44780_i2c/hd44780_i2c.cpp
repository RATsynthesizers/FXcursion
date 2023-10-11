/*
 * hd44780_i2c.c
 *
 *  Created on: Dec 18, 2020
 *      Author: romte
 */

#include "hd44780_i2c.h"

const unsigned char array_LCD[]={0x80, 0xC0, 0x94, 0xD4};

void LCD_com(unsigned char data)
{
LCD_send_byte (data,0x00,0x00);
}


void LCD_dat(unsigned char data)
{
LCD_send_byte (data,0x00,RS);
}


void LCD_send_byte (unsigned char data, uint8_t com, uint8_t dat)
{
uint8_t buff;
buff = data;
buff = (buff & LH)|E|com|dat|ON_BLACKLIGHT;
PCF8574AT_WriteByte(buff,HD44780_I2C_ADDR);
buff &= ~E;
PCF8574AT_WriteByte(buff,HD44780_I2C_ADDR);
//HAL_Delay(1);
delay_us(80);
buff = data;
buff = (buff <<4)|E|com|dat|ON_BLACKLIGHT;
PCF8574AT_WriteByte(buff,HD44780_I2C_ADDR);
buff &= ~E;
PCF8574AT_WriteByte(buff,HD44780_I2C_ADDR);
}


void LCD_init(void)
{

	HAL_Delay(20);
HAL_Delay(20);
LCD_com(0x33); HAL_Delay(7);
LCD_com(0x32); delay_us(200);
LCD_com(0x28); LCD_com(0x08);
 LCD_com(0x01); HAL_Delay(20);
 LCD_com(0x06);
 LCD_com(0x0D);


}


void LCD_puts(char *p)
{
	while(*p) LCD_dat(*p++);
}

void LCD_puts(int size, char *p) {
	while (size) {
		LCD_dat(*p++);
		size--;
	}
}



void LCD_gotoxy(int row, char col)
{
	LCD_com(array_LCD[row]+col);
}


void LCD_clear(void)
{
	LCD_com(0x01);
	HAL_Delay(2);
}


void PCF8574AT_WriteByte(uint8_t data, uint8_t adres)
{
//while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
//I2C_GenerateSTART(I2C1, ENABLE);
//while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
//I2C_Send7bitAddress(I2C1, adres<<1, I2C_Direction_Transmitter);
//while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
//I2C_SendData(I2C1,data);
// while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
//I2C_GenerateSTOP(I2C1, ENABLE);

HAL_I2C_Master_Transmit(&hi2c1, adres<<1, &data, 1, 1) ;

}

void delay_init(void)
{
RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
}

void delay_ms(unsigned int delay)
{
TIM2->PSC = F_APB1/1000+1;
TIM2->ARR = delay;
TIM2->EGR |= TIM_EGR_UG;
TIM2->CR1 |= TIM_CR1_CEN | TIM_CR1_OPM;
while ((TIM2->CR1&TIM_CR1_CEN)!=0);
}

void delay_us(unsigned int delay)
{
TIM2->PSC = F_APB1/1000000-1;
TIM2->ARR = delay;
TIM2->EGR |= TIM_EGR_UG;
TIM2->CR1 |= TIM_CR1_CEN | TIM_CR1_OPM;
while ((TIM2->CR1&TIM_CR1_CEN)!=0);
}
