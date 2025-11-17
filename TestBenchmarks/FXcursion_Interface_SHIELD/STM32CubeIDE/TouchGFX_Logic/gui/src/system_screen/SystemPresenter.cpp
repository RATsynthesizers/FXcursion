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

void SystemPresenter::btnYesUpdate(void)
{

}

void SystemPresenter::btnNoUpdate(void)
{

}

void SystemPresenter::btnUpUpdate(void)
{
	view.btnUpUpdate();
}

void SystemPresenter::btnDownUpdate(void)
{
	view.btnDownUpdate();
}

void SystemPresenter::btnFootUpdate(U8 nID)
{

}
