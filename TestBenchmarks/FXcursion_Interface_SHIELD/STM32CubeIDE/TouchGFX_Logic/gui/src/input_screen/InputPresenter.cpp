#include <gui/input_screen/InputView.hpp>
#include <gui/input_screen/InputPresenter.hpp>

InputPresenter::InputPresenter(InputView& v)
    : view(v)
{

}

void InputPresenter::activate()
{

}

void InputPresenter::deactivate()
{

}

void InputPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void InputPresenter::encParamUpdate(U8 nID, S8 nValue)
{

}

void InputPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue);
}

void InputPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue);
}

void InputPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate();
}

void InputPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate();
}

void InputPresenter::btnFootUpdate(U8 nID)
{

}

