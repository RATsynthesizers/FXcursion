#ifndef STOMPBOARD_HPP
#define STOMPBOARD_HPP

#include <gui_generated/containers/StompBoardBase.hpp>

typedef enum enFootSwitches
{
	FOOT_SWITCH_1 = 0,
	FOOT_SWITCH_2 = 1,
	FOOT_SWITCH_3 = 2,

} FootSwitches;

class StompBoard : public StompBoardBase
{
public:
    StompBoard();
    virtual ~StompBoard() {}

    virtual void initialize();

    void select(FootSwitches eModuleNum);
    void deselect();
    FootSwitches getSelectedFootSwitch();
protected:

    FootSwitches eSelectedFootSwitch;
};

#endif // STOMPBOARD_HPP
