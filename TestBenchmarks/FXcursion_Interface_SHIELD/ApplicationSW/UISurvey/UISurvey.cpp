/*
 * UISurvey.cpp
 *
 *  Created on: Nov 16, 2025
 *      Author: ga
 */


#include "UISurvey.hpp"
#include "encoder.hpp"
#include "button.hpp"

extern "C" {

// Get RTOS interface
#include "cmsis_os.h"

#include "pubsub.h"

}

static osThreadId xUISurveyThreadHandle;
static UIObjectInfo_t uiObjectChange;

static STD_RESULT UISurveyThreadInit(void);
static void UISurveyThreadWrapper(void const *arg);

STD_RESULT UISurveyInit()
{
	if (RESULT_OK != UISurveyThreadInit())
	{
		return RESULT_NOT_OK;
	}
	return RESULT_OK;
}

static STD_RESULT UISurveyThreadInit(void)
{
    char strThreadName[configMAX_TASK_NAME_LEN];

    /* Create Rx Thread */
    strThreadName[0U] = 'U';
    strThreadName[1U] = 'I';
    strThreadName[2U] = 'S';
    strThreadName[3U] = 'u';
    strThreadName[4U] = 'r';
    strThreadName[5U] = 'v';
    strThreadName[6U] = 'e';
    strThreadName[7U] = 'y';
    strThreadName[8U] = 0U;

    /* Create the thread */
    osThreadDef_t ThreadDef =
    {
            .name = strThreadName,
            .pthread = UISurveyThreadWrapper,
            .tpriority = osPriorityBelowNormal,
            .instances = 0,
            .stacksize = 200U
    };
    xUISurveyThreadHandle = osThreadCreate(&ThreadDef, NULL_PTR);

    if (NULL == xUISurveyThreadHandle)
    {
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}


/**
 * @fn        void UISurveyThreadWrapper(void const *argument)
 *
 * @brief     Thread for hardware UI state update.
 *
 * @param[in] argument - pointer to input arguments.
 *
 * @return    None.
 */
void UISurveyThreadWrapper(void const *argument)
{

    for(;;)
    {
    	encMenu.update();
    	if(TRUE == encMenu.wasChanged())
    	{
    		encMenu.clearChangedFlag();
    		uiObjectChange.eName  = ENC_MENU;
    		uiObjectChange.nValue = encMenu.getState();
    		PUBSUB_Publish(PUBSUB_TOPIC_UI, (void*) &uiObjectChange, sizeof(uiObjectChange));
    	}

    	// Repeat for each enc param
    	encParam.update();
    	if(TRUE == encParam.wasChanged())
    	{
    		encParam.clearChangedFlag();
    		uiObjectChange.eName  = ENC_PARAM;
    		uiObjectChange.nID    = 0;
    		uiObjectChange.nValue = encParam.getState();
    		PUBSUB_Publish(PUBSUB_TOPIC_UI, (void*) &uiObjectChange, sizeof(uiObjectChange));
    	}

    	btnYes.update();
    	if(TRUE == btnYes.wasChanged()
    			&& BTN_PRESSED == btnYes.getState())
    	{
    		btnYes.clearChangedFlag();
    		uiObjectChange.eName  = BTN_YES;
    		uiObjectChange.nValue = btnYes.getState();
    		PUBSUB_Publish(PUBSUB_TOPIC_UI, (void*) &uiObjectChange, sizeof(uiObjectChange));
    	}

    	btnNo.update();
    	if(TRUE == btnNo.wasChanged()
    			&& BTN_PRESSED == btnNo.getState())
    	{
    		btnNo.clearChangedFlag();
    		uiObjectChange.eName  = BTN_NO;
    		uiObjectChange.nValue = btnNo.getState();
    		PUBSUB_Publish(PUBSUB_TOPIC_UI, (void*) &uiObjectChange, sizeof(uiObjectChange));
    	}

    	btnUp.update();
    	if(TRUE == btnUp.wasChanged()
    			&& BTN_PRESSED == btnUp.getState())
    	{
    		btnUp.clearChangedFlag();
    		uiObjectChange.eName  = BTN_UP;
    		uiObjectChange.nValue = btnUp.getState();
    		PUBSUB_Publish(PUBSUB_TOPIC_UI, (void*) &uiObjectChange, sizeof(uiObjectChange));
    	}

    	btnDown.update();
    	if(TRUE == btnDown.wasChanged()
    			&& BTN_PRESSED == btnDown.getState())
    	{
    		btnDown.clearChangedFlag();
    		uiObjectChange.eName  = BTN_DOWN;
    		uiObjectChange.nValue = btnDown.getState();
    		PUBSUB_Publish(PUBSUB_TOPIC_UI, (void*) &uiObjectChange, sizeof(uiObjectChange));
    	}

    	// Repeat for each foot switch
    	btnFoot.update();
    	if(TRUE == btnFoot.wasChanged()
    			&& BTN_PRESSED == btnFoot.getState())
    	{
    		btnFoot.clearChangedFlag();
    		uiObjectChange.eName  = BTN_FOOT;
    		uiObjectChange.nID    = 0;
    		uiObjectChange.nValue = btnFoot.getState();
    		PUBSUB_Publish(PUBSUB_TOPIC_UI, (void*) &uiObjectChange, sizeof(uiObjectChange));
    	}
    }
}
