#ifndef EFFECTLISTITEMSELECTED_HPP
#define EFFECTLISTITEMSELECTED_HPP

#include <gui_generated/containers/EffectListItemSelectedBase.hpp>

class EffectListItemSelected : public EffectListItemSelectedBase
{
public:
    EffectListItemSelected();
    virtual ~EffectListItemSelected() {}

    virtual void initialize();

    virtual void setEffect(TEXTS eEffectNameID);
protected:
};

#endif // EFFECTLISTITEMSELECTED_HPP
