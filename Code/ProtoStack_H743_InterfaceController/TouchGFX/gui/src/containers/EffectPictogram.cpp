#include <gui/containers/EffectPictogram.hpp>

EffectPictogram::EffectPictogram()
{

}

void EffectPictogram::initialize()
{
    EffectPictogramBase::initialize();
}

void EffectPictogram::setEffect(MenuItemInfo newEffectInfo)
{
	effectInfo = newEffectInfo;

	pictRegular.setBitmap(touchgfx::Bitmap(effectInfo.bitmapRegular));
	pictEditing.setBitmap(touchgfx::Bitmap(effectInfo.bitmapSelected));

	invalidate();
}

MenuItemInfo EffectPictogram::getEffect()
{
	return effectInfo;
}

void EffectPictogram::select(bool select)
{
	selectingBox.setVisible(select);
	invalidate();
}

void EffectPictogram::edit(bool edit)
{
	editingBox.setVisible(edit);
	pictEditing.setVisible(edit);
	invalidate();
}
