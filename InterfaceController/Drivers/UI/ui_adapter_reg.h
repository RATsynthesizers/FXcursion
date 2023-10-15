/*
 * ui_pinmap.hpp
 *
 *  Created on: Jan 20, 2022
 *      Author: rorka
 */

#ifndef HW_UI_ADAPTER_H_
#define HW_UI_ADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "MemRegions.h"
#include "spi.h"
#include "stdbool.h"
#include "cmsis_os.h"

#define NUM_OF_UI_REGS 3
#define DEBOUNCE_CNT 2

typedef enum {
	SINGLE, COLOR
} UI_Led_Types;

typedef enum {
	ACTIVE_LOW, ACTIVE_HIGH
} UI_Polarity_Types;

typedef struct {
	uint8_t btnBit;
	bool btn;
	UI_Polarity_Types polarity;
} UiBtnData_TypeDef;

// LED hardware data bits
typedef struct {
	uint8_t RByteNum;
	uint8_t RBitNum;
	bool RBit; // red color bit for RGB
	uint8_t GByteNum;
	uint8_t GBitNum;
	bool GBit; // grn color bit for RGB
	uint8_t BByteNum;
	uint8_t BBitNum;
	bool BBit; // blu color bit for RGB
	UI_Polarity_Types polarity;
} UiRGBLedData_TypeDef;

typedef struct {
	uint8_t AByteNum;
	uint8_t ABitNum;
	uint8_t BByteNum;
	uint8_t BBitNum;
	bool encA;
	bool encB;
	UI_Polarity_Types polarity;
} UiEncData_TypeDef;

typedef struct {
	int8_t UIregUpdFlag[NUM_OF_UI_REGS][8];						// got new data from UI flag
	uint32_t UIregPrevTime[NUM_OF_UI_REGS][8];
	uint8_t UIinputRegs[NUM_OF_UI_REGS];
	uint8_t UIinputRegsPrev[NUM_OF_UI_REGS];	// previous input reg data
	uint8_t UIoutputRegs[NUM_OF_UI_REGS];
} UIadapter_reg_TypeDef;

extern UIadapter_reg_TypeDef UIadapterReg;

void UIadapter_Init(UIadapter_reg_TypeDef *UIadapter);
//void UIadapter_ReadWriteUI(UIadapter_reg_TypeDef *UIadapter);// read pins 1-st part
void UIadapter_ReadUI_spi2_callback(UIadapter_reg_TypeDef *UIadapter);	// read pins 2-nd part

void getBtnHWdata(UIadapter_reg_TypeDef *UIadapter, UiBtnData_TypeDef *btnData);
void setLedHWdata(UIadapter_reg_TypeDef *UIadapter, UiRGBLedData_TypeDef *ledData);
void getEncHWdata(UIadapter_reg_TypeDef *UIadapter, UiEncData_TypeDef *encData);

#ifdef __cplusplus
}
#endif

#endif /* HW_UI_ADAPTER_H_ */

