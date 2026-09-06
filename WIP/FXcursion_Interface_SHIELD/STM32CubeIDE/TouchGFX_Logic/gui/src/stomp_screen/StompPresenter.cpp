#include <gui/stomp_screen/StompView.hpp>
#include <gui/stomp_screen/StompPresenter.hpp>

StompPresenter::StompPresenter(StompView& v)
    : view(v)
{

}

void StompPresenter::activate()
{

}

void StompPresenter::deactivate()
{

}

void StompPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void StompPresenter::encParamUpdate(U8 nID, S8 nValue)
{

}

void StompPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue);
}

void StompPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue);
}

void StompPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate();
}

void StompPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate();
}

void StompPresenter::btnFootUpdate(U8 nID)
{

}
