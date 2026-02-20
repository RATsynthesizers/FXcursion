#ifndef EFFECTLISTITEM_HPP
#define EFFECTLISTITEM_HPP

#include <gui_generated/containers/EffectListItemBase.hpp>

class EffectListItem : public EffectListItemBase
{
public:
    EffectListItem();
    virtual ~EffectListItem() {}

    virtual void initialize();

    void setEffect(TEXTS eEffectNameID, BOOLEAN bAvailable);
protected:
};

#endif // EFFECTLISTITEM_HPP
