#include <gui/containers/SubMenuItem.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>

SubMenuItem::SubMenuItem() {

}

void SubMenuItem::initialize() {
	SubMenuItemBase::initialize();
}

void SubMenuItem::select() {
	isSelected = true;
	whiteBox.setVisible(!isSelected);
	Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	whiteBox.invalidate();

}

void SubMenuItem::deSelect() {
	isSelected = false;
	whiteBox.setVisible(!isSelected);
	Text.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
	whiteBox.invalidate();
}

void SubMenuItem::setGray() {
	isGray = true;
	grayBox.setVisible(isGray);
	grayBox.invalidate();
}

void SubMenuItem::resetGray() {
	isGray = false;
	grayBox.setVisible(isGray);
	grayBox.invalidate();
}

void SubMenuItem::setText(const uint8_t *text) {
	Unicode::fromUTF8(text, TextBuffer, TEXT_SIZE);
	Text.invalidate();
}

