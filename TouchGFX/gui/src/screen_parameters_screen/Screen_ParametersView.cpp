#include <gui/screen_parameters_screen/Screen_ParametersView.hpp>
#include "cmsis_os.h"

extern osSemaphoreId_t requestParamValuesSemHandle;
extern osMessageQueueId_t GaugeValuesQueueHandle;

Screen_ParametersView::Screen_ParametersView()
{
	modifier = 1;
}

void Screen_ParametersView::setupScreen()
{
    Screen_ParametersViewBase::setupScreen();
}

void Screen_ParametersView::tearDownScreen()
{
    Screen_ParametersViewBase::tearDownScreen();
}

void Screen_ParametersView::changeScreenLeft()
{
	application().gotoScreen1_2ScreenSlideTransitionWest();
}

void Screen_ParametersView::handleTickEvent()
{

}

void Screen_ParametersView::parameterChange()
{

	UPDATEGAUGEVALUES_OBJ_t updateGaugeValues_msg;
	osStatus_t status = osMessageQueueGet(GaugeValuesQueueHandle, &updateGaugeValues_msg, 0U, 10);
	if(status == osOK)
		gauge[updateGaugeValues_msg.gaugeNum].setValue(updateGaugeValues_msg.value*100);

}
