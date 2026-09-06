#include <gui/system_screen/SystemView.hpp>
#include <gui/system_screen/SystemPresenter.hpp>

SystemPresenter::SystemPresenter(SystemView& v)
    : view(v)
{

}

void SystemPresenter::activate()
{

}

void SystemPresenter::deactivate()
{

}

void SystemPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void SystemPresenter::encParamUpdate(U8 nID, S8 nValue)
{

}

void SystemPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue, bIsFuncPressed);
}

void SystemPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue, bIsFuncPressed);
}

void SystemPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate(bIsFuncPressed);
}

void SystemPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate(bIsFuncPressed);
}

void SystemPresenter::btnFootUpdate(U8 nID)
{

}
