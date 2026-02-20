#include <gui/containers/EffectListItemSelected.hpp>

EffectListItemSelected::EffectListItemSelected()
{

}

void EffectListItemSelected::initialize()
{
    EffectListItemSelectedBase::initialize();
}

void EffectListItemSelected::setEffect(TEXTS eEffectNameID)
{
	Unicode::snprintf(textBuffer, TEXT_SIZE, "%s",
			touchgfx::TypedText(eEffectNameID).getText());

	invalidate();
}
