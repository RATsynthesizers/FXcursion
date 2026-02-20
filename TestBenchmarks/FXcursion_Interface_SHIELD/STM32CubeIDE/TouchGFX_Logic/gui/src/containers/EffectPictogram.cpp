#include <gui/containers/EffectPictogram.hpp>

EffectPictogram::EffectPictogram()
{

}

void EffectPictogram::initialize()
{
    EffectPictogramBase::initialize();
}

void EffectPictogram::setEffect(FXChainItemInfo newEffectInfo)
{
	effectInfo = newEffectInfo;

	pictRegular.setBitmap(touchgfx::Bitmap(effectInfo.nBitmapRegular));
	pictEditing.setBitmap(touchgfx::Bitmap(effectInfo.nBitmapSelected));

	invalidate();
}

FXChainItemInfo EffectPictogram::getEffect()
{
	return effectInfo;
}

void EffectPictogram::select(bool select)
{
	selectBox.setVisible(select);
	invalidate();
}

void EffectPictogram::edit(bool edit)
{
	editingBox.setVisible(edit);
	pictEditing.setVisible(edit);
	invalidate();
}
