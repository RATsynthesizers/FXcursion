#include <gui/containers/SubMenuContainer.hpp>

SubMenuContainer::SubMenuContainer() {

}

void SubMenuContainer::initialize() {
	subMenuItems[currentPos]->select();

	const uint8_t text[30] = "";
	for (int i = 0; i < 4; i++) {
		Unicode::toUTF8(
				touchgfx::TypedText(T_CH1_SUBMENU0 + (TEXTS) i).getText(),
				(uint8_t*) text, 30);
		subMenuItems[i]->setText(text);
	}
	SubMenuContainerBase::initialize();
}

void SubMenuContainer::scrollContents(int8_t scrollAmount) {
	if (!isDeletingEffect) {
		int newSelectedPosition = currentPos + scrollAmount;
		while (newSelectedPosition >= 0 && newSelectedPosition < 4
				&& subMenuItems[newSelectedPosition]->isGray)
			newSelectedPosition += scrollAmount;
		if (newSelectedPosition >= 0 && newSelectedPosition < 4) {
			subMenuItems[currentPos]->deSelect();
			subMenuItems[newSelectedPosition]->select();
			currentPos = newSelectedPosition;
		}
	}
}

void SubMenuContainer::resetSubMenu(bool isFirst, bool isLast) {

	if (isFirst)
		subMenuItems[1]->setGray();
	else
		subMenuItems[1]->resetGray();

	if (isLast)
		subMenuItems[2]->setGray();
	else
		subMenuItems[2]->resetGray();

	subMenuItems[currentPos]->deSelect();
	subMenuItems[0]->select();
	currentPos = 0;
}

uint8_t SubMenuContainer::getCurrentPos() {
	return currentPos;
}

bool SubMenuContainer::getIsDeletingEffect() {
	return isDeletingEffect;
}

void SubMenuContainer::openDeletingEffect(TEXTS effectNameID) {
	isDeletingEffect = true;
	modalWindowDelete.setEffectName(effectNameID);
	modalWindowDelete.setVisible(isDeletingEffect);
	invalidate();
}

void SubMenuContainer::closeDeletingEffect() {
	isDeletingEffect = false;
	modalWindowDelete.setVisible(isDeletingEffect);
	invalidate();
}
