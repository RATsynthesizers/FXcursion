#include <gui/containers/FXChainItem.hpp>
#include <texts/TextKeysAndLanguages.hpp>

FXChainItem::FXChainItem()
{
	/* Captured from whatever the Designer put in the Base, so this file is not
	   a second opinion on the normal colour. */
	activeColor = whiteBox.getColor();

	/* The same grey AddModuleWindow::blockSelect already uses for "you cannot
	   have this" - one disabled colour across the app rather than two that
	   nearly match. */
	bypassedColor = touchgfx::Color::getColorFromRGB(128, 128, 128);
}

void FXChainItem::initialize()
{
    FXChainItemBase::initialize();
}

void FXChainItem::setEffect(FXChainItemInfo newEffectInfo) {

	effectInfo = newEffectInfo;

	Unicode::snprintf(effectNameBuffer, EFFECTNAME_SIZE, "%s",
			touchgfx::TypedText(effectInfo.eEffectNameID).getText());
	pictRegular.setBitmap(touchgfx::Bitmap(effectInfo.nBitmapRegular));
	pictSelected.setBitmap(touchgfx::Bitmap(effectInfo.nBitmapSelected));
	blackBox.setVisible(effectInfo.eEffectNameID != T_EMPTYEFFECT);

	/*
	 * Bypass rides along with the effect data rather than being a separate
	 * call, so a reorder repaints it correctly for free - swapEffects only
	 * moves FXChainItemInfo values and calls setEffect.
	 */
	whiteBox.setColor((FALSE != effectInfo.bBypassed) ? bypassedColor
	                                                  : activeColor);

	if (isSelected)
	{
		effectName.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
		whiteBox.setVisible(false);
		dashRegular.setVisible(false);
	}
	else
	{
		effectName.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		whiteBox.setVisible(effectInfo.eEffectNameID != T_EMPTYEFFECT);
		dashRegular.setVisible(true);
	}

	pictRegular.setVisible(!isSelected);

	invalidate();
}

void FXChainItem::select(bool select) {

	isSelected = select;
	if (isSelected)
	{
		effectName.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	}
	else
	{
		effectName.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
	}

	pictRegular.setVisible(!isSelected);
	if (effectInfo.eEffectNameID == T_EMPTYEFFECT)
	{
		dashRegular.setVisible(!isSelected);
	}
	else
	{
		whiteBox.setVisible(!isSelected);
	}

	invalidate();
}
