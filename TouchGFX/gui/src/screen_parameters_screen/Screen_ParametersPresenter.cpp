#include <gui/screen_parameters_screen/Screen_ParametersView.hpp>
#include <gui/screen_parameters_screen/Screen_ParametersPresenter.hpp>

Screen_ParametersPresenter::Screen_ParametersPresenter(Screen_ParametersView& v)
    : view(v)
{

}

void Screen_ParametersPresenter::activate()
{

}

void Screen_ParametersPresenter::deactivate()
{

}

void Screen_ParametersPresenter::scrollWindow(int8_t scrollAmount)
{

}

void Screen_ParametersPresenter::changeScreenRight()
{

}

void Screen_ParametersPresenter::changeScreenLeft()
{
	view.changeScreenLeft();
}

void Screen_ParametersPresenter::parameterChange()
{
	view.parameterChange();
}
