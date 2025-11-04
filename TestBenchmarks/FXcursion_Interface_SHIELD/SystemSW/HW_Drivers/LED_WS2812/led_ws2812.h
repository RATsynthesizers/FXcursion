/*
 * led_ws2812.h
 *
 *  Created on: May 30, 2025
 *      Author: Predtechenskii Dmitrii
 */

#ifndef LED_WS2812_H
#define LED_WS2812_H



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

// None



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

// Initialize module
extern STD_RESULT LED_WS2812_Init(const LED_MODULE eLedModule,
								  SpiTransport* const pSpiHandle);

// Set certain led color
extern STD_RESULT LED_WS2812_SetLedColor(const LED_MODULE eLedModule,
									     const U16 nLedNumber,
                                    	 PIXEL_COLOR* const pLedColor);

// Fills all leds data with one color
extern STD_RESULT LED_WS2812_SetAllLedsColor(const LED_MODULE eLedModule,
											 PIXEL_COLOR* const pLedColor);

// Sends data bytes for all pixels
extern STD_RESULT LED_WS2812_SendAllLedsData(const LED_MODULE eLedModule);

/* Implementation of LED movement for WS2812 */
extern STD_RESULT LED_WS2812_MoveAllLeds(const LED_MODULE eLedModule,
										 const U8 nDirection);


#endif /* LED_WS2812_H */
