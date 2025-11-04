/**
 * @file        pixel_drv_cfg.h
 *
 * @details     This is configuration file for pixel driver
 *
 * @version     1.0.0
 *
 * \date        22.08.2025 - 1.0.0 - DVP - First release
 *
 * @copyright   RAT Synthesizers
 */



#ifndef PIXEL_DRV_CFG_H
#define PIXEL_DRV_CFG_H

#include "spi.h"


/***************************************************************************************************
 * Module constants
 ***************************************************************************************************/


/// Total quantity of UI LEDs
#define PIXEL_UI_LED_QUANTITY           	(32U)

/// Specify SPI hardware module for control UI LEDs
#define PIXEL_UI_SPI_CHANNEL            	(3U)

/// Total quantity of looper leds (16 * 2)
#define PIXEL_LOOPER_LED_QUANTITY           (32U)

/// Specify SPI hardware module for control looper LEDs
#define PIXEL_LOOPER_SPI_CHANNEL            (2U)

/// RGB value (8 bits - red, 8 bits - green, 8 bits - blue}
#define PIXEL_LEDS_INITIAL_RED_COLOR       	(0U)
#define PIXEL_LEDS_INITIAL_GREEN_COLOR     	(0U)
#define PIXEL_LEDS_INITIAL_BLUE_COLOR      	(0U)

#endif
