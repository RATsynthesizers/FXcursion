#ifndef EFFECTLISTITEM_HPP
#define EFFECTLISTITEM_HPP

#include <gui_generated/containers/EffectListItemBase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class EffectListItem : public EffectListItemBase
{
public:
    EffectListItem();
    virtual ~EffectListItem() {}

    virtual void initialize();

    virtual void setEffect(TEXTS effectNameID, bool available);
protected:
};

#endif // EFFECTLISTITEM_HPP
