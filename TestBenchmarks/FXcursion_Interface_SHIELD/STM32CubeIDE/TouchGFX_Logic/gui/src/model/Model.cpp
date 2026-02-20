#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C" {

#include "pubsub.h"

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

	/* Initialize model variables */

	sprintf(projectName, "Untitled", sizeof("Untitled"));
	nBatteryState = 100;
	nBPM = 120;
	bIsFuncPressed = FALSE;

	/* Initializing system screen variables */

	// TODO: if loading project, read values from config file or EEPROM
	bInputIsStereo1 = FALSE;
	bInputIsStereo2 = FALSE;
	nMixerPosition = -1;
	eSelectedModule = SELECT_INPUT;
	eSelectedChainModule = CHAIN_MODULE_1;
	eSelectedFootSwitch = FOOT_SWITCH_1;

	for(U8 i = 0; i < 4; i++)
	{
		for(U8 j = 0; j < 4; j++)
		{
			eAddedMonoModule[i][j] = MODULE_NONE;
		}
	}
	for(U8 i = 0; i < 2; i++)
	{
		for(U8 j = 0; j < 4; j++)
		{
			eAddedStereoModule[i][j] = MODULE_NONE;
		}
	}

	/* Initializing global module settings variables */

    eSelectedChannel = CHANNEL_MONO_1;

	/* Initializing FX chain settings variables */

	for (int i = 0; i < CHANNELS_NUM; i++)
	{
		clearFXChain((ChannelType) i);
	}
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
			modelListener->btnYesUpdate(uiObjectInfo.nValue, bIsFuncPressed);
			break;
		case BTN_NO:
			modelListener->btnNoUpdate(uiObjectInfo.nValue, bIsFuncPressed);
			break;
		case BTN_UP:
			modelListener->btnUpUpdate(bIsFuncPressed);
			break;
		case BTN_DOWN:
			modelListener->btnDownUpdate(bIsFuncPressed);
			break;
		case BTN_FOOT:
			modelListener->btnFootUpdate(uiObjectInfo.nID);
			break;
		case BTN_FUNC:
			switch(uiObjectInfo.nValue)
			{
			case 0:
				bIsFuncPressed = FALSE;
				break;
			case 1:
				bIsFuncPressed = TRUE;
				break;
			default:
				break;
			}
			break;
		case BTN_PARAM:
//			modelListener->btnParamUpdate(uiObjectInfo.nID);
			break;
		case BTN_PLAY:
//			modelListener->btnPlayUpdate(bIsFuncPressed);
			break;
		case BTN_STOP:
//			modelListener->btnStopUpdate(bIsFuncPressed);
			break;
		case BTN_REC:
//			modelListener->btnRecUpdate(bIsFuncPressed);
			break;
		case BTN_MENU:
//			modelListener->btnMenuUpdate();
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

void Model::clearFXChain(ChannelType channel)
{
	channelFXChainPosition[channel] = 0;

	for (int j = 0; j < MAX_EFFECTS_NUM; j++)
	{
		channelFXInfo[channel][j].eEffectNameID = T_EMPTYEFFECT;
		channelFXInfo[channel][j].nBitmapRegular = BITMAP_EMPTYPICT_ID;
		channelFXInfo[channel][j].nBitmapSelected = BITMAP_EMPTYSELECTEDPICT_ID;
		channelFXInfo[channel][j].nEffectsAmount = 0;

		for(int k = 0; k < MAX_PARAMETERS; k++)
		{
			effectParameters[channel][j][k] = 0;
		}
	}

	for (int j = 0; j < EFFECT_TYPES; j++)
	{
		channelFXPool[channel][j].bAvailable = TRUE;
		channelFXPool[channel][j].eEffectNameID = (TEXTS) (T_CHORUSEFFECT + j);

		// TODO: change all bitmaps when they are ready
		switch (channelFXPool[channel][j].eEffectNameID)
		{
		case T_CHORUSEFFECT:
		case T_COMPRESSOREFFECT:
		case T_DELAYEFFECT:
		case T_DISTORTIONEFFECT:
		case T_FLANGEREFFECT:
		case T_OVERDRIVEEFFECT:
		case T_PHASEREFFECT:
		case T_REVERBEFFECT:
		case T_TREMOLOEFFECT:
		case T_VIBRATOEFFECT:
		default:
			channelFXPool[channel][j].nBitmapRegular =
					BITMAP_EMPTYPICT_ID;
			channelFXPool[channel][j].nBitmapSelected =
					BITMAP_EMPTYSELECTEDPICT_ID;
			channelFXPool[channel][j].nEffectsAmount =
					MAX_PARAMETERS;
			break;
		}
	}
}
