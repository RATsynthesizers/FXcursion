/*
 * hd44780.h
 *
 *  Created on: Dec 18, 2020
 *      Author: romte
 */

// EXAMPLE
//	delay_init();
//	LCD_init();
//	LCD_com(0x0C);
//	LCD_gotoxy(1, 0);
//	char a[32];
//	LCD_puts(sprintf(a,"sooka it works"), a);

#ifndef HW_HD44780_I2C_H_
#define HW_HD44780_I2C_H_

#include "i2c.h"
//#include "main.h"

#define F_APB1 45000000L
#define HD44780_I2C_ADDR 0x27 // 0x3f for pcf8574at

#define RS 0x01
#define RW 0x02
#define E 0x004
#define LH 0xF0
#define ON_BLACKLIGHT 0x08

void PCF8574AT_WriteByte(uint8_t data,uint8_t adress);
void delay_init(void);
void delay_us(unsigned int delay);

void LCD_init(void);
void LCD_send_byte (unsigned char data, uint8_t com, uint8_t dat);
void LCD_com(unsigned char data);
void LCD_dat(unsigned char data);
void LCD_puts(char *p);
void LCD_puts(int size, char *p);
void LCD_gotoxy(int row, char col);
void LCD_clear(void);
void LCD_own_symbols(void);
void LCD_hor_graph(float value, char row);
void LCD_vert_graph(char value, char row, char col);


#endif /* HW_HD44780_I2C_H_ */
