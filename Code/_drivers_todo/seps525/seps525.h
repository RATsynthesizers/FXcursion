/*
 * seps525.h
 *
 *  Created on: Feb 14, 2022
 *      Author: romte
 */


#ifndef HW_SEPS525_H_
#define HW_SEPS525_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "spi.h"


#define SEND_CMD	1   // 4-wire - Display instruction (command)
#define SEND_DAT	2   // 4-wire - Display instruction (data)

#define CS_REL		1	// reload CS pin
#define CS_NREL		0	// not reload CS pin

#define MAXROWS     128
#define MAXCOLS     160

void CS_Low(void);
void CS_High(void);
void Set_CMD(void);
void Set_DAT(void);
void RESET_Low(void);
void RESET_High(void);

void displaySend_(uint8_t sendType, uint8_t data, uint8_t reload);
void displaySend(uint8_t sendType, uint8_t data);

void Set_Column_Address(uint16_t a, uint16_t b);
void Set_Row_Address(uint16_t a, uint16_t b);
void Set_Memory_Access_Pointer(uint16_t a, uint16_t b);

void Set_Write_RAM();
void Reset_SEPS525();

void DrawRandomRect();
void DrawRandomDot();
void DrawRect(uint32_t color, uint16_t colMin, uint16_t colMax, uint16_t rowMin, uint16_t rowMax);


#ifdef __cplusplus
}
#endif

#endif /* HW_SEPS525_H_ */
