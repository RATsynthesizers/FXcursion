/*
 * led_ws2812.c
 *
 *  Created on: May 30, 2025
 *      Author: Predtechenskii Dmitrii
 */


/***************************************************************************************************
 * Module includes
***************************************************************************************************/

// Get native header
#include "led_ws2812.h"

// Get memmov and memcpy functions
#include "string.h"


/***************************************************************************************************
 * Local module constants
***************************************************************************************************/

#define LED_WS2812_DATA_BYTES_FOR_ONE_LED        (9U)

/***************************************************************************************************
 * Local module datatypes
***************************************************************************************************/

typedef struct stLED_WS2812_HANDLE
{
    BOOLEAN             bInitialized;
    SPI_HandleTypeDef*  pSPIHandler;
} LED_WS2812_HANDLE;

typedef struct stLED_WS2812_Pixel
{
    /// Spi pulses for each color
    U32 red;
    U32 green;
    U32 blue;
} LED_WS2812_Pixel;


/***************************************************************************************************
 * Local (static) variable definitions
***************************************************************************************************/

/// IC driver handle
static LED_WS2812_HANDLE LED_WS2812_xHandle[LED_MODULES_AMOUNT];

/**
 * @var LED_WS2812_aLooperLedColors[] - array with current looper leds colors
 */
static U8 LED_WS2812_aLooperLedColors[PIXEL_LOOPER_LED_QUANTITY * LED_WS2812_DATA_BYTES_FOR_ONE_LED + 2];

/**
 * @var LED_WS2812_aUILedColors[] - array with current UI leds colors
 */
static U8 LED_WS2812_aUILedColors[PIXEL_UI_LED_QUANTITY * LED_WS2812_DATA_BYTES_FOR_ONE_LED + 2];

// This array allows neo pixel makes linear transition between colors
extern const U8 PIXEL_aLedGamma[256];

/***************************************************************************************************
 * Local (static) function declarations
***************************************************************************************************/

// Converts U8 color to U32 (24 bits color data)
static U32 LED_WS2812_ConvertColor2Bits(const U8 color);

// Fill appropriate led with pixel data
static void LED_WS2812_FillLedData(const LED_MODULE eLedModule,
								   const U16 nLedNum,
								   LED_WS2812_Pixel* const pPixelData);

// Fill all leds with pixel data
static void LED_WS2812_FillAllLedsColor(const LED_MODULE eLedModule,
										LED_WS2812_Pixel* const pLedColor);


/***************************************************************************************************
 * Global function implementation
***************************************************************************************************/

/**
 * @fn    STD_RESULT IC_AT24CS02_Init(IC_AT24CS02_CONFIG *const pConfig);
 *
 * @brief Initializes SW and HW resources for IC AT24CS02.
 *
 * @param[in] pConfig - pointer to configuration container.
 *
 * @return    Result of the function execution. See STD_RESULT type.
 */
STD_RESULT LED_WS2812_Init(const LED_MODULE eLedModule,
						   SpiTransport* const pSpiHandle)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if (NULL_PTR != pSpiHandle && NULL_PTR != pSpiHandle->pSPIHandler)
    {
        // Save configuration parameters
        LED_WS2812_xHandle[eLedModule].pSPIHandler  = pSpiHandle->pSPIHandler;
        LED_WS2812_xHandle[eLedModule].bInitialized = TRUE;

        switch(eLedModule)
        {
        case LED_MODULE_LOOPER:
            memset(LED_WS2812_aLooperLedColors, 0, sizeof(LED_WS2812_aLooperLedColors));
            if (HAL_OK == HAL_SPI_Transmit_DMA(LED_WS2812_xHandle[eLedModule].pSPIHandler,
            								   LED_WS2812_aLooperLedColors,
                                               sizeof(LED_WS2812_aLooperLedColors)))
            {
                eResult = RESULT_OK;
            }
            break;
        case LED_MODULE_UI:
            memset(LED_WS2812_aUILedColors, 0, sizeof(LED_WS2812_aUILedColors));
            if (HAL_OK == HAL_SPI_Transmit_DMA(LED_WS2812_xHandle[eLedModule].pSPIHandler,
            								   LED_WS2812_aUILedColors,
                                               sizeof(LED_WS2812_aUILedColors)))
            {
                eResult = RESULT_OK;
            }
            break;
        default:
        	break;
        }
    }

    return eResult;
}

/**
 * @fn      STD_RESULT LED_WS2812_SetLedColor(const U16, const PIXEL_COLOR* const)
 * @brief   Sets to the certain pixel appropriate color
 * @warning This function does not send bits to leds. Just set driver variables
 *
 * @param   led_number - led order number
 * @param   led_color  - GBR color (8 bit green, 8 bit blue, 8 bit red)
 *
 * @return  STD_RESULT
 */
STD_RESULT LED_WS2812_SetLedColor(const LED_MODULE eLedModule,
								  const U16 nLedNumber,
                                  PIXEL_COLOR* const pLedColor)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if (NULL_PTR != pLedColor && nLedNumber < PIXEL_TOTAL_LED_QUANTITY)
    {
        // Get colors from PIXEL_aLedGamma array for approximate linear effects
        U8 red   = PIXEL_aLedGamma[pLedColor->red];
        U8 green = PIXEL_aLedGamma[pLedColor->green];
        U8 blue  = PIXEL_aLedGamma[pLedColor->blue];

        /* Variable, that holds appropriate bits sequence for each color according to
           LED protocol and SPI timings */
        LED_WS2812_Pixel stPixelData = {0U, 0U, 0U};

        stPixelData.red   = LED_WS2812_ConvertColor2Bits(red);
        stPixelData.green = LED_WS2812_ConvertColor2Bits(green);
        stPixelData.blue  = LED_WS2812_ConvertColor2Bits(blue);

        /* Fill data array with new LED data for the corresponding LED */
        LED_WS2812_FillLedData(eLedModule,
        					   nLedNumber,
                               &stPixelData);

        eResult = RESULT_OK;
    }

    return eResult;
}

/**
 * @fn      STD_RESULT LED_WS2812_SetAllLedsColor(PIXEL_COLOR* const pLedColor)
 *
 * @brief   Fill all leds data with one appropriate color.
 *
 * @param   PIXEL_COLOR* - RGB color structure
 *
 * @return  STD_RESULT
 *
 */
STD_RESULT LED_WS2812_SetAllLedsColor(PIXEL_COLOR* const pLedColor)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if (NULL_PTR != pLedColor)
    {
        // Get colors from PIXEL_aLedGamma array for approximate linear effects
        U8 red   = PIXEL_aLedGamma[pLedColor->red];
        U8 green = PIXEL_aLedGamma[pLedColor->green];
        U8 blue  = PIXEL_aLedGamma[pLedColor->blue];

        // Variable, that holds appropriate bits sequence for each color according to
        // ws2812 protocol and SPI timings
        LED_WS2812_Pixel stPixelData = {0U, 0U, 0U};

        stPixelData.red   = LED_WS2812_ConvertColor2Bits(red);
        stPixelData.green = LED_WS2812_ConvertColor2Bits(green);
        stPixelData.blue  = LED_WS2812_ConvertColor2Bits(blue);

        LED_WS2812_FillAllLedsColor(&stPixelData);

        eResult = RESULT_OK;
    }

    return eResult;
}


/**
 * @fn      STD_RESULT LED_WS2812_SendAllLedsData(void)
 * @brief   SPI dma pixel data send function
 *
 * @param   void
 * @return  STD_RESULT
 */
STD_RESULT LED_WS2812_SendAllLedsData(void)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if(TRUE == LED_WS2812_xHandle.bInitialized
            && NULL_PTR != LED_WS2812_xHandle.pSPIHandler)
    {
        if (HAL_OK == HAL_SPI_Transmit_DMA(LED_WS2812_xHandle.pSPIHandler,
                                           LED_WS2812_aLedColors,
                                           sizeof(LED_WS2812_aLedColors)))
        {
            eResult = RESULT_OK;
        }
    }

    return eResult;
}

/* Implementation of LED movement for WS2812 */
STD_RESULT LED_WS2812_MoveAllLeds(const U8 nDirection)
{
    const U16 nTotalLeds = PIXEL_TOTAL_LED_QUANTITY;
    const U8 nBytesPerLed = LED_WS2812_DATA_BYTES_FOR_ONE_LED;
    const U16 nTotalBytes = nTotalLeds * nBytesPerLed;

    /* Temporary buffer for one LED */
    U8 tempLed[LED_WS2812_DATA_BYTES_FOR_ONE_LED];

    if (nDirection == 0)
    {
        /* Move left: Shift all LEDs to lower indices */
        memcpy(tempLed, &LED_WS2812_aLedColors[0], nBytesPerLed);
        memmove(&LED_WS2812_aLedColors[0],
                &LED_WS2812_aLedColors[nBytesPerLed],
                nTotalBytes - nBytesPerLed);
        memcpy(&LED_WS2812_aLedColors[nTotalBytes - nBytesPerLed],
               tempLed,
               nBytesPerLed);
    }
    else if (nDirection == 1)
    {
        /* Move right: Shift all LEDs to higher indices */
        memcpy(tempLed, &LED_WS2812_aLedColors[nTotalBytes - nBytesPerLed], nBytesPerLed);
        memmove(&LED_WS2812_aLedColors[nBytesPerLed],
                &LED_WS2812_aLedColors[0],
                nTotalBytes - nBytesPerLed);
        memcpy(&LED_WS2812_aLedColors[0], tempLed, nBytesPerLed);
    }
    else
    {
        return RESULT_NOT_OK;  // Invalid direction
    }

    return RESULT_OK;
}


/***************************************************************************************************
 * Local functions implementation
***************************************************************************************************/

/**
 * @fn      U64 LED_WS2812_ConvertColor2Bits(const U8)
 * @brief   Converts real RGB color value to pixel sequence for
 *          address leds protocol
 *
 * @note     This function is called very often and contains a lot of
 *           calculation. That`s why it`s better to make it __inline.
 *
 * @details  For SPI protocol 1 converts to 0xFC, 0 to 0xC0
 *
 * @param   color    - 8 bit color
 * @return  U64 - converted result
 */
__inline static U32 LED_WS2812_ConvertColor2Bits(const U8 color)
{
    U32 color_bits = 0U;
    U8 _0_bits = 0b100;
    U8 _1_bits = 0b110;

    for (U8 bit_num = 0U; bit_num < 8 /* PIXEL_COLOR_BITS_DEPTH */; bit_num ++)
    {
        if (_CHECK_BIT(color,bit_num))
        {
            color_bits += (U32)((U32)_1_bits << (bit_num * 3U));
        }

        else
        {
            color_bits += (U32)((U32)_0_bits << (bit_num * 3U));
        }
    }

    return color_bits;
}


/**
 * @fn      void LED_WS2812_FillLedData(U16, const PIXEL_DATA* const )
 * @brief   Fills spi array with correspond values
 *
 * @note    This function is called very often and contains a lot of
 * @note    calculation. That`s why it`s better to make it __inline.
 *
 *
 * @param   color - 24 bit RGB value
 * @return  void
 */
__inline static void LED_WS2812_FillLedData(const U16 nLedNum,
                                            LED_WS2812_Pixel* const pPixelData)
{
    /* Array for store color bytes values */
    U8 aColorBytes[3U];

    // Why *PIXEL_DATA_BYTES_FOR_ONE_LED see in the comments to array pixel_spi_data definition
    U16 nDataOffset = nLedNum * LED_WS2812_DATA_BYTES_FOR_ONE_LED;

    aColorBytes[0U] = LOBYTE(HIWORD(pPixelData->green));
    aColorBytes[1U] = HIBYTE(LOWORD(pPixelData->green));
    aColorBytes[2U] = LOBYTE(LOWORD(pPixelData->green));
    memcpy(&LED_WS2812_aLedColors[nDataOffset + 0U], aColorBytes, 3U);

    aColorBytes[0U] = LOBYTE(HIWORD(pPixelData->red));
    aColorBytes[1U] = HIBYTE(LOWORD(pPixelData->red));
    aColorBytes[2U] = LOBYTE(LOWORD(pPixelData->red));
    memcpy(&LED_WS2812_aLedColors[nDataOffset + 3U], aColorBytes, 3U);

    aColorBytes[0U] = LOBYTE(HIWORD(pPixelData->blue));
    aColorBytes[1U] = HIBYTE(LOWORD(pPixelData->blue));
    aColorBytes[2U] = LOBYTE(LOWORD(pPixelData->blue));
    memcpy(&LED_WS2812_aLedColors[nDataOffset + 6U], aColorBytes, 3U);
}

/**
 * @fn      void LED_WS2812_FillAllLedsColor(U16, const PIXEL_DATA* const )
 * @brief   Fills spi array with correspond values
 *
 * @note    This function is called very often and contains a lot of
 * @note    calculation. That`s why it`s better to make it __inline.
 *
 *
 * @param   color - 24 bit RGB value
 * @return  void
 */
static void LED_WS2812_FillAllLedsColor(LED_WS2812_Pixel* const pLedColor)
{
    /* Array for store color bytes values */
    U8 aColorBytes[3U];

    /* Fill green color */
    aColorBytes[0U] = LOBYTE(HIWORD(pLedColor->green));
    aColorBytes[1U] = HIBYTE(LOWORD(pLedColor->green));
    aColorBytes[2U] = LOBYTE(LOWORD(pLedColor->green));
    for (U16 nLedNum = 0U; nLedNum < PIXEL_TOTAL_LED_QUANTITY; nLedNum ++)
    {
        memcpy(&LED_WS2812_aLedColors[nLedNum * LED_WS2812_DATA_BYTES_FOR_ONE_LED + 0U], aColorBytes, 3U);
    }

    /* Fill red color */
    aColorBytes[0U] = LOBYTE(HIWORD(pLedColor->red));
    aColorBytes[1U] = HIBYTE(LOWORD(pLedColor->red));
    aColorBytes[2U] = LOBYTE(LOWORD(pLedColor->red));
    for (U16 nLedNum = 0U; nLedNum < PIXEL_TOTAL_LED_QUANTITY; nLedNum ++)
    {
        memcpy(&LED_WS2812_aLedColors[nLedNum * LED_WS2812_DATA_BYTES_FOR_ONE_LED + 3U], aColorBytes, 3U);
    }

    /* Fill blue color */
    aColorBytes[0U] = LOBYTE(HIWORD(pLedColor->blue));
    aColorBytes[1U] = HIBYTE(LOWORD(pLedColor->blue));
    aColorBytes[2U] = LOBYTE(LOWORD(pLedColor->blue));
    for (U16 nLedNum = 0U; nLedNum < PIXEL_TOTAL_LED_QUANTITY; nLedNum ++)
    {
        memcpy(&LED_WS2812_aLedColors[nLedNum * LED_WS2812_DATA_BYTES_FOR_ONE_LED + 6U], aColorBytes, 3U);
    }
}





