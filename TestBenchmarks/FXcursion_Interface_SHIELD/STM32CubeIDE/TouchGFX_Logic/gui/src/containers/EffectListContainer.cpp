#include <gui/containers/EffectListContainer.hpp>

EffectListContainer::EffectListContainer()
{
	memset(visibleEffects, 0, sizeof(visibleEffects));
}

void EffectListContainer::initialize()
{
    EffectListContainerBase::initialize();
}

void EffectListContainer::beginInit()
{
	bInitializing = TRUE;
}

void EffectListContainer::endInit()
{
	/* The guard that used to be here tested bInitializing on the line after
	   clearing it, so it was always true. Dropped - the rebuild is
	   unconditional by definition at the end of a batch. */
	bInitializing = FALSE;

	rebuildVisibleList();
}

void EffectListContainer::scrollContents(S8 nScrollAmount)
{
    int nextIndex = scrollEffects.getSelectedItem() + nScrollAmount;

    if(nextIndex < 0 || nextIndex >= visibleCount)
        return;

    scrollEffects.animateToItem(nextIndex);
    scrollEffects.stopAnimation();
    nCurrentPos = nextIndex;
}

void EffectListContainer::setAvailable(S8 nEffectID,
									   BOOLEAN bAvailable)
{
	effectInfoArray[nEffectID].bAvailable = bAvailable;
	if (!bInitializing)
	{
		rebuildVisibleList();
	}
}

EffectInfo EffectListContainer::getInfo(U8 nEffectID)
{
	return effectInfoArray[nEffectID];
}

void EffectListContainer::setInfo(U8 nEffectID,
								  EffectInfo effectInfo)
{
	effectInfoArray[nEffectID] = effectInfo;
	if (!bInitializing)
	{
		rebuildVisibleList();
	}
}

U8 EffectListContainer::getCurrentPos()
{
	return visibleEffects[nCurrentPos];
}

void EffectListContainer::rebuildVisibleList()
{
    visibleCount = 0;

    // rebuild mapping based on availability
    for(int i = 0; i < EFFECT_TYPES; i++)
    {
        if(effectInfoArray[i].bAvailable)
        {
            visibleEffects[visibleCount++] = i;
        }
    }

    // tell scrollwheel new size
    scrollEffects.setNumberOfItems(visibleCount);
//
//    // force redraw
//    scrollEffects.invalidate();
}

void EffectListContainer::scrollEffectsUpdateItem(EffectListItem &item,
												  S16 nItemIndex)
{
    if(nItemIndex < 0 || nItemIndex >= visibleCount)
        return;

    uint8_t realID = visibleEffects[nItemIndex];

    item.setEffect(effectInfoArray[realID].eEffectNameID,
    			   TRUE);

    item.invalidate();
}

void EffectListContainer::scrollEffectsUpdateCenterItem(
		EffectListItemSelected &item, S16 nItemIndex)
{
    if(nItemIndex < 0 || nItemIndex >= visibleCount)
        return;

    uint8_t realID = visibleEffects[nItemIndex];

    item.setEffect(effectInfoArray[realID].eEffectNameID);

    item.invalidate();
}
