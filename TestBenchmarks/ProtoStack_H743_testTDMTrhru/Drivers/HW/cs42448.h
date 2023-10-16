/*
 * cs42448.h
 *
 *  Created on: Jul 14, 2023
 *      Author: romte
 */

#ifndef HW_CS42448_H_
#define HW_CS42448_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../Core/Inc/i2c.h"

#define CS42448_I2C_SEL					0x03  // i2c addr selection thru AD0 AD1 pins
#define CS42448_I2C_ADDR				(0x48 | (CS42448_I2C_SEL & 0x03))

#define CS42448_Chip_ID					0x01
#define CS42448_Power_Control			0x02
#define CS42448_Functional_Mode			0x03
#define CS42448_Interface_Formats		0x04
#define CS42448_ADC_Control_DAC_DeEmphasis	0x05
#define CS42448_Transition_Control		0x06
#define CS42448_DAC_Channel_Mute		0x07
#define CS42448_AOUT1_Volume_Control	0x08
#define CS42448_AOUT2_Volume_Control	0x09
#define CS42448_AOUT3_Volume_Control	0x0A
#define CS42448_AOUT4_Volume_Control	0x0B
#define CS42448_AOUT5_Volume_Control	0x0C
#define CS42448_AOUT6_Volume_Control	0x0D
#define CS42448_AOUT7_Volume_Control	0x0E
#define CS42448_AOUT8_Volume_Control	0x0F
#define CS42448_DAC_Channel_Invert		0x10
#define CS42448_AIN1_Volume_Control		0x11
#define CS42448_AIN2_Volume_Control		0x12
#define CS42448_AIN3_Volume_Control		0x13
#define CS42448_AIN4_Volume_Control		0x14
#define CS42448_AIN5_Volume_Control		0x15
#define CS42448_AIN6_Volume_Control		0x16
#define CS42448_ADC_Channel_Invert		0x17
#define CS42448_Status_Control			0x18
#define CS42448_Status					0x19
#define CS42448_Status_Mask				0x1A
#define CS42448_MUTEC_Pin_Control		0x1B

// interface func
void CS42448_enable(void);
void CS42448_volume(float level);
void   CS42448_inputLevel(float level);
void   CS42448_inputSelect(int n);
//void CS42448_volume(int channel, float level);
//void CS42448_inputLevel(int channel, float level);
void CS42448_filterFreeze(void);
void CS42448_invertDAC(uint32_t data);
void CS42448_invertADC(uint32_t data);


// internal func
  void CS42448_volumeInteger(uint32_t n);
//void CS42448_volumeInteger(int channel, uint32_t n);
  void CS42448_inputLevelInteger(int32_t n);
//void CS42448_inputLevelInteger(int chnnel, int32_t n);
// convert level to volume byte, section 6.9.1, page 50
uint32_t CS42448_volumebyte(float level);
// convert level to input gain, section 6.11.1, page 51
int32_t CS42448_inputlevelbyte(float level);
void CS42448_write_byte(uint32_t address, uint8_t data);
void CS42448_write(uint32_t address, const void *data, uint32_t len);
uint8_t* CS42448_checkAddr(void);



#ifdef __cplusplus
}
#endif

#endif /* HW_CS42448_H_ */
