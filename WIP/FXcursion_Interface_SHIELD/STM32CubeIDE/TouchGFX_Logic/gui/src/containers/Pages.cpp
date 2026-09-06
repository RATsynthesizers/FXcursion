#include <gui/containers/Pages.hpp>

Pages::Pages()
{
	selectedColor = page1.getColor();
    normalColor = page2.getColor();
}

void Pages::initialize()
{
    PagesBase::initialize();
}

void Pages::setAmount(U8 effectsAmount)
{
	nPageSelected = 0;

	if(effectsAmount > 4)
	{
		nPagesAmount = 2;

		page0.setVisible(FALSE);

		page1.setColor(selectedColor);
		page1.setVisible(TRUE);

		page2.setColor(normalColor);
		page2.setVisible(TRUE);
	}
	else
	{
		nPagesAmount = 1;

		page0.setVisible(TRUE);
		page1.setVisible(FALSE);
		page2.setVisible(FALSE);
	}

	invalidate();
}

BOOLEAN Pages::pageUp()
{
	if(nPagesAmount > 1 && nPageSelected != 0)
	{
		nPageSelected = 0;

		page1.setColor(selectedColor);
		page2.setColor(normalColor);

		invalidate();

		return TRUE;
	}

	return FALSE;
}

BOOLEAN Pages::pageDown()
{
	if(nPagesAmount > 1 && nPageSelected != 1)
	{
		nPageSelected = 1;

		page1.setColor(normalColor);
		page2.setColor(selectedColor);

		invalidate();

		return TRUE;
	}

	return FALSE;
}

U8 Pages::getPage()
{
	return nPageSelected;
}
