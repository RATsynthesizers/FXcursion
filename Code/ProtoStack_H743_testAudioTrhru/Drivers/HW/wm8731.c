/*
 * wm8731.c
 *
 *  Created on: Jul 31, 2020
 *      Author: romte
 */

#include "wm8731.h"

void CodecRegWrite(uint16_t regAddr, uint16_t regData, uint8_t codecAddr) {
	regAddr = (regAddr << 1) | ((regData >> 8) & 0x01);
	uint8_t regData_ = regData & 0xFF;
	while (HAL_OK != HAL_I2C_Mem_Write(&hi2c4, codecAddr, regAddr,
	CODEC_MEMADD_SIZE, &regData_, 1, CODEC_I2C_TIMEOUT))
		;
}

void SetLineInGain(int gain, uint8_t codecAddr) {
//	if ((gain <= 12) && (gain >= -34.5)) TODO
//		gain = gain + 34.5
	CodecRegWrite(CODEC_REG_LEFT_LINE_IN, gain, codecAddr);
	CodecRegWrite(CODEC_REG_RIGHT_LINE_IN, gain, codecAddr);
}

// +6dB to -73dB in 1dB steps
void SetHPGain(int gain, uint8_t codecAddr) {
	if ((gain <= 6) && (gain >= -73))
		gain = gain + 73 + 0x30;  // 0011 0000 = 0x30 = -73dB
	else if (gain > 6)
		gain = 0x7F;              // +6dB
	else
		gain = 0;                 // MUTE
	CodecRegWrite(CODEC_REG_LEFT_HEADPHONES_OUT, gain, codecAddr);
	CodecRegWrite(CODEC_REG_RIGHT_HEADPHONES_OUT, gain, codecAddr);
}

void CodecPowerUp(uint8_t codecAddr) {
	// Power up analog & digital 3v3 LDO for codec
	HAL_Delay(100);
	// Codec reset
	CodecRegWrite(CODEC_REG_RESET, 0, codecAddr);

	//enable all exept OUTPUT
	CodecRegWrite(CODEC_REG_POWER_MANAGEMENT, CODEC_POWER_DOWN_LINE_OUT,
			codecAddr);

	// Configure L&R inputs
	CodecRegWrite(CODEC_REG_LEFT_LINE_IN, CODEC_INPUT_0_DB, codecAddr);
	CodecRegWrite(CODEC_REG_RIGHT_LINE_IN, CODEC_INPUT_0_DB, codecAddr);

	// Configure L&R headphone outputs
	CodecRegWrite(CODEC_REG_LEFT_HEADPHONES_OUT, CODEC_HEADPHONES_0_DB,
			codecAddr);
	CodecRegWrite(CODEC_REG_RIGHT_HEADPHONES_OUT, CODEC_HEADPHONES_0_DB,
			codecAddr);

	// Configure analog routing
	CodecRegWrite(
	CODEC_REG_ANALOGUE_ROUTING,
	CODEC_MIC_MUTE | CODEC_ADC_LINE | CODEC_OUTPUT_DAC_ENABLE , codecAddr); //| CODEC_OUTPUT_FROM_LINE);

	// Configure digital routing
	CodecRegWrite(CODEC_REG_DIGITAL_ROUTING, CODEC_DEEMPHASIS_48K, codecAddr);


	if (codecAddr == CODEC_ADDRESS_0) {
		// Configure digital audio format & LR swap          //// if  codec master -> CODEC_MCLK_DIV2 from 24 to 12 & enable osc
		CodecRegWrite(CODEC_REG_DIGITAL_FORMAT,	CODEC_PROTOCOL_MASK_PHILIPS | CODEC_FORMAT_MASK_16_BIT | CODEC_FORMAT_MASTER, codecAddr);
		// Configure sample rate
		CodecRegWrite(CODEC_REG_SAMPLE_RATE, CODEC_RATE_48K_48K | CODEC_MCLK_DIV2 /*| CODEC_CLKO_DIV2*/, codecAddr);
		// Configure power management
		CodecRegWrite(CODEC_REG_POWER_MANAGEMENT,
		CODEC_POWER_DOWN_MIC, codecAddr); //| CODEC_POWER_DOWN_CLOCK_OUTPUT| CODEC_POWER_DOWN_OSCILLATOR);
	}	else {
		// Configure digital audio format & LR swap
		CodecRegWrite(CODEC_REG_DIGITAL_FORMAT,	CODEC_PROTOCOL_MASK_PHILIPS | CODEC_FORMAT_MASK_16_BIT | CODEC_FORMAT_SLAVE, codecAddr);
		// Configure sample rate
		CodecRegWrite(CODEC_REG_SAMPLE_RATE,CODEC_RATE_48K_48K/* | CODEC_MCLK_DIV2 | CODEC_CLKO_DIV2*/,	codecAddr);
		// Configure power management
		CodecRegWrite(CODEC_REG_POWER_MANAGEMENT,
		CODEC_POWER_DOWN_MIC | CODEC_POWER_DOWN_CLOCK_OUTPUT  | CODEC_POWER_DOWN_OSCILLATOR, codecAddr);
	}


	// Now codec is active.
	CodecRegWrite(CODEC_REG_ACTIVE, CODEC_ACTIVE, codecAddr);
}

void CodecPowerDown(uint8_t codecAddr) {
	// Write power down register and power down LINE_OUT
	CodecRegWrite(CODEC_REG_POWER_MANAGEMENT,
			CODEC_POWER_DOWN_MIC | CODEC_POWER_DOWN_CLOCK_OUTPUT
					| CODEC_POWER_DOWN_OSCILLATOR, codecAddr);
	// Power down analog & digital 3v3 LDO for codec
//	HAL_GPIO_WritePin(GPIOE, CODEC_nSHDN_Pin, GPIO_PIN_RESET);
}

