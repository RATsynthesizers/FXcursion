#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Color.hpp>

Screen1View::Screen1View()
{

}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::scrollWindow(int8_t scrollAmount)
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

void Screen1View::changeScreenRight()
{
	if(currentMenuPos == 0)
		application().gotoScreen1_1ScreenSlideTransitionWest();
	else if(currentMenuPos == 1)
		application().gotoScreen1_2ScreenSlideTransitionWest();
}

void Screen1View::handleTickEvent()
{

}
