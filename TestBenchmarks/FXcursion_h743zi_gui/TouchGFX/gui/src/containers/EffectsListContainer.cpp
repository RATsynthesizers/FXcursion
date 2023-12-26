#include <gui/containers/EffectsListContainer.hpp>

EffectsListContainer::EffectsListContainer() {

}

void EffectsListContainer::initialize() {
	EffectsListContainerBase::initialize();
}

void EffectsListContainer::scrollContents(int8_t scrollAmount) {
	scrollDirection = scrollAmount;
	scrollEffects.animateToItem(scrollEffects.getSelectedItem() + scrollAmount,
			-1);
	currentPos = scrollEffects.getSelectedItem();
}

void EffectsListContainer::setAvailable(int8_t effectID, bool available) {

	effectInfoArray[effectID].available = available;

	if (effectInfoArray[effectID].associatedNumberRegular >= 0)
		scrollEffectsListItems[effectInfoArray[effectID].associatedNumberRegular].setEffect(
				effectInfoArray[effectID].effectNameID,
				effectInfoArray[effectID].available);

	if (effectInfoArray[effectID].associatedNumberSelected >= 0)
		scrollEffectsSelectedListItems[effectInfoArray[effectID].associatedNumberSelected].setEffect(
				effectInfoArray[effectID].effectNameID);
}

EffectInfo EffectsListContainer::getInfo(uint8_t effectID) {
	return effectInfoArray[effectID];
}

void EffectsListContainer::setInfo(uint8_t effectID, EffectInfo effectInfo) {
	effectInfoArray[effectID] = effectInfo;
}

uint8_t EffectsListContainer::getCurrentPos() {
	return currentPos;
}

void EffectsListContainer::scrollEffectsUpdateItem(EffectListItem &item,
		int16_t itemIndex) {
	// Override and implement this function in EffectsListContainer
	for (int i = 0; i < scrollEffectsListItems.getNumberOfDrawables(); i++)
		if (&item == &scrollEffectsListItems[i]) {
			if (i == 0) {
				if (itemIndex > 0
						&& effectInfoArray[itemIndex - 1].associatedNumberRegular
								== 0)
					effectInfoArray[itemIndex - 1].associatedNumberRegular =
							effectInfoArray[itemIndex].associatedNumberRegular;
				else
					effectInfoArray[itemIndex + 1].associatedNumberRegular =
							effectInfoArray[itemIndex].associatedNumberRegular;
			}
			effectInfoArray[itemIndex].associatedNumberRegular = i;
		}

	if (scrollDirection == 1) {
		for (int j = itemIndex - 1; j >= 0; j--)
			if (effectInfoArray[j].associatedNumberRegular
					== effectInfoArray[itemIndex].associatedNumberRegular)
				effectInfoArray[j].associatedNumberRegular = -1;
	} else {
		for (int j = itemIndex + 1; j < EFFECT_TYPES; j++)
			if (effectInfoArray[j].associatedNumberRegular
					== effectInfoArray[itemIndex].associatedNumberRegular)
				effectInfoArray[j].associatedNumberRegular = -1;
	}

	for (int i = itemIndex;
			i
					< MIN(
							itemIndex
									+ scrollEffectsListItems.getNumberOfDrawables(),
							EFFECT_TYPES); i++)
		if (effectInfoArray[i].associatedNumberRegular > -1)
			scrollEffectsListItems[effectInfoArray[i].associatedNumberRegular].setEffect(
					effectInfoArray[i].effectNameID,
					effectInfoArray[i].available);
}

void EffectsListContainer::scrollEffectsUpdateCenterItem(
		EffectListItemSelected &item, int16_t itemIndex) {
	// Override and implement this function in EffectsListContainer
	for (int i = 0; i < scrollEffectsSelectedListItems.getNumberOfDrawables();
			i++)
		if (&item == &scrollEffectsSelectedListItems[i]) {
			for (int j = 0; j < EFFECT_TYPES; j++)
				if (effectInfoArray[j].associatedNumberSelected == i)
					effectInfoArray[j].associatedNumberSelected = -1;
			effectInfoArray[itemIndex].associatedNumberSelected = i;
		}

	for (int i = 0; i < EFFECT_TYPES; i++)
		if (effectInfoArray[i].associatedNumberSelected > -1)
			scrollEffectsSelectedListItems[effectInfoArray[i].associatedNumberSelected].setEffect(
					effectInfoArray[i].effectNameID);
}
