#ifndef STOMPBOARD_HPP
#define STOMPBOARD_HPP

#include <gui_generated/containers/StompBoardBase.hpp>

class StompBoard : public StompBoardBase
{
public:
    StompBoard();
    virtual ~StompBoard() {}

    virtual void initialize();
protected:
};

#endif // STOMPBOARD_HPP
