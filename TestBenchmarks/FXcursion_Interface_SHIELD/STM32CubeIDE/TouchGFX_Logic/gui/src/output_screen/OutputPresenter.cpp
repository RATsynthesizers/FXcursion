#include <gui/output_screen/OutputView.hpp>
#include <gui/output_screen/OutputPresenter.hpp>

OutputPresenter::OutputPresenter(OutputView& v)
    : view(v)
{

}

void OutputPresenter::activate()
{

}

void OutputPresenter::deactivate()
{

}

void OutputPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void OutputPresenter::encParamUpdate(U8 nID, S8 nValue)
{

}

void OutputPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue);
}

void OutputPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue);
}

void OutputPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate();
}

void OutputPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate();
}

void OutputPresenter::btnFootUpdate(U8 nID)
{

}

