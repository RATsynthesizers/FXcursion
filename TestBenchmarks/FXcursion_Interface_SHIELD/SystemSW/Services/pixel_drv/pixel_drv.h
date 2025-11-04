/**
 * @file        pixel_drv.h
 *
 * @details     Address LED module interface
 *
 * @version     1.0.0
 *
 *\date         1.0.0 - 22.01.2024 - AVV - First release
 *
 * @copyright   LLC Fly Fire
 *
 */

#ifndef PIXEL_DRV_H
#define PIXEL_DRV_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"

// Get pixel general header
#include "pixel_drv_general.h"

// Get configuration parameters
#include "pixel_drv_cfg.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

// None

/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @enum    PIXEL_JOB_TYPE_enum
 * @brief   Available pixel states
 *
 */
typedef enum enPIXEL_MODE_TYPE
{
    PIXEL_SET_ALL     = 0U,/**< PIXEL_IDLE */
    PIXEL_SET_LED     = 1U,/**< PIXEL_CONST */
    PIXEL_PULSE       = 2U,/**< PIXEL_PULSE */
    PIXEL_BLINK       = 3U, /**< PIXEL_BLINK */
    PIXEL_RUNNING     = 4U, /**< PXEL_RUNNING */
    PIXEL_TRANSITION  = 5U,
    PIXEL_BLINK_ONE   = 6U,
    PIXEL_PULSE_ONE   = 7U,
    PIXEL_TRANSIT_ONE = 8U
}PIXEL_MODE_TYPE;

/**
 * @struct PIXEL_SETTINGS_struct
 * @brief  Settings for pixel module
 *
 */
typedef struct PIXEL_CMD_struct
{
    U8               ePixelMode; /*PIXEL_MODE_TYPE*/
    PIXEL_COLOR      ledColor1;
    PIXEL_COLOR      ledColor2;
    U16              nCmdParam1;
    U16              nCmdParam2;
    U16              nCmdParam3;
    U16              nCmdParam4;
    U16              nCmdParam5;
} PIXEL_CMD;

/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

// Initialize module
extern STD_RESULT PIXEL_Init(void);

// Set idle mode
extern STD_RESULT PIXEL_SetAllLedsColor(PIXEL_COLOR* const pLedColor);

// Set led color
extern STD_RESULT PIXEL_SetLedColor(PIXEL_COLOR* const pLedColor, const U16 nLedNum);

// Set running module
extern STD_RESULT PIXEL_SetRunningMode(PIXEL_COLOR* const pLineColor,
                                        const U16 nLinesNumber,
                                        const U16 nLinesLength,
                                        const U16 nLinesDistance,
                                        const U16 bDirection,
                                        const  U16 nFrameDelay);

// Set blink mode
extern STD_RESULT PIXEL_SetBlinkMode(PIXEL_COLOR* const pLedColor,
                                        const U16 nOnTime,
                                        const U16 nOffTime);

// Set pulse mode
extern STD_RESULT PIXEL_SetPulseMode(PIXEL_COLOR* const pLedColor,
                                        const U16 nRiseTime,
                                        const U16 nFallTime,
                                        const U16 nFrameDelay);

// Set transit module
extern STD_RESULT PIXEL_SetTransitMode(PIXEL_COLOR* const pBaseColor,
                                        PIXEL_COLOR* const pNewColor,
                                        const U16 nTransitionTime,
                                        const U16 nFrameDelay);

// Set one led blink mode
extern STD_RESULT PIXEL_SetBlinkOneMode(PIXEL_COLOR* const pLedColor,
                                        const U16 nOnTime,
                                        const U16 nOffTime,
                                        const U16 nLedNum);

// Set one led pulse mode
extern STD_RESULT PIXEL_SetPulseOneMode(PIXEL_COLOR* const pLedColor,
                                        const U16 nRiseTime,
                                        const U16 nFallTime,
                                        const U16 nFrameDelay,
                                        const U16 nLedNum);

// Set one led transit module
extern STD_RESULT PIXEL_SetTransitOneMode(PIXEL_COLOR* const pBaseColor,
                                            PIXEL_COLOR* const pNewColor,
                                            const U16 nTransitionTime,
                                            const U16 nFrameDelay,
                                            const U16 nLedNum);



#endif
