#ifndef GENERAL_TYPES_H
#define GENERAL_TYPES_H


/**
 * @brief Contains platform-specific types definitions
 */
#include "general_platform.h"



/**
 * @brief True/False definition
 */
#ifndef FALSE
#define FALSE                       (0U)
#endif
#ifndef TRUE
#define TRUE                        (1U)
#endif

/**
 * @brief On/Off definition
 */
#ifndef OFF
#define OFF                         (0U)
#endif
#ifndef ON
#define ON                          (1U)
#endif

/**
 * @brief Logical signal state definitions
 */
#ifndef LOGIC_0
#define LOGIC_0                     (0U)
#endif
#ifndef LOGIC_1
#define LOGIC_1                     (1U)
#endif


#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * @brief Null pointer definition
 */
#ifndef NULL_PTR
#define NULL_PTR            ((void*)(0))
#endif


/**
 * @brief Result types
 */
typedef enum STD_RESULT_enum
{
    RESULT_OK                      = (U8)0U,
    RESULT_NOT_OK                  = (U8)1U,
    RESULT_BUSY                    = (U8)2U,
    RESULT_TIMEOUT                 = (U8)3U,
    RESULT_NOT_INIT                = (U8)4U,
    RESULT_ALREDY_INIT             = (U8)5U,
    RESULT_INVALID_PARAM_0         = (U8)6U,
    RESULT_INVALID_PARAM_1         = (U8)7U,
    RESULT_INVALID_PARAM_2         = (U8)8U,
    RESULT_INVALID_PARAM_3         = (U8)9U,
    RESULT_INVALID_PARAM_4         = (U8)10U,
    RESULT_INVALID_PARAM_5         = (U8)11U,
    RESULT_INVALID_PARAM_6         = (U8)12U,
    RESULT_INVALID_PARAM_7         = (U8)13U,
    RESULT_INVALID_PARAM_8         = (U8)14U,
    RESULT_INVALID_PARAM_9         = (U8)15U,
    RESULT_INTERNAL_ERROR          = (U8)16U,
    RESULT_LAST                    = (U8)17U,
} STD_RESULT;

/**
 * @brief Power state types
 */
typedef enum enPOWER_STATE
{
    TURN_OFF = (U8)0U,
    TURN_ON  = (U8)1U,

} POWER_STATE;

/**
 * @brief Link state types
 */
typedef enum enLINK_STATE
{
    LINK_OK      = (U8)0U,
    LINK_NOT_OK  = (U8)1U,

} LINK_STATE;


// Voltage
// Resolution: 1  millivolts / bit,   Offset: 0 volts,   Range: 0 - 65.535 volts
typedef U16 U16_PHYS_VOLT_1mV;
// Resolution: 10 millivolts / bit,   Offset: 0 volts,   Range: 0 - 655.35 volts
typedef U16 U16_PHYS_VOLT_10mV;
// Resolution: 100 millivolts / bit,  Offset: 0 volts,   Range: 0 - 6553.5 volts
typedef U16 U16_PHYS_VOLT_100mV;
// Resolution: 1  millivolts / bit,   Offset: 0 volts,   Range: -32.767 - 32.768 volts
typedef S16 S16_PHYS_VOLT_1mV;

// Amperage
// Resolution: 1  milliamps / bit,    Offset: 0 amps,   Range: 0 - 65.535 amps
typedef U16 U16_PHYS_AMP_1mA;
// Resolution: 10 milliamps / bit,    Offset: 0 amps,   Range: 0 - 655.35 amps
typedef U16 U16_PHYS_AMP_10mA;
// Resolution: 1  milliamps / bit,    Offset: 0 amps,   Range: -32.767 - 32.768 amps
typedef S16 S16_PHYS_AMP_1mA;
// Resolution: 10  milliamps / bit,   Offset: 0 amps,   Range: -327.67 - 327.68 amps
typedef S16 S16_PHYS_AMP_10mA;
// Resolution: 100  milliamps / bit,  Offset: 0 amps,   Range: -3276.7 - 3276.8 amps
typedef S16 S16_PHYS_AMP_100mA;

// Temperature
// Resolution: 1 degrees / bit,    Offset: 0 deg,   Range: 0 - 255 deg
typedef U8  U8_PHYS_TEMP_1DEG;
// Resolution: 1 degrees / bit,    Offset: 0 deg,   Range: -127 - 128 deg
typedef S8  S8_PHYS_TEMP_1DEG;
// Resolution: 1 degrees / bit,    Offset: 0 deg,   Range: -32767 - 32768 deg
typedef S16 S16_PHYS_TEMP_1DEG;
// Resolution: 0.1 degrees / bit,  Offset: 0 deg,   Range: -3276.7 - 3276.8 deg
typedef S16 S16_PHYS_TEMP_01DEG;
// Resolution: 0.01 degrees / bit,  Offset: 0 deg,   Range: -327.67 - 327.68 deg
typedef S16 S16_PHYS_TEMP_001DEG;
// Resolution: 1 degrees / bit,    Offset: 0 deg,   Range: 0 - 65535 deg
typedef U16 U16_PHYS_TEMP_1DEG;
// Resolution: 0.1 degrees / bit,  Offset: 0 deg,   Range: 0 - 6553.5 deg
typedef U16 U16_PHYS_TEMP_01DEG;



/**
 * @brief Memory manager functions types
 */
typedef STD_RESULT (*MemManRead)(const U32 nAddr, U8* const pBuf, const U32 nLength);
typedef STD_RESULT (*MemManWrite)(const U32 nAddr, U8* const pBuf, const U32 nLength);
typedef STD_RESULT (*MemManErase)(const U32 nAddr, const U32 nPage, const U8 nBank);
typedef U8         (*MemManUnlock)(void);
typedef void       (*MemManLock)(void);



/**
 * @brief TIME data types
 */
typedef struct stTIME
{
    U8 nSecond;
    U8 nMinute;
    U8 nHour;
    U8 nDay;
    U8 nMonth;
    U8 nYear;
} TIME;


#endif // #ifndef GENERAL_TYPES_H

/****************************************** end of file *******************************************/
