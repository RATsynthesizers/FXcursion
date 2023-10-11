#ifndef EFFECTSLISTCONTAINER_HPP
#define EFFECTSLISTCONTAINER_HPP

#include <gui_generated/containers/EffectsListContainerBase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class EffectsListContainer : public EffectsListContainerBase
{
public:
    EffectsListContainer();
    virtual ~EffectsListContainer() {}

    virtual void initialize();

    virtual void scrollEffectsUpdateItem(EffectListItem& item, int16_t itemIndex);
    virtual void scrollEffectsUpdateCenterItem(EffectListItemSelected& item, int16_t itemIndex);

    virtual void scrollContents(int8_t scrollAmount);
    virtual void setAvailable(int8_t effectID, bool available);
    virtual uint8_t getCurrentPos();
    virtual EffectInfo getInfo(uint8_t effectID);
    virtual void setInfo(uint8_t effectID, EffectInfo effectInfo);

protected:
    EffectInfo effectInfoArray[EFFECT_TYPES];
    int8_t scrollDirection = 0;
    uint8_t currentPos = 0;
};

#endif // EFFECTSLISTCONTAINER_HPP
