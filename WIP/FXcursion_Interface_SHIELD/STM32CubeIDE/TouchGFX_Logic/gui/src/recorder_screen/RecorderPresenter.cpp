#include <gui/recorder_screen/RecorderView.hpp>
#include <gui/recorder_screen/RecorderPresenter.hpp>

RecorderPresenter::RecorderPresenter(RecorderView& v)
    : view(v)
{

}

void RecorderPresenter::activate()
{

}

void RecorderPresenter::deactivate()
{

}

void RecorderPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void RecorderPresenter::encParamUpdate(U8 nID, S8 nValue)
{

}

void RecorderPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue);
}

void RecorderPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue);
}

void RecorderPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate();
}

void RecorderPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate();
}

void RecorderPresenter::btnFootUpdate(U8 nID)
{

}

