/*
 * w9812g6jh.c
 *
 *  Created on: 30 ???. 2020 ?.
 *      Author: Predtech
 */

#include "w9812g6jh.h"

/* static: these used to be external symbols that any translation unit could
 * collide with. */
static FMC_SDRAM_CommandTypeDef command;

static HAL_StatusTypeDef hal_stat;

void W9812G6JH_Init(SDRAM_HandleTypeDef *hsdram) {

	W9812G6JH_InitBank(hsdram, FMC_SDRAM_CMD_TARGET_BANK1);
}

void W9812G6JH_InitBank(SDRAM_HandleTypeDef *hsdram, uint32_t nTarget) {

	__IO uint32_t tmpmrd = 0;
	command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
	command.CommandTarget = nTarget;
	command.AutoRefreshNumber = 1;
	command.ModeRegisterDefinition = 0;
	hal_stat = HAL_SDRAM_SendCommand(hsdram, &command, SDRAM_TIMEOUT);
	HAL_Delay(1);

	command.CommandMode = FMC_SDRAM_CMD_PALL;
	command.CommandTarget = nTarget;
	command.AutoRefreshNumber = 1;
	command.ModeRegisterDefinition = 0;
	hal_stat = HAL_SDRAM_SendCommand(hsdram, &command, SDRAM_TIMEOUT);
	HAL_Delay(1);

	command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
	command.CommandTarget = nTarget;
	command.AutoRefreshNumber = 8;
	command.ModeRegisterDefinition = 0;
	hal_stat = HAL_SDRAM_SendCommand(hsdram, &command, SDRAM_TIMEOUT);
	HAL_Delay(1);

	tmpmrd = (uint32_t) SDRAM_MODEREG_BURST_LENGTH_1 |
						SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
						SDRAM_MODEREG_CAS_LATENCY_2 |
						SDRAM_MODEREG_OPERATING_MODE_STANDARD |
						SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	command.CommandTarget = nTarget;
	command.AutoRefreshNumber = 1;
	command.ModeRegisterDefinition = tmpmrd;
	hal_stat = HAL_SDRAM_SendCommand(hsdram, &command, SDRAM_TIMEOUT);
	HAL_Delay(1);

	// Riddle  COUNT = (SDRAM refresh period ⁄ Number of rows) – 20
	hsdram->Instance->SDRTR |= ((uint32_t)((SDRAM_RFR_COUNT - 1)<< 1));
}


STD_RESULT W9812G6JH_SelfTest(uint32_t nBase)
{
	__IO uint32_t* const pMem = (__IO uint32_t*)nBase;
	uint32_t             nOfs;
	uint8_t              nBit;

	/* ---- data bus: walking ones at the base ---------------------------------
	   Catches a data line stuck, open, or shorted to its neighbour. One address,
	   so an address fault cannot mask a data fault here. */
	for (nBit = 0U; nBit < 32U; nBit++)
	{
		const uint32_t nPattern = (uint32_t)1UL << nBit;

		pMem[0] = nPattern;

		if (pMem[0] != nPattern)
		{
			return RESULT_NOT_OK;
		}
	}

	/* ---- address bus: one unique value per power-of-two offset ---------------
	   THE ONE THAT MATTERS FOR A LOOPER. A swapped or stuck address line makes
	   two different addresses the same cell, so a loop long enough to reach the
	   aliased offset quietly overwrites its own beginning - which sounds like a
	   looper bug and is not one.

	   Word offsets, so the loop stops at the bank size in bytes. */
	for (nOfs = 1UL; (nOfs * 4UL) < (uint32_t)SDRAM_BANK_SIZE_BYTES; nOfs <<= 1U)
	{
		pMem[nOfs] = nOfs;
	}

	/* Base last, so it is not one of the values a stuck line could satisfy. */
	pMem[0] = 0xAA55AA55UL;

	for (nOfs = 1UL; (nOfs * 4UL) < (uint32_t)SDRAM_BANK_SIZE_BYTES; nOfs <<= 1U)
	{
		if (pMem[nOfs] != nOfs)
		{
			return RESULT_NOT_OK;
		}
	}

	if (pMem[0] != 0xAA55AA55UL)
	{
		return RESULT_NOT_OK;
	}

	return RESULT_OK;
}
