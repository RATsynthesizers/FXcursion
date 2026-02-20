/**
 * @file      common_cfg.h
 *
 * @details   Common configuration parameters
 *
 * @version   1.0.0
 *
 * @authors   Predtechenskii Dmitrii (predtech4@yandex.ru)
 *
 * \date      12.08.2025 - First release
 *
 */



#ifndef COMMON_CFG_H
#define COMMON_CFG_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"

/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define PUBSUB_TOPIC_UI		(char* const)"ui_survey"

/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

typedef enum enUIObjectType
{
	BTN_YES		= 0,
	BTN_NO		= 1,
	BTN_UP		= 2,
	BTN_DOWN	= 3,
	BTN_FOOT	= 4,
	BTN_FUNC	= 5,
	BTN_PARAM	= 6,
	BTN_PLAY	= 7,
	BTN_STOP	= 8,
	BTN_REC		= 9,
	BTN_MENU	= 10,

	ENC_MENU	= 11,
	ENC_PARAM	= 12,

} UIObjectType;

typedef struct stUIObjectInfo
{
	UIObjectType eName;
	U8 nID;
	S8 nValue;

} UIObjectInfo_t;

/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/// None.

#endif  // #ifndef COMMON_CFG_H

