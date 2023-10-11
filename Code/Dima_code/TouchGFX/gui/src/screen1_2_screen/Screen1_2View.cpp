#include <gui/screen1_2_screen/Screen1_2View.hpp>
#include <touchgfx/Color.hpp>

Screen1_2View::Screen1_2View()
{

}

void Screen1_2View::setupScreen()
{
    Screen1_2ViewBase::setupScreen();
}

void Screen1_2View::tearDownScreen()
{
    Screen1_2ViewBase::tearDownScreen();
}

void Screen1_2View::scrollWindow(int8_t scrollAmount)
{
	MenuBoxes[currentMenuPos].setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
	MenuBoxes[currentMenuPos].invalidate();

	MenuTexts[currentMenuPos].setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	MenuTexts[currentMenuPos].invalidate();

	if(currentMenuPos + scrollAmount >= 0 && currentMenuPos + scrollAmount < NUMBER_OF_OPTIONS)
		currentMenuPos = currentMenuPos + scrollAmount;
	else if (currentMenuPos + scrollAmount < 0 && currentMenuPos > 0)
		currentMenuPos = 0;
	else if(currentMenuPos + scrollAmount >= NUMBER_OF_OPTIONS && currentMenuPos < NUMBER_OF_OPTIONS - 1)
		currentMenuPos = NUMBER_OF_OPTIONS - 1;

	MenuBoxes[currentMenuPos].setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
	MenuBoxes[currentMenuPos].invalidate();

	MenuTexts[currentMenuPos].setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
	MenuTexts[currentMenuPos].invalidate();

	if(currentMenuPos + scrollAmount >= 0 && currentMenuPos + scrollAmount < NUMBER_OF_OPTIONS)
	{
		if (MenuBoxes[currentMenuPos].getY() < 0)
			ScrollableMenu.doScroll(0, -MenuBoxes[currentMenuPos].getY());
		else if (MenuBoxes[currentMenuPos].getY() + MENU_ITEM_HEIGHT > SCREEN_HEIGHT)
			ScrollableMenu.doScroll(0, SCREEN_HEIGHT - MenuBoxes[currentMenuPos].getY() - MENU_ITEM_HEIGHT);
	}
	else if (currentMenuPos + scrollAmount < 0 && currentMenuPos > 0)
	{
		if (MenuBoxes[currentMenuPos].getY() < 0)
			ScrollableMenu.doScroll(0, -MenuBoxes[currentMenuPos].getY());
	}
	else if(currentMenuPos + scrollAmount >= NUMBER_OF_OPTIONS && currentMenuPos < NUMBER_OF_OPTIONS - 1)
	{
		if (MenuBoxes[currentMenuPos].getY() + MENU_ITEM_HEIGHT > SCREEN_HEIGHT)
			ScrollableMenu.doScroll(0, SCREEN_HEIGHT - MenuBoxes[currentMenuPos].getY() - MENU_ITEM_HEIGHT);
	}
}

void Screen1_2View::changeScreenLeft()
{
	application().gotoScreen1ScreenSlideTransitionEast();
}

void Screen1_2View::changeScreenRight()
{
	if(currentMenuPos == 4)
		application().gotoScreen_ParametersScreenSlideTransitionWest();
}
