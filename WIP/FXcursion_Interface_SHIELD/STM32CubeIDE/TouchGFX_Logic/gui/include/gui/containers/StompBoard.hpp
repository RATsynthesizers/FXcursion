#ifndef STOMPBOARD_HPP
#define STOMPBOARD_HPP

#include <gui_generated/containers/StompBoardBase.hpp>

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
