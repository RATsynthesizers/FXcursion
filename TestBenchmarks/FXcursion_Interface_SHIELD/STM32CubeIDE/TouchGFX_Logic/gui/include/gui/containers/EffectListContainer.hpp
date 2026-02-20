#ifndef EFFECTLISTCONTAINER_HPP
#define EFFECTLISTCONTAINER_HPP

#include <gui_generated/containers/EffectListContainerBase.hpp>

class EffectListContainer : public EffectListContainerBase
{
public:
    EffectListContainer();
    virtual ~EffectListContainer() {}

    virtual void initialize();


    void scrollEffectsUpdateItem(EffectListItem& item, S16 nItemIndex) override;
    void scrollEffectsUpdateCenterItem(EffectListItemSelected& item, S16 nItemIndex) override;

    void scrollContents(S8 nScrollAmount);
    void setAvailable(S8 nEffectID, BOOLEAN bAvailable);
    EffectInfo getInfo(U8 nEffectID);
    void setInfo(U8 nEffectID, EffectInfo effectInfo);
    U8 getCurrentPos();
    void beginInit();
    void endInit();
protected:

    void rebuildVisibleList();

    BOOLEAN bInitializing = FALSE;

    U8 visibleEffects[EFFECT_TYPES];
    U8 visibleCount = 0;

    EffectInfo effectInfoArray[EFFECT_TYPES];
    S8 nScrollDirection = 0;
    U8 nCurrentPos = 0;

};

#endif // EFFECTLISTCONTAINER_HPP
