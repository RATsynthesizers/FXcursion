#include <gui/containers/SettingsItem.hpp>
#include <touchgfx/Color.hpp>

SettingsItem::SettingsItem() {

}

void SettingsItem::initialize() {
	SettingsItemBase::initialize();
}

void SettingsItem::setName(const uint8_t *text) {
	Unicode::fromUTF8(text, TextBuffer, TEXT_SIZE);
	Text.invalidate();
}

void SettingsItem::selectItem(void) {
	whiteBox.setVisible(false);
	whiteBox.invalidate();

	Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	Text.invalidate();
}

void SettingsItem::deselectItem(void) {
	whiteBox.setVisible(true);
	whiteBox.invalidate();

	Text.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
	Text.invalidate();
}

void SettingsItem::selectConnection(void) {
	orangeBox.setVisible(true);
	orangeBox.invalidate();
}

void SettingsItem::deselectConnection(void) {
	orangeBox.setVisible(false);
	orangeBox.invalidate();
}
