#include <gui/containers/StompBoard.hpp>

StompBoard::StompBoard()
{
	eSelectedFootSwitch = FOOT_SWITCH_1;
}

void StompBoard::initialize()
{
    StompBoardBase::initialize();
}

void StompBoard::select(FootSwitches eModuleNum)
{
	switch(eModuleNum)
	{
	case FOOT_SWITCH_1:
		eSelectedFootSwitch = FOOT_SWITCH_1;
		stomp1select.setVisible(true);
		stomp2select.setVisible(false);
		stomp3select.setVisible(false);
		break;

	case FOOT_SWITCH_2:
		eSelectedFootSwitch = FOOT_SWITCH_2;
		stomp1select.setVisible(false);
		stomp2select.setVisible(true);
		stomp3select.setVisible(false);
		break;

	case FOOT_SWITCH_3:
		eSelectedFootSwitch = FOOT_SWITCH_3;
		stomp1select.setVisible(false);
		stomp2select.setVisible(false);
		stomp3select.setVisible(true);
		break;

	default:
		break;
	}

	stomp1select.invalidate();
	stomp2select.invalidate();
	stomp3select.invalidate();
}

void StompBoard::deselect()
{
	stomp1select.setVisible(false);
	stomp2select.setVisible(false);
	stomp3select.setVisible(false);

	stomp1select.invalidate();
	stomp2select.invalidate();
	stomp3select.invalidate();
}

FootSwitches StompBoard::getSelectedFootSwitch()
{
	return eSelectedFootSwitch;
}
