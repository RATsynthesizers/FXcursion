/**
 * @file      wm8731.c
 *
 * @details   WM8731 driver. See wm8731.h for the SCL-gating scheme.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "wm8731.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/* --- register map ----------------------------------------------------------------------------- */
#define WM_R_LEFT_LINE_IN               (0x00U)
#define WM_R_RIGHT_LINE_IN              (0x01U)
#define WM_R_LEFT_OUT                   (0x02U)
#define WM_R_RIGHT_OUT                  (0x03U)
#define WM_R_ANALOGUE_PATH              (0x04U)
#define WM_R_DIGITAL_PATH               (0x05U)
#define WM_R_POWER_DOWN                 (0x06U)
#define WM_R_INTERFACE                  (0x07U)
#define WM_R_SAMPLING                   (0x08U)
#define WM_R_ACTIVE                     (0x09U)
#define WM_R_RESET                      (0x0FU)
#define WM_REG_QTY                      (16U)

/* --- R7, digital audio interface -------------------------------------------------------------- */
#define WM_IF_FORMAT_I2S                (0x0002U)   /* FORMAT[1:0] = 10             */
#define WM_IF_IWL_24BIT                 (0x0008U)   /* IWL[3:2]    = 10             */
#define WM_IF_SLAVE                     (0x0000U)   /* MS = 0: the MCU is master    */
#define WM_INTERFACE_CFG                (WM_IF_FORMAT_I2S | WM_IF_IWL_24BIT | WM_IF_SLAVE)

/* --- R8, sampling ------------------------------------------------------------------------------
 * Normal mode, BOSR = 0, SR = 0000. With MCLK = 256 x Fs (12.288 MHz from the
 * 24.576 MHz crystal divided by two) that is exactly 48 kHz for ADC and DAC.  */
#define WM_SAMPLING_48K_256FS           (0x0000U)

/* --- R4, analogue path -------------------------------------------------------------------------
 * DAC to the output, line input selected, mic muted, no sidetone, no bypass.  */
#define WM_APATH_MUTEMIC                (0x0002U)
#define WM_APATH_DACSEL                 (0x0010U)
#define WM_ANALOGUE_CFG                 (WM_APATH_MUTEMIC | WM_APATH_DACSEL)

/* --- R5, digital path --------------------------------------------------------------------------
 * ADC high-pass filter ON (removes DC offset from the input stage), no
 * de-emphasis, DAC un-muted.                                                  */
#define WM_DPATH_DACMU                  (0x0008U)
#define WM_DIGITAL_CFG                  (0x0000U)

/* --- R6, power down: a SET bit powers the block DOWN ------------------------------------------- */
#define WM_PD_LINEIN                    (0x0001U)
#define WM_PD_MIC                       (0x0002U)
#define WM_PD_ADC                       (0x0004U)
#define WM_PD_DAC                       (0x0008U)
#define WM_PD_OUT                       (0x0010U)
#define WM_PD_OSC                       (0x0020U)
#define WM_PD_CLKOUT                    (0x0040U)
#define WM_PD_POWEROFF                  (0x0080U)

/** Full-duplex line codec: mic, on-chip oscillator and CLKOUT are unused. */
#define WM_POWER_LINE_CODEC             (WM_PD_MIC | WM_PD_OSC | WM_PD_CLKOUT)

/** Headphone monitor: output only, so the whole input path stays down too. */
#define WM_POWER_HP_CODEC               (WM_PD_LINEIN | WM_PD_MIC | WM_PD_ADC |                 \
                                         WM_PD_OSC | WM_PD_CLKOUT)

/* --- R2/R3, output volume ---------------------------------------------------------------------- */
#define WM_OUT_VOL_0DB                  (0x0079U)   /* 121 -> 0 dB                  */
#define WM_OUT_ZCEN                     (0x0080U)   /* update on zero crossing      */
#define WM_OUT_BOTH                     (0x0100U)   /* write both channels at once  */
#define WM_OUT_VOL_MASK                 (0x007FU)

/* --- R0/R1, line input ------------------------------------------------------------------------- */
#define WM_LINEIN_MUTE                  (0x0080U)
#define WM_LINEIN_VOL_MASK              (0x001FU)

/** Headphone master comes up quiet. The user raises it; nobody gets hurt. */
#define WM_HP_STARTUP_DB                (-30)

#define WM_I2C_TIMEOUT_MS               (50U)

/** Gate level that reaches each codec. See the table in wm8731.h. */
#define WM_GATE_GROUP_A                 (GPIO_PIN_RESET)    /* CODEC_0, CODEC_1    */
#define WM_GATE_GROUP_B                 (GPIO_PIN_SET)      /* CODEC_HP            */



/***************************************************************************************************
* Definitions of local (private) data types
***************************************************************************************************/

typedef struct stWM_CODEC_DESC
{
    U8         nI2cAddr7;       /**< 7-bit address, 0x1A or 0x1B                 */
    GPIO_PinState eGateLevel;   /**< SCL gate level that reaches this codec      */
    BOOLEAN    bOutputOnly;     /**< TRUE for the headphone monitor              */

} WM_CODEC_DESC;



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/*
 * Board wiring. If the schematic puts CODEC_1 behind the other gate, this is the
 * one place to change - nothing else in the driver knows about the gate.
 */
static const WM_CODEC_DESC aCodec[WM8731_CODEC_QTY] =
{
    /* WM8731_CODEC_0  */ { 0x1AU, WM_GATE_GROUP_A, FALSE },
    /* WM8731_CODEC_1  */ { 0x1BU, WM_GATE_GROUP_A, FALSE },
    /* WM8731_CODEC_HP */ { 0x1AU, WM_GATE_GROUP_B, TRUE  },
};



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

static I2C_HandleTypeDef* pBus;
static GPIO_TypeDef*      pGate;
static U16                nGateBit;
static BOOLEAN            bBound;

/** Shadow of the write-only registers, one set per codec. */
static U16 aShadow[WM8731_CODEC_QTY][WM_REG_QTY];



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Read-modify-write one register through the shadow copy.
 */
static STD_RESULT UpdateReg(const WM8731_CODEC eCodec,
                            const U8 nReg,
                            const U16 nMask,
                            const U16 nValue)
{
    U16 nNew = (U16)((aShadow[eCodec][nReg] & (U16)(~nMask)) | (nValue & nMask));

    return WM8731_WriteReg(eCodec, nReg, nNew);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Map dB to the 7-bit output volume code. 0x30 = -73 dB, 0x7F = +6 dB.
 */
static U16 OutVolFromDb(const S8 nDb)
{
    S16 nCode;

    if (nDb <= WM8731_VOL_MIN_DB)
    {
        nCode = 0x30;
    }
    else if (nDb >= WM8731_VOL_MAX_DB)
    {
        nCode = 0x7F;
    }
    else
    {
        nCode = (S16)(0x79 + nDb);          /* 0x79 is 0 dB, 1 dB per step */
    }

    return (U16)nCode;
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief One codec's full configuration, outputs still muted.
 */
static STD_RESULT ConfigureCodec(const WM8731_CODEC eCodec)
{
    const BOOLEAN bHp     = aCodec[eCodec].bOutputOnly;
    const U16     nPower  = bHp ? (U16)WM_POWER_HP_CODEC : (U16)WM_POWER_LINE_CODEC;
    const S8      nStartDb = bHp ? (S8)WM_HP_STARTUP_DB : (S8)0;
    STD_RESULT    eResult;

    /* Reset. Any value works; the device only decodes the register address. */
    eResult = WM8731_WriteReg(eCodec, WM_R_RESET, 0x0000U);
    HAL_Delay(1U);

    /* Outputs down while we configure, so the codec does not pop. */
    if (eResult == RESULT_OK)
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_POWER_DOWN, (U16)(nPower | WM_PD_OUT));
    }

    if ((eResult == RESULT_OK) && (bHp == FALSE))
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_LEFT_LINE_IN,
                                  (U16)(WM8731_LINEIN_0DB | WM_OUT_BOTH));
    }

    if (eResult == RESULT_OK)
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_LEFT_OUT,
                                  (U16)(OutVolFromDb(nStartDb) | WM_OUT_ZCEN | WM_OUT_BOTH));
    }
    if (eResult == RESULT_OK)
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_ANALOGUE_PATH, (U16)WM_ANALOGUE_CFG);
    }
    if (eResult == RESULT_OK)
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_DIGITAL_PATH, (U16)WM_DIGITAL_CFG);
    }
    if (eResult == RESULT_OK)
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_INTERFACE, (U16)WM_INTERFACE_CFG);
    }
    if (eResult == RESULT_OK)
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_SAMPLING, (U16)WM_SAMPLING_48K_256FS);
    }

    /* Digital core on, then outputs - in that order. */
    if (eResult == RESULT_OK)
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_ACTIVE, 0x0001U);
    }
    if (eResult == RESULT_OK)
    {
        HAL_Delay(1U);
        eResult = WM8731_WriteReg(eCodec, WM_R_POWER_DOWN, nPower);
    }

    return eResult;
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT WM8731_Init(I2C_HandleTypeDef* const pI2c,
                       GPIO_TypeDef* const pGatePort,
                       const U16 nGatePin)
{
    STD_RESULT eResult;

    if ((pI2c == NULL_PTR) || (pGatePort == NULL_PTR))
    {
        eResult = RESULT_NOT_OK;
    }
    else
    {
        U8 c;
        U8 r;

        pBus     = pI2c;
        pGate    = pGatePort;
        nGateBit = nGatePin;

        for (c = 0U; c < (U8)WM8731_CODEC_QTY; c++)
        {
            for (r = 0U; r < WM_REG_QTY; r++)
            {
                aShadow[c][r] = 0x0000U;
            }
        }

        bBound  = TRUE;
        eResult = RESULT_OK;
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT WM8731_Start(void)
{
    STD_RESULT eResult = RESULT_OK;
    U8         c;

    if (bBound == FALSE)
    {
        return RESULT_NOT_INIT;
    }

    for (c = 0U; c < (U8)WM8731_CODEC_QTY; c++)
    {
        if (ConfigureCodec((WM8731_CODEC)c) != RESULT_OK)
        {
            eResult = RESULT_NOT_OK;        /* keep going: report the first failure */
        }
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT WM8731_WriteReg(const WM8731_CODEC eCodec, const U8 nReg, const U16 nValue)
{
    STD_RESULT eResult;

    if (bBound == FALSE)
    {
        eResult = RESULT_NOT_INIT;
    }
    else if ((eCodec >= WM8731_CODEC_QTY) || (nReg >= WM_REG_QTY))
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else
    {
        /* THE GATE IS SET HERE, AND ONLY HERE.
         *
         * CODEC_0 and CODEC_HP share address 0x1A, so talking to one requires
         * the other to be gated off. Putting this inside the write is what makes
         * it impossible for a caller to forget - and there is no locking to get
         * wrong, because everything on this bus runs in the super-loop. */
        HAL_GPIO_WritePin(pGate, nGateBit, aCodec[eCodec].eGateLevel);

        {
            /* 7-bit register address then 9 bits of data, packed into two bytes. */
            U8 aFrame[2];

            aFrame[0] = (U8)(((nReg & 0x7FU) << 1U) | (U8)((nValue >> 8U) & 0x01U));
            aFrame[1] = (U8)(nValue & 0xFFU);

            if (HAL_I2C_Master_Transmit(pBus,
                                        (U16)((U16)aCodec[eCodec].nI2cAddr7 << 1U),
                                        aFrame, 2U, WM_I2C_TIMEOUT_MS) == HAL_OK)
            {
                aShadow[eCodec][nReg] = nValue;
                eResult = RESULT_OK;
            }
            else
            {
                eResult = RESULT_NOT_OK;
            }
        }
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT WM8731_SetOutVolumeDb(const WM8731_CODEC eCodec, const S8 nDb)
{
    STD_RESULT eResult;

    if (eCodec >= WM8731_CODEC_QTY)
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else
    {
        /* WM_OUT_BOTH makes the device apply the write to L and R together, so
         * the two channels can never end up at different volumes. */
        eResult = WM8731_WriteReg(eCodec, WM_R_LEFT_OUT,
                                  (U16)(OutVolFromDb(nDb) | WM_OUT_ZCEN | WM_OUT_BOTH));
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT WM8731_SetLineInVolume(const WM8731_CODEC eCodec, const U8 nStep)
{
    STD_RESULT eResult;

    if (eCodec >= WM8731_CODEC_QTY)
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else if (aCodec[eCodec].bOutputOnly != FALSE)
    {
        eResult = RESULT_NOT_OK;            /* the monitor codec has no input path */
    }
    else if (nStep >= WM8731_LINEIN_STEP_QTY)
    {
        eResult = RESULT_INVALID_PARAM_1;
    }
    else
    {
        eResult = WM8731_WriteReg(eCodec, WM_R_LEFT_LINE_IN, (U16)(nStep | WM_OUT_BOTH));
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

STD_RESULT WM8731_Mute(const WM8731_CODEC eCodec, const BOOLEAN bMute)
{
    STD_RESULT eResult;

    if (eCodec >= WM8731_CODEC_QTY)
    {
        eResult = RESULT_INVALID_PARAM_0;
    }
    else
    {
        eResult = UpdateReg(eCodec, WM_R_DIGITAL_PATH, (U16)WM_DPATH_DACMU,
                            (bMute != FALSE) ? (U16)WM_DPATH_DACMU : 0x0000U);
    }

    return eResult;
}

/****************************************** end of file *******************************************/
