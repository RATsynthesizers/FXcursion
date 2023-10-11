#include <gui/containers/EffectListItem.hpp>

EffectListItem::EffectListItem()
{

}

void EffectListItem::initialize()
{
    EffectListItemBase::initialize();
}

void EffectListItem::setEffect(TEXTS effectNameID, bool available)
{
	Unicode::snprintf(TextBuffer, TEXT_SIZE, "%s",
			touchgfx::TypedText(effectNameID).getText());
	grayBox.setVisible(!available);

	invalidate();
}
