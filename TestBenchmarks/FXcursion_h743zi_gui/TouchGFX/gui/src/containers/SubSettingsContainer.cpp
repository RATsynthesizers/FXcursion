#include <gui/containers/SubSettingsContainer.hpp>

SubSettingsContainer::SubSettingsContainer() {

}

void SubSettingsContainer::initialize() {
	SubSettingsContainerBase::initialize();
}

void SubSettingsContainer::scrollContents(int8_t scrollAmount) {
	int newSelectedPosition = currentPos + scrollAmount;
	while (newSelectedPosition >= 0 && newSelectedPosition < 3
			&& subMenuItems[newSelectedPosition]->isGray)
		newSelectedPosition += scrollAmount;
	if (newSelectedPosition >= 0 && newSelectedPosition < 3) {
		subMenuItems[currentPos]->deSelect();
		subMenuItems[newSelectedPosition]->select();
		currentPos = newSelectedPosition;
	}
}

void SubSettingsContainer::resetSubMenu(uint8_t selectedChain,
		bool grayDeleteWire, bool grayCreateWire) {

	const uint8_t text[20] = "";
	const uint8_t io[30] = "";
	if (selectedChain < 3) {
		strcpy((char*) io, "Input ");
		char buf[2];
		sprintf(buf, "%d", selectedChain + 1);
		strcat((char*) io, buf);
	} else {
		strcpy((char*) io, "Output ");
		char buf[2];
		sprintf(buf, "%d", (selectedChain - 3) + 1);
		strcat((char*) io, buf);
	}
	Unicode::toUTF8(touchgfx::TypedText(T_SELECTCHAIN).getText(), (uint8_t*) text, 20);
	strcat((char*) io, (char*) text);
	subMenuItems[0]->setText(io);

	Unicode::toUTF8(touchgfx::TypedText(T_CONNECTWIRE).getText(), (uint8_t*) text, 20);
	subMenuItems[1]->setText(text);
	if (grayCreateWire)
		subMenuItems[1]->setGray();
	else
		subMenuItems[1]->resetGray();

	Unicode::toUTF8(touchgfx::TypedText(T_DELETEWIRE).getText(), (uint8_t*) text, 20);
	subMenuItems[2]->setText(text);
	if (grayDeleteWire)
		subMenuItems[2]->setGray();
	else
		subMenuItems[2]->resetGray();

	subMenuItems[currentPos]->deSelect();
	subMenuItems[0]->select();
	currentPos = 0;
}

uint8_t SubSettingsContainer::getCurrentPos() {
	return currentPos;
}
