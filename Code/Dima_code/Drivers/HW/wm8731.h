/*
 * wm8731.h
 *
 *  Created on: Jul 31, 2020
 *      Author: romte
 */

#ifndef HW_WM8731_H_
#define HW_WM8731_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "i2c.h"

#define W8731_ADDR_0 					0x1A
#define W8731_ADDR_1 					0x1B
#define W8731_NUM_REGS 					10
#define CODEC_ADDRESS_0           		(W8731_ADDR_0 << 1)
#define CODEC_ADDRESS_1           		(W8731_ADDR_1 << 1)
#define CODEC_MEMADD_SIZE				I2C_MEMADD_SIZE_8BIT
#define CODEC_I2C_TIMEOUT				10

#define CODEC_RIGHT						0
#define CODEC_LEFT						1

// CodecRegister {
  #define CODEC_REG_LEFT_LINE_IN         0x00
  #define CODEC_REG_RIGHT_LINE_IN        0x01
  #define CODEC_REG_LEFT_HEADPHONES_OUT  0x02
  #define CODEC_REG_RIGHT_HEADPHONES_OUT 0x03
  #define CODEC_REG_ANALOGUE_ROUTING     0x04
  #define CODEC_REG_DIGITAL_ROUTING      0x05
  #define CODEC_REG_POWER_MANAGEMENT     0x06
  #define CODEC_REG_DIGITAL_FORMAT       0x07
  #define CODEC_REG_SAMPLE_RATE          0x08
  #define CODEC_REG_ACTIVE               0x09
  #define CODEC_REG_RESET                0x0f
//};

// CodecSettings {
  #define CODEC_INPUT_0_DB               0x17
  #define CODEC_INPUT_3_DB               0x19
  #define CODEC_INPUT_UPDATE_BOTH        0x40
  #define CODEC_HEADPHONES_MUTE          0x00
  #define CODEC_HEADPHONES_0_DB			 0x79
  #define CODEC_HEADPHONES_6_DB			 0x7F
  #define CODEC_MIC_BOOST                0x1
  #define CODEC_MIC_MUTE                 0x2
  #define CODEC_ADC_MIC                  0x4
  #define CODEC_ADC_LINE                 0x0
  #define CODEC_OUTPUT_FROM_LINE         0x08
  #define CODEC_OUTPUT_DAC_ENABLE        0x10
  #define CODEC_OUTPUT_MONITOR           0x20
  #define CODEC_DEEMPHASIS_NONE          0x00
  #define CODEC_DEEMPHASIS_32K           0x01
  #define CODEC_DEEMPHASIS_44K           0x02
  #define CODEC_DEEMPHASIS_48K           0x03
  #define CODEC_SOFT_MUTE                0x01
  #define CODEC_ADC_HPF                  0x00

  #define CODEC_POWER_DOWN_LINE_IN       0x01
  #define CODEC_POWER_DOWN_MIC           0x02
  #define CODEC_POWER_DOWN_ADC           0x04
  #define CODEC_POWER_DOWN_DAC           0x08
  #define CODEC_POWER_DOWN_LINE_OUT      0x10
  #define CODEC_POWER_DOWN_OSCILLATOR    0x20
  #define CODEC_POWER_DOWN_CLOCK_OUTPUT  0x40
  #define CODEC_POWER_DOWN_EVERYTHING    0x80

  #define CODEC_PROTOCOL_MASK_RIGHT_JUST 0x00
  #define CODEC_PROTOCOL_MASK_LEFT_JUST  0x01
  #define CODEC_PROTOCOL_MASK_PHILIPS    0x02
  #define CODEC_PROTOCOL_MASK_DSP        0x03

  #define CODEC_FORMAT_MASK_16_BIT       (0x00 << 2)
  #define CODEC_FORMAT_MASK_20_BIT       (0x01 << 2)
  #define CODEC_FORMAT_MASK_24_BIT       (0x02 << 2)
  #define CODEC_FORMAT_MASK_32_BIT       (0x03 << 2)

  #define CODEC_FORMAT_LR_SWAP           0x20
  #define CODEC_FORMAT_MASTER            0x40
  #define CODEC_FORMAT_SLAVE             0x00
  #define CODEC_FORMAT_INVERT_CLOCK      0x80

  #define CODEC_RATE_48K_48K             (0x00 << 2)
  #define CODEC_RATE_8K_8K               (0x03 << 2)
  #define CODEC_RATE_96K_96K             (0x07 << 2)
  #define CODEC_RATE_32K_32K             (0x06 << 2)
  #define CODEC_RATE_44K_44K             (0x08 << 2)
  #define CODEC_MCLK_DIV2                (0x01 << 6)
  #define CODEC_CLKO_DIV2                (0x01 << 7)

  #define CODEC_ACTIVE                   1
  #define CODEC_INACTIVE                 0

//};

void CodecRegWrite(uint16_t regAddr, uint16_t regData, uint8_t codecAddr);
void SetHPGain( int gain, uint8_t codecAddr);
void SetLineInGain( int gain, uint8_t codecAddr);
void CodecPowerUp(uint8_t codecAddr);
void CodecPowerDown(uint8_t codecAddr);

#ifdef __cplusplus
}
#endif

#endif /* HW_WM8731_H_ */
