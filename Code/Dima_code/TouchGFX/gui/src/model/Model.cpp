#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "main.h"
#include "cmsis_os.h"

extern osMessageQueueId_t UpdateGUIQueueHandle;

Model::Model() :
		modelListener(0) {
	for (int i = 0; i < GUI_NESTING_LEVELS_NUMBER - 1; i++)
		modelListener->MenuPosition[i] = 0;
}

void Model::tick() {
#ifndef SIMULATOR
	UPDATEGUIQUEUE_OBJ_t msg;
	osStatus_t status = osMessageQueueGet(UpdateGUIQueueHandle, &msg, NULL, 0U);

	if (status == osOK) {
		switch(msg.uiObject)
		{
		case BTN_YES_GUI:
			modelListener->changeScreenRight();
			break;
		case BTN_NO_GUI:
			modelListener->changeScreenLeft();
			break;
		case ENC_GUI_PARAMETER:
			modelListener->parameterChange();
			break;
		case ENC_GUI_SCROLL:
			modelListener->scrollWindow(msg.value);
			break;
		default:
			break;
		}
	}
//	osThreadYield();
#endif /*SIMULATOR*/
}
