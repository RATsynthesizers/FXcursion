#include <gui/containers/FXChainItem.hpp>
#include <texts/TextKeysAndLanguages.hpp>

FXChainItem::FXChainItem()
{

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
