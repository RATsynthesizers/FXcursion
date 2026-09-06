#include <gui/containers/EffectListItem.hpp>

EffectListItem::EffectListItem()
{

}

void EffectListItem::initialize()
{
    EffectListItemBase::initialize();
}

void EffectListItem::setEffect(TEXTS eEffectNameID, BOOLEAN bAvailable)
{
	Unicode::snprintf(textBuffer, TEXT_SIZE, "%s",
			touchgfx::TypedText(eEffectNameID).getText());
	grayBox.setVisible(!bAvailable);

	invalidate();
}
