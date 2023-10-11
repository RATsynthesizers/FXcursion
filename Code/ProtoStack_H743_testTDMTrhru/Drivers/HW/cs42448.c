/*
 * cs42448.c
 *
 *  Created on: Jul 14, 2023
 *      Author: romte
 */

#include "cs42448.h"

static const uint8_t default_config[] = {
	0xF4, // CS42448_Functional_Mode = slave mode, MCLK 25.6 MHz max
	0x76, // CS42448_Interface_Formats = TDM mode
	0x00, //0x1C, // CS42448_ADC_Control_DAC_DeEmphasis = single ended ADC // i have differential
	0x63, // CS42448_Transition_Control = soft vol control
	0x00  // all outs enabled 0xFF  // CS42448_DAC_Channel_Mute = all outputs mute
};

void CS42448_enable(void)
{
	// TODO: wait for reset signal high??
	HAL_Delay(10);
	CS42448_write_byte(CS42448_Power_Control, 0xFF); // power down
	CS42448_write(CS42448_Functional_Mode, default_config, sizeof(default_config));
	CS42448_write_byte(CS42448_Power_Control, 0);  // power up
}

void CS42448_volumeInteger(uint32_t n)
{
	uint8_t data[9];
	data[0] = 0;
	for (int i=1; i < 9; i++) {
		data[i] = n;
	}
	CS42448_write(CS42448_DAC_Channel_Mute, data, 9);
}

//void CS42448_volumeInteger(int channel, uint32_t n)
//{
//}

void CS42448_inputLevelInteger(int32_t n)
{
	uint8_t data[7];
	data[0] = 0;
	for (int i=1; i < 7; i++) {
		data[i] = n;
	}
	CS42448_write(CS42448_DAC_Channel_Invert, data, 7);
}

//void CS42448_inputLevelInteger(int chnnel, int32_t n)
//{
//}

uint32_t CS42448_volumebyte(float level) {
	if (level >= 1.0f) return 0;
	if (level <= 0.0000003981f) return 128;
	return roundf(log10f(level) * -20.0f);
}

int32_t CS42448_inputlevelbyte(float level) {
	if (level > 15.8489f) return 48;
	if (level < 0.00063095734f) return -128;
	return roundf(log10f(level) * 40.0f);
}

void CS42448_filterFreeze(void)
{
	CS42448_write_byte(CS42448_ADC_Control_DAC_DeEmphasis, 0xDC); // disable internal high-pass filter and freeze current dc offset
}

void CS42448_invertDAC(uint32_t data)
{
	CS42448_write_byte(CS42448_DAC_Channel_Invert, data); // these bits will invert the signal polarity of their respective DAC channels (1-8)
}

void CS42448_invertADC(uint32_t data)
{
	CS42448_write_byte(CS42448_ADC_Channel_Invert, data); // these bits will invert the signal polarity of their respective ADC channels (1-6)
}

void CS42448_write_byte(uint32_t address, uint8_t data)
{
	while (HAL_OK != HAL_I2C_Mem_Write(&hi2c4, (CS42448_I2C_ADDR<<1), address, I2C_MEMADD_SIZE_8BIT, &data, 1, 1));
}

void CS42448_write(uint32_t address, const void *data, uint32_t len)
{
	while (HAL_OK != HAL_I2C_Mem_Write(&hi2c4, (CS42448_I2C_ADDR<<1), (address | 0x80), I2C_MEMADD_SIZE_8BIT, data, len, 10));
}

uint8_t* CS42448_checkAddr(void) {
	uint8_t tmp[] = {0,0,0};
	for (uint8_t j = 0; j < 3; j++) {
		for (uint8_t i = 0; i < 0x7F; i++) {
			uint32_t status = HAL_I2C_Mem_Write(&hi2c4, i<<1, 0, I2C_MEMADD_SIZE_8BIT, &i, 1, 1);
			if (status == HAL_OK) {
				tmp[j] = i;
			}
		}
	}
	return tmp;
}

