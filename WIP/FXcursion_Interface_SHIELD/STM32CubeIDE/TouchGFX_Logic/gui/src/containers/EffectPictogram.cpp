#include <gui/containers/EffectPictogram.hpp>
#include <touchgfx/Color.hpp>

EffectPictogram::EffectPictogram()
{
	bSelected = false;
	bEditing  = false;
	bMoving   = false;

	/* Whatever the Designer configured, so this file is not a second opinion
	   on the normal colour. Same trick Pages uses for its page dots. */
	normalOutline = selectBoxPainter.getColor();

	/* The same blue StatusBar's moveBox uses, so "this is being moved" reads
	   as one idea across the screen rather than two unrelated highlights. */
	movingOutline = touchgfx::Color::getColorFromRGB(21, 0, 255);
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
	setState(select, bEditing, bMoving);
}

void EffectPictogram::edit(bool edit)
{
	setState(bSelected, edit, bMoving);
}

void EffectPictogram::move(bool move)
{
	setState(bSelected, bEditing, move);
}

void EffectPictogram::setState(bool bSelect, bool bEdit, bool bMove)
{
	if ((bSelect == bSelected) && (bEdit == bEditing) && (bMove == bMoving))
	{
		/* Nothing to repaint. Lets the view re-apply the whole row on every
		   step without paying for the three that did not change. */
		return;
	}

	bSelected = bSelect;
	bEditing  = bEdit;
	bMoving   = bMove;

	applyState();
}

void EffectPictogram::applyState()
{
	selectBox.setVisible(bSelected);

	editingBox.setVisible(bEditing);
	pictEditing.setVisible(bEditing);

	selectBoxPainter.setColor(bMoving ? movingOutline : normalOutline);

	invalidate();
}
