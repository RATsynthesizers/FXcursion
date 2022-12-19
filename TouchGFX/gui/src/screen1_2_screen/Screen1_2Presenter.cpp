#include <gui/screen1_2_screen/Screen1_2View.hpp>
#include <gui/screen1_2_screen/Screen1_2Presenter.hpp>

Screen1_2Presenter::Screen1_2Presenter(Screen1_2View& v)
    : view(v)
{

}

void Screen1_2Presenter::activate()
{
	view.currentMenuPos = MenuPosition[view.NESTED_LEVEL];
	view.setupScreen();
}

void Screen1_2Presenter::deactivate()
{

}

void Screen1_2Presenter::scrollWindow(int8_t scrollAmount)
{
	view.scrollWindow(scrollAmount);
}

void Screen1_2Presenter::changeScreenRight()
{
	MenuPosition[view.NESTED_LEVEL] = view.currentMenuPos;
	view.changeScreenRight();
}

void Screen1_2Presenter::changeScreenLeft()
{
	MenuPosition[view.NESTED_LEVEL] = 0;
	view.changeScreenLeft();
}

void Screen1_2Presenter::parameterChange()
{

}
