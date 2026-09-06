#include <gui/input_screen/InputView.hpp>

InputView::InputView()
{

}

void InputView::setupScreen()
{
    InputViewBase::setupScreen();
}

void InputView::tearDownScreen()
{
    InputViewBase::tearDownScreen();
}

void InputView::encMenuUpdate(S8 nValue)
{

}

void InputView::btnYesUpdate(S8 nValue)
{

}

void InputView::btnNoUpdate(S8 nValue)
{
	application().gotoSystemScreenNoTransition();
}

void InputView::btnUpUpdate()
{

}

void InputView::btnDownUpdate()
{

}
