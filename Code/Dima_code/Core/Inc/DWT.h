/*
 * DWT.h
 *
 *  Created on: Oct 22, 2020
 *      Author: romte
 */

#ifndef INC_DWT_H_
#define INC_DWT_H_

#include "main.h"

#define    DWT_CYCCNT    *(volatile uint32_t*)0xE0001004
#define    DWT_CONTROL   *(volatile uint32_t*)0xE0001000
#define    SCB_DEMCR     *(volatile uint32_t*)0xE000EDFC


//SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // ��������� ������������ DWT
//DWT_CONTROL|= DWT_CTRL_CYCCNTENA_Msk;// �������� �������
//DWT_CYCCNT = 0;// �������� �������

// code under test

//__IO uint32_t count_tic = DWT_CYCCNT;// ���-�� ������

#endif /* INC_DWT_H_ */
