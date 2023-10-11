#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern osMessageQueueId_t UpdateGUIQueueHandle;

Model::Model() :
		modelListener(0), settingsPosition(0), projectName(
				(const uint8_t*) "didactic-fx-journey") {

	for (int i = 0; i < WIRES_NUM; i++) {
		if (i == 0 || i == 4 || i == 8)
			wireConnected[i] = true;
		else
			wireConnected[i] = false;
	}

	for (int i = 0; i < CHANNELS_NUM; i++) {

		channelChainPosition[i] = 0;

		for (int j = 0; j < MAX_EFFECTS_NUM; j++) {
			channelEffectsArray[i][j].effectNameID = T_EMPTYEFFECT;
			channelEffectsArray[i][j].bitmapRegular = BITMAP_EMPTYPICT_ID;
			channelEffectsArray[i][j].bitmapSelected =
					BITMAP_EMPTYSELECTEDPICT_ID;
		}

		for (int j = 0; j < EFFECT_TYPES; j++) {
			channelEffectAmountsArray[i][j].available = true;
			channelEffectAmountsArray[i][j].effectNameID =
					(TEXTS) (T_CHORUSEFFECT + j);
			switch (channelEffectAmountsArray[i][j].effectNameID) {
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
				channelEffectAmountsArray[i][j].bitmapRegular =
						BITMAP_EMPTYPICT_ID;
				channelEffectAmountsArray[i][j].bitmapSelected =
						BITMAP_EMPTYSELECTEDPICT_ID;
				break;
			}
			channelEffectAmountsArray[i][j].associatedNumberRegular = -1;
			channelEffectAmountsArray[i][j].associatedNumberSelected = -1;
		}
	}
}

void Model::tick() {
	UPDATEGUIQUEUE_OBJ_t msg;
	osStatus_t status = osMessageQueueGet(UpdateGUIQueueHandle, &msg, NULL, 0U);

	if (status == osOK) {
		switch (msg.uiObject) {
		case BTN_YES_GUI:
			modelListener->btnYES_action();
			break;
		case BTN_NO_GUI:
			modelListener->btnNO_action();
			break;
		case ENC_GUI_SCROLL:
			modelListener->encScroll_action(msg.value);
			break;
		case ENC_GUI_PARAMETER:
			modelListener->parameterChange(msg.id, msg.value);
			break;
		default:
			break;
		}
	}
}
