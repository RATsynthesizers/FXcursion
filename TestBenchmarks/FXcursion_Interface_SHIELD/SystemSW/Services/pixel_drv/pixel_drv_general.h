/*
 * pixel_drv_general.h
 *
 *  Created on: Jun 9, 2025
 *      Author: Predtechenskii Dmitrii
 */

#ifndef PIXEL_DRV_GENERAL_H_
#define PIXEL_DRV_GENERAL_H_

#include "spi.h"



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

#define LED_MODULES_AMOUNT 2

/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/


/**
 * @struct PIXEL_COLOR
 * @brief  common color structure
 *
 */
typedef struct stPIXEL_COLOR
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} PIXEL_COLOR;

/**
 * @struct PIXEL_COLOR
 * @brief  common color structure
 *
 */
typedef enum eLED_MODULE
{
    LED_MODULE_LOOPER = 0U,
    LED_MODULE_UI	  = 1U,
} LED_MODULE;

/**
 * @struct SpiTransport
 * @brief  SPI transport structure
 *
 */
typedef struct stSpiTransport
{
    SPI_HandleTypeDef* pSPIHandler;
} SpiTransport;

#endif /* PIXEL_DRV_GENERAL_H_ */
