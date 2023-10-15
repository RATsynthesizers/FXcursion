#include <gui/containers/MenuItem.hpp>

MenuItem::MenuItem() {

}

void MenuItem::initialize() {
	MenuItemBase::initialize();
}

void MenuItem::setEffect(MenuItemInfo newEffectInfo) {

	effectInfo = newEffectInfo;

	Unicode::snprintf(TextBuffer, TEXT_SIZE, "%s",
			touchgfx::TypedText(effectInfo.effectNameID).getText());
	pictogramRegular.setBitmap(touchgfx::Bitmap(effectInfo.bitmapRegular));
	pictogramSelected.setBitmap(touchgfx::Bitmap(effectInfo.bitmapSelected));
	blackBox.setVisible(effectInfo.effectNameID != T_EMPTYEFFECT);

	if (isSelected)
	{
		Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
		whiteBox.setVisible(false);
		dashedImage.setVisible(false);
	}
	else
	{
		Text.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		whiteBox.setVisible(effectInfo.effectNameID != T_EMPTYEFFECT);
		dashedImage.setVisible(true);
	}

	pictogramRegular.setVisible(!isSelected);

	invalidate();
}

void MenuItem::select(bool select) {

	isSelected = select;

	if (isSelected)
		Text.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	else
		Text.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));

	pictogramRegular.setVisible(!isSelected);
	if (effectInfo.effectNameID == T_EMPTYEFFECT)
		dashedImage.setVisible(!isSelected);
	else
		whiteBox.setVisible(!isSelected);

	invalidate();
}

