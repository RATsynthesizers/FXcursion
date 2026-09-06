#include <gui/fxchain_screen/FXChainView.hpp>
#include <gui/fxchain_screen/FXChainPresenter.hpp>

FXChainPresenter::FXChainPresenter(FXChainView& v)
    : view(v)
{

}

void FXChainPresenter::activate()
{

}

void FXChainPresenter::deactivate()
{

}

void FXChainPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void FXChainPresenter::encParamUpdate(U8 nID, S8 nValue)
{

}

void FXChainPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue, bIsFuncPressed);
}

void FXChainPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue, bIsFuncPressed);
}

void FXChainPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate(bIsFuncPressed);
}

void FXChainPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate(bIsFuncPressed);
}

void FXChainPresenter::btnFootUpdate(U8 nID)
{

}
