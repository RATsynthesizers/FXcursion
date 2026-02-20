#include <gui/looper_screen/LooperView.hpp>
#include <gui/looper_screen/LooperPresenter.hpp>

LooperPresenter::LooperPresenter(LooperView& v)
    : view(v)
{

}

void LooperPresenter::activate()
{

}

void LooperPresenter::deactivate()
{

}
void LooperPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void LooperPresenter::encParamUpdate(U8 nID, S8 nValue)
{

}

void LooperPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue);
}

void LooperPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue);
}

void LooperPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate();
}

void LooperPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate();
}

void LooperPresenter::btnFootUpdate(U8 nID)
{

}

