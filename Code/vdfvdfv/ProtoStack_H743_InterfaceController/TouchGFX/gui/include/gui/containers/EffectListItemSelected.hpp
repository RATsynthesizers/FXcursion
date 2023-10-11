#ifndef EFFECTLISTITEMSELECTED_HPP
#define EFFECTLISTITEMSELECTED_HPP

#include <gui_generated/containers/EffectListItemSelectedBase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class EffectListItemSelected : public EffectListItemSelectedBase
{
public:
    EffectListItemSelected();
    virtual ~EffectListItemSelected() {}

    virtual void initialize();

    virtual void setEffect(TEXTS effectNameID);
protected:
};

#endif // EFFECTLISTITEMSELECTED_HPP
