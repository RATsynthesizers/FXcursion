#ifndef __I2C_LCD__
#define __I2C_LCD__

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"

extern I2C_HandleTypeDef hi2c1;  // change your handler here accordingly
#define SLAVE_ADDRESS_LCD 0x4E // change this according to ur setup

void lcd_init (void);   // initialize lcd

void lcd_send_cmd (char cmd);  // send command to the lcd

void lcd_send_data (char data);  // send data to the lcd

void lcd_send_string (char *str);  // send string to the lcd

void lcd_put_cur(int row, int col);  // put cursor at the entered position row (0 or 1), col (0-15);

void lcd_clear (void);


#ifdef __cplusplus
}
#endif
#endif /*__I2C_LCD__ */
