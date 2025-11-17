#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C" {

// Get RTOS interface
#include "cmsis_os.h"

#include "pubsub.h"

#include "common_cfg.h"

#include "stdio.h"

}

static SUB_HANDLE xUISurveyHandle;

Model::Model() : modelListener(0)
{
    /* Initialize pubsub service */
    if (RESULT_NOT_OK == PUBSUB_Init())
    {
        printf("PUBSUB init failed!\n");
    }

    if (RESULT_NOT_OK == PUBSUB_CreateTopic(PUBSUB_TOPIC_UI, sizeof(UIObjectInfo_t)))
    {
    	printf("UI Survey topic create failed!\n");
    }

	xUISurveyHandle = PUBSUB_Subscribe(PUBSUB_TOPIC_UI, NULL);
}

void Model::tick()
{
	UIObjectInfo_t uiObjectInfo;
    if (RESULT_OK == PUBSUB_Update(xUISurveyHandle,
                                   &uiObjectInfo,
                                   sizeof(UIObjectInfo_t),
                                   0))
    {
		switch (uiObjectInfo.eName) {
		case BTN_YES:
			modelListener->btnYesUpdate();
			break;
		case BTN_NO:
			modelListener->btnNoUpdate();
			break;
		case BTN_UP:
			modelListener->btnUpUpdate();
			break;
		case BTN_DOWN:
			modelListener->btnDownUpdate();
			break;
		case BTN_FOOT:
			modelListener->btnFootUpdate(uiObjectInfo.nID);
			break;
		case BTN_FUNC:
//			modelListener->btnFuncUpdate();
			break;
		case BTN_PARAM:
//			modelListener->btnParamUpdate(uiObjectInfo.nID);
			break;
		case BTN_PLAY:
//			modelListener->btnPlayUpdate();
			break;
		case BTN_STOP:
//			modelListener->btnStopUpdate();
			break;
		case BTN_REC:
//			modelListener->btnRecUpdate();
			break;
		case ENC_MENU:
			modelListener->encMenuUpdate(uiObjectInfo.nValue);
			break;
		case ENC_PARAM:
			modelListener->encParamUpdate(uiObjectInfo.nID, uiObjectInfo.nValue);
			break;
		default:
			break;
		}
	}
}
