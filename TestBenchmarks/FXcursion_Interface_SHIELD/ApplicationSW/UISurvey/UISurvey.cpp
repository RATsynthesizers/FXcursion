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

#include "Recorder.h"

/*
 * Defined in Recorder.c and not exported by Recorder.h, so it has to be
 * spelled out here. Inside the extern "C" block deliberately: the definition
 * has C linkage, and although a global of this shape happens to mangle to the
 * same symbol either way, relying on that is not worth the surprise. It would
 * be better placed in Recorder.h next to RecorderInit.
 */
extern osSemaphoreId sem_RecStart;

}

/*
 * All of the button instances live here, next to the only loop that polls
 * them. button.cpp used to declare five of these as globals too; those were
 * unreachable and are gone.
 */
static Button btnYes(BTN_YES_GPIO_Port,
			  	  	 BTN_YES_Pin);

static Button btnNo(BTN_NO_GPIO_Port,
			 	 	BTN_NO_Pin);

static Button btnUp(BTN_UP_GPIO_Port,
			 	 	BTN_UP_Pin);

static Button btnDown(BTN_DOWN_GPIO_Port,
			   	   	  BTN_DOWN_Pin);

static Button btnFunc(BTN_FUNC_GPIO_Port,
			   	   	  BTN_FUNC_Pin);

static Button btnRec(BTN_REC_GPIO_Port,
					 BTN_REC_Pin);

static osThreadId xUISurveyThreadHandle;


/*
 * Poll period.
 *
 * This loop used to have no delay at all. At osPriorityBelowNormal it could
 * not starve the GUI, but the idle task never ran either, so the part is
 * permanently awake - which on something running off a battery is a straight
 * current cost - and the button debounce window, which is measured against
 * HAL_GetTick, was being sampled at a rate that varied with system load.
 *
 * 1 ms is comfortably faster than anything a hand can do to an encoder: a
 * 20-detent knob spun at three revolutions per second only puts 60 Hz on
 * channel A, so there is no risk of aliasing away an edge, and the poll is a
 * dozen GPIO reads. It also makes the 50 ms debounce mean 50 samples rather
 * than "however many fitted".
 */
#define UI_SURVEY_POLL_MS   (1U)


static STD_RESULT UISurveyThreadInit(void);
static void UISurveyThreadWrapper(void const *arg);

static void PublishEncoderSteps(Encoder* const pEncoder,
                                const UIObjectType eName,
                                const U8 nID);
static BOOLEAN PublishUIEvent(const UIObjectType eName,
                              const U8 nID,
                              const S8 nValue);

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
            .stacksize = 256U
    };
    xUISurveyThreadHandle = osThreadCreate(&ThreadDef, NULL_PTR);

    if (NULL == xUISurveyThreadHandle)
    {
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}

/**
 * @fn        BOOLEAN PublishUIEvent(const UIObjectType eName,
 *                                   const U8 nID,
 *                                   const S8 nValue)
 *
 * @brief     Post one UI event to the GUI, filling in EVERY field.
 *
 * @note      The message used to be built in a single shared static whose nID
 *            was only ever assigned in the ENC_PARAM branch. Every other
 *            event therefore carried whatever nID the last encoder message
 *            had left behind - harmless today only because BTN_FOOT, the one
 *            consumer that reads nID for anything else, is not published yet.
 *            Taking all three fields as arguments makes that impossible.
 *
 * @return    TRUE if the message was accepted by the topic.
 */
static BOOLEAN PublishUIEvent(const UIObjectType eName,
                              const U8 nID,
                              const S8 nValue)
{
	UIObjectInfo_t uiObjectChange;

	uiObjectChange.eName  = eName;
	uiObjectChange.nID    = nID;
	uiObjectChange.nValue = nValue;

	return (RESULT_OK == PUBSUB_Publish(PUBSUB_TOPIC_UI,
	                                    (void*) &uiObjectChange,
	                                    sizeof(uiObjectChange)))
	       ? TRUE : FALSE;
}

/**
 * @fn        void PublishEncoderSteps(Encoder* const pEncoder,
 *                                     const UIObjectType eName,
 *                                     const U8 nID)
 *
 * @brief     Hand over as many detents as the topic will take, one message
 *            each.
 *
 * @note      Publish first, consume second. If the topic is full the step
 *            stays owed and is retried on the next poll, so a burst costs
 *            latency instead of losing clicks. The old order - clear the
 *            changed flag, then publish and ignore the result - dropped them.
 *
 * @note      One step per message, never an accumulated total: the consumers
 *            test `1 == nValue` and `-1 == nValue` exactly.
 *
 * @return    None.
 */
static void PublishEncoderSteps(Encoder* const pEncoder,
                                const UIObjectType eName,
                                const U8 nID)
{
	S8 nStep = pEncoder->pendingStep();

	while (0 != nStep)
	{
		if (FALSE == PublishUIEvent(eName, nID, nStep))
		{
			/* Topic full - leave the step owed and come back next poll. */
			break;
		}

		pEncoder->stepDelivered();

		nStep = pEncoder->pendingStep();
	}
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
static void UISurveyThreadWrapper(void const *argument)
{
    for(;;)
    {
    	encMenu.update();
    	PublishEncoderSteps(&encMenu, ENC_MENU, 0U);

    	// Repeat for each enc param
    	encParam.update();
    	PublishEncoderSteps(&encParam, ENC_PARAM, 0U);

    	/*
    	 * Buttons follow the same publish-then-clear order as the encoders,
    	 * so a full topic retries instead of swallowing a press.
    	 */
    	btnYes.update();
    	if(TRUE == btnYes.wasChanged())
    	{
    		if(TRUE == PublishUIEvent(BTN_YES, 0U, btnYes.getState()))
    		{
    			btnYes.clearChangedFlag();
    		}
    	}

    	btnNo.update();
    	if(TRUE == btnNo.wasChanged())
    	{
    		if(TRUE == PublishUIEvent(BTN_NO, 0U, btnNo.getState()))
    		{
    			btnNo.clearChangedFlag();
    		}
    	}

    	btnUp.update();
    	if(TRUE == btnUp.wasChanged()
    			&& BTN_PRESSED == btnUp.getState())
    	{
    		if(TRUE == PublishUIEvent(BTN_UP, 0U, btnUp.getState()))
    		{
    			btnUp.clearChangedFlag();
    		}
    	}

    	btnDown.update();
    	if(TRUE == btnDown.wasChanged()
    			&& BTN_PRESSED == btnDown.getState())
    	{
    		if(TRUE == PublishUIEvent(BTN_DOWN, 0U, btnDown.getState()))
    		{
    			btnDown.clearChangedFlag();
    		}
    	}

    	// Repeat for each foot switch
    	btnFunc.update();
    	if(TRUE == btnFunc.wasChanged())
    	{
    		if(TRUE == PublishUIEvent(BTN_FUNC, 0U, btnFunc.getState()))
    		{
    			btnFunc.clearChangedFlag();
    		}
    	}

    	btnRec.update();
    	if(TRUE == btnRec.wasChanged()
    			&& BTN_PRESSED == btnRec.getState())
    	{
    		btnRec.clearChangedFlag();
    		osSemaphoreRelease(sem_RecStart);
    	}

    	osDelay(UI_SURVEY_POLL_MS);
    }
}
