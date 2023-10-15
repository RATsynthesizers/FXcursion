#include <gui/containers/EffectListItemSelected.hpp>

EffectListItemSelected::EffectListItemSelected() {

}

void EffectListItemSelected::initialize() {
	EffectListItemSelectedBase::initialize();
}

void EffectListItemSelected::setEffect(TEXTS effectNameID) {
	Unicode::snprintf(TextBuffer, TEXT_SIZE, "%s",
			touchgfx::TypedText(effectNameID).getText());

	invalidate();
}
