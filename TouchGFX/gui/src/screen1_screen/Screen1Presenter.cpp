#include <gui/screen1_screen/Screen1View.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>

Screen1Presenter::Screen1Presenter(Screen1View& v)
    : view(v)
{

}

void Screen1Presenter::activate()
{
	view.currentMenuPos = MenuPosition[view.NESTED_LEVEL];
	view.setupScreen();
}

void Screen1Presenter::deactivate()
{

}

void Screen1Presenter::scrollWindow(int8_t scrollAmount)
{
	view.scrollWindow(scrollAmount);
}

void Screen1Presenter::changeScreenRight()
{
	MenuPosition[view.NESTED_LEVEL] = view.currentMenuPos;
	view.changeScreenRight();
}

void Screen1Presenter::changeScreenLeft()
{

}

void Screen1Presenter::parameterChange()
{

}
