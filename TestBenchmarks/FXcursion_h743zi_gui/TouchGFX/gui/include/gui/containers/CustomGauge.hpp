#ifndef CUSTOMGAUGE_HPP
#define CUSTOMGAUGE_HPP

#include <gui_generated/containers/CustomGaugeBase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class CustomGauge : public CustomGaugeBase
{
public:
    CustomGauge();
    virtual ~CustomGauge() {}

    virtual void initialize();

    virtual void changeValue(int8_t scrollAmount);
    virtual void setValue(uint8_t newValue);
    virtual uint8_t getValue();
    virtual void setParamName(TEXTS paramName);
protected:
    uint8_t rotationSpeed = 15;
    uint8_t value = 127;
};

#endif // CUSTOMGAUGE_HPP
