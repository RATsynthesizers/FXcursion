#ifndef WIRINGCHANGEBOX_HPP
#define WIRINGCHANGEBOX_HPP

#include <gui_generated/containers/WiringChangeBoxBase.hpp>

class WiringChangeBox : public WiringChangeBoxBase
{
public:
    WiringChangeBox();
    virtual ~WiringChangeBox() {}

    virtual void initialize();
    virtual void setText(uint8_t connectDelete, uint8_t i);
protected:
};

#endif // WIRINGCHANGEBOX_HPP
