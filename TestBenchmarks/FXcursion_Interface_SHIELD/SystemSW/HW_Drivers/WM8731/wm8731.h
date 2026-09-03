/**
 * @file      wm8731.h
 *
 * @details   Driver for the three WM8731 codecs on the FXcursion audio board.
 *
 *            ------------------------------------------------------------------
 *            THREE CODECS, TWO ADDRESSES
 *            ------------------------------------------------------------------
 *
 *            A WM8731 has exactly two I2C addresses, 0x1A and 0x1B, selected by
 *            its CSB pin. Three of them cannot share a bus. This board solves it
 *            by GATING SCL: I2C1_SCL is split into I2C1_SCL1 and I2C1_SCL2 by
 *            two AND gates, steered by one GPIO. Flip the GPIO and the third
 *            codec answers on the same address as the first.
 *
 *                CODEC_0   0x1A   gate LOW    channels 0,1   (SAI1)
 *                CODEC_1   0x1B   gate LOW    channels 2,3   (SAI2)
 *                CODEC_HP  0x1A   gate HIGH   headphones     (I2S3, TX only)
 *
 *            CODEC_0 and CODEC_HP are therefore MUTUALLY EXCLUSIVE on the bus.
 *            The gate is set inside WM8731_WriteReg, never as a separate call a
 *            caller could forget - that is the whole point of the design here.
 *
 *            ------------------------------------------------------------------
 *            WRITE-ONLY REGISTERS
 *            ------------------------------------------------------------------
 *
 *            WM8731 control registers cannot be read back. The driver keeps a
 *            shadow copy so that read-modify-write on a single field is
 *            possible. Never write a register except through this driver, or
 *            the shadow and the device diverge.
 *
 *            ------------------------------------------------------------------
 *            CONTEXT
 *            ------------------------------------------------------------------
 *
 *            All of this runs in the SUPER-LOOP: boot-time configuration and
 *            user actions. HAL_I2C blocking calls and HAL_Delay are used, so
 *            NOTHING here may be called from the audio ISR.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef WM8731_H
#define WM8731_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "main.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/** Headphone / line output volume range, dB. 0x30..0x7F maps -73..+6. */
#define WM8731_VOL_MIN_DB               (-73)
#define WM8731_VOL_MAX_DB               (6)

/** Line input volume steps. 0x00..0x1F maps -34.5 dB..+12 dB in 1.5 dB steps. */
#define WM8731_LINEIN_STEP_QTY          (32U)
#define WM8731_LINEIN_0DB               (23U)       /* 0x17 */



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief The three codecs, in channel order.
 */
typedef enum enWM8731_CODEC
{
    WM8731_CODEC_0   = 0U,      /**< SAI1, audio channels 0 and 1                */
    WM8731_CODEC_1   = 1U,      /**< SAI2, audio channels 2 and 3                */
    WM8731_CODEC_HP  = 2U,      /**< I2S3, headphone monitor - output only       */

    WM8731_CODEC_QTY = 3U

} WM8731_CODEC;



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Bind the driver to its bus and its SCL gate.
 *
 * Does not touch the hardware. Call once, before WM8731_Start.
 *
 * @param pI2c       control bus, already initialised (CODEC_CTRL_I2C)
 * @param pGatePort  GPIO port of the SCL gate select
 * @param nGatePin   GPIO pin  of the SCL gate select
 */
extern STD_RESULT WM8731_Init(I2C_HandleTypeDef* const pI2c,
                              GPIO_TypeDef* const pGatePort,
                              const U16 nGatePin);

/**
 * @brief Reset and configure all three codecs, then bring them out of mute.
 *
 * 48 kHz, 24-bit, I2S standard, all three as CLOCK SLAVES - the MCU's SAI1
 * block A is the master for codecs 0 and 1, and I2S3 for the headphone codec.
 *
 * The headphone codec is configured output-only: its ADC and line input are
 * powered down, which is both correct and saves a little current.
 *
 * Outputs are enabled last, after ACTIVE, so the codecs do not pop.
 */
extern STD_RESULT WM8731_Start(void);

/**
 * @brief Output volume, in dB, clamped to WM8731_VOL_MIN_DB..WM8731_VOL_MAX_DB.
 *
 * This is the ANALOGUE volume inside the codec, after the DAC. Use it for the
 * headphone master rather than scaling in the DSP - at low listening levels a
 * float multiply throws away resolution the analogue attenuator keeps.
 */
extern STD_RESULT WM8731_SetOutVolumeDb(const WM8731_CODEC eCodec, const S8 nDb);

/**
 * @brief Line input gain, 0..WM8731_LINEIN_STEP_QTY-1. WM8731_LINEIN_0DB is unity.
 *
 * Refused for WM8731_CODEC_HP, which has no input path enabled.
 */
extern STD_RESULT WM8731_SetLineInVolume(const WM8731_CODEC eCodec, const U8 nStep);

/** Mute or unmute the DAC path. */
extern STD_RESULT WM8731_Mute(const WM8731_CODEC eCodec, const BOOLEAN bMute);

/**
 * @brief Raw register write. Sets the SCL gate for the target codec itself.
 *
 * @param eCodec  which codec
 * @param nReg    register address, 0..15
 * @param nValue  9-bit register value
 */
extern STD_RESULT WM8731_WriteReg(const WM8731_CODEC eCodec,
                                  const U8 nReg,
                                  const U16 nValue);



#endif // #ifndef WM8731_H

/****************************************** end of file *******************************************/
