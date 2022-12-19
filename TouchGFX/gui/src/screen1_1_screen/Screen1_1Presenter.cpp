#include <gui/screen1_1_screen/Screen1_1View.hpp>
#include <gui/screen1_1_screen/Screen1_1Presenter.hpp>

Screen1_1Presenter::Screen1_1Presenter(Screen1_1View& v)
    : view(v)
{

}

void Screen1_1Presenter::activate()
{

}

void Screen1_1Presenter::deactivate()
{

}

void Screen1_1Presenter::scrollWindow(int8_t scrollAmount)
{
	view.scrollWindow(scrollAmount);
}

void Screen1_1Presenter::changeScreenRight()
{

}

void Screen1_1Presenter::changeScreenLeft()
{
	view.changeScreenLeft();
}

void Screen1_1Presenter::parameterChange()
{

}
