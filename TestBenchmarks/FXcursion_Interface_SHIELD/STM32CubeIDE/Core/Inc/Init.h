
#ifndef INIT_H
#define INIT_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"

// Get OS interface
#include "cmsis_os.h"


/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/// None.



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/* All initialization function */
extern void init_all(void);
/* GetIdleTaskMemory prototype (linked to static allocation support) */
extern void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
/* GetTimerTaskMemory prototype (linked to static allocation support) */
extern void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

#if (configGENERATE_RUN_TIME_STATS == 1)
#include "stm32h7xx_hal.h"
extern void vConfigureTimerForRunTimeStats(void);
extern uint32_t vGetRunTimeCounterValue(void);
#endif



#endif  // #ifndef INIT_H
