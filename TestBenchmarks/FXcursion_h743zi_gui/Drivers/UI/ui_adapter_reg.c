/*
 * ui_dapter.c
 *
 *  Created on: Jan 21, 2022
 *      Author: rorka
 */

#include "ui_adapter_reg.h"

IN_RAM_D1_DMA UIadapter_reg_TypeDef UIadapterReg;

extern osMessageQueueId_t ReadWriteUIQueueHandle;

void UIadapter_Init(UIadapter_reg_TypeDef *UIadapter) {
	for (uint8_t i = 0; i < NUM_OF_UI_REGS; i++) {
		UIadapter->UIinputRegs[i] = 0xFF;
		UIadapter->UIinputRegsPrev[i] = UIadapter->UIinputRegs[i];
		UIadapter->UIoutputRegs[i] = 0x00;
		for (uint8_t j = 0; j < NUM_OF_UI_REGS; j++) {
			UIadapter->UIregUpdFlag[i][j] = 0;
			UIadapter->UIregPrevTime[i][j] = 0;
		}
	}
}

// NOT NEEDED ANYMORE, DOING READING IN TxRxCALLBACK
// TODO make non-blocking register polling
//void UIadapter_ReadWriteUI(UIadapter_reg_TypeDef *UIadapter) {
////	for (uint8_t i = 0; i < NUM_OF_UI_REGS; i++) // store old input regs states
////		UIadapter->UIinputRegsPrev[i] = UIadapter->UIinputRegs[i];
//
////	HAL_GPIO_WritePin(USER_SPI2_SW_CS_GPIO_Port, USER_SPI2_SW_CS_Pin, GPIO_PIN_SET);// pull UI software CS pin high (latch written values in hc595)
//	//SCB_CleanDCache_by_Addr((uint32_t*) UIadapter->UIoutputRegs, 3);
//
//	// --then wait until spi6 rxtx cplt callback is called, and contnue
//}

void UIadapter_ReadUI_spi2_callback(UIadapter_reg_TypeDef *UIadapter) {
	SCB_InvalidateDCache_by_Addr((uint32_t*) UIadapter->UIinputRegs, 3);

//	HAL_GPIO_WritePin(USER_SPI2_SW_CS_GPIO_Port, USER_SPI2_SW_CS_Pin, GPIO_PIN_RESET);// pull UI software CS pin low (load hc165 with values)

	for (uint8_t i = 0; i < NUM_OF_UI_REGS; i++) {
		uint8_t result = UIadapter->UIinputRegs[i]
				^ UIadapter->UIinputRegsPrev[i];
		if (result) {
			__NOP();
			for (uint8_t j = 0; j < 8; j++)
				if ((result >> j) % 2
						&& (HAL_GetTick() - UIadapter->UIregPrevTime[i][j] > 25)) {
					struct READWRITEUIQUEUE_OBJ_t ReadWriteUI_msg;
					UIadapter->UIregPrevTime[i][j] = HAL_GetTick();
					ReadWriteUI_msg.reg = i;
					ReadWriteUI_msg.bit = j;
					osMessageQueuePut(ReadWriteUIQueueHandle, &ReadWriteUI_msg, 0U, 0U);
				}
		}
		UIadapter->UIinputRegsPrev[i] = UIadapter->UIinputRegs[i];
	}
}

void getBtnHWdata(UIadapter_reg_TypeDef *UIadapter, UiBtnData_TypeDef *btnData) {
	btnData->btn = (UIadapter->UIinputRegs[(btnData->btnBit) / 8]
			& (1 << ((btnData->btnBit) % 8))); // mask reg data and store btn pin state
}

void setLedHWdata(UIadapter_reg_TypeDef *UIadapter,
		UiRGBLedData_TypeDef *ledData) {
	if (ledData->RBit)
		UIadapter->UIoutputRegs[ledData->RByteNum] |= (1 << ledData->RBitNum);
	else
		UIadapter->UIoutputRegs[ledData->RByteNum] &= ~((uint8_t) (1
				<< ledData->RBitNum));

	if (ledData->GBit)
		UIadapter->UIoutputRegs[ledData->GByteNum] |= (1 << ledData->GBitNum);
	else
		UIadapter->UIoutputRegs[ledData->GByteNum] &= ~((uint8_t) (1
				<< ledData->GBitNum));

	if (ledData->BBit)
		UIadapter->UIoutputRegs[ledData->BByteNum] |= (1 << ledData->BBitNum);
	else
		UIadapter->UIoutputRegs[ledData->BByteNum] &= ~((uint8_t) (1
				<< ledData->BBitNum));

}

void getEncHWdata(UIadapter_reg_TypeDef *UIadapter, UiEncData_TypeDef *encData) {
	encData->encA = (UIadapter->UIinputRegs[encData->AByteNum]
			& (1 << (encData->ABitNum)));
	encData->encB = (UIadapter->UIinputRegs[encData->BByteNum]
			& (1 << (encData->BBitNum)));
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if(hspi->Instance == SPI2)
	{
		UIadapter_ReadUI_spi2_callback(&UIadapterReg);
		HAL_SPI_TransmitReceive_DMA(&(UI_ADAPTER_SPI), UIadapterReg.UIoutputRegs,
						UIadapterReg.UIinputRegs, 3);
	}
}
