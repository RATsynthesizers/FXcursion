#include <gui/mixer_screen/MixerView.hpp>
#include <gui/mixer_screen/MixerPresenter.hpp>

MixerPresenter::MixerPresenter(MixerView& v)
    : view(v)
{

}

void MixerPresenter::activate()
{

}

void MixerPresenter::deactivate()
{

}

void MixerPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void MixerPresenter::encParamUpdate(U8 nID, S8 nValue)
{

}

void MixerPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue);
}

void MixerPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue);
}

void MixerPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate();
}

void MixerPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate();
}

void MixerPresenter::btnFootUpdate(U8 nID)
{

}
