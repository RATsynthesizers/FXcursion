#include <gui/effectsettings_screen/EffectSettingsView.hpp>
#include <gui/effectsettings_screen/EffectSettingsPresenter.hpp>

EffectSettingsPresenter::EffectSettingsPresenter(EffectSettingsView& v)
    : view(v)
{

}

void EffectSettingsPresenter::activate()
{

}

void EffectSettingsPresenter::deactivate()
{

}

void EffectSettingsPresenter::encMenuUpdate(S8 nValue)
{
	view.encMenuUpdate(nValue);
}

void EffectSettingsPresenter::encParamUpdate(U8 nID, S8 nValue)
{
	view.encParamUpdate(nID, nValue);
}

void EffectSettingsPresenter::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnYesUpdate(nValue, bIsFuncPressed);
}

void EffectSettingsPresenter::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	view.btnNoUpdate(nValue, bIsFuncPressed);
}

void EffectSettingsPresenter::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnUpUpdate(bIsFuncPressed);
}

void EffectSettingsPresenter::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	view.btnDownUpdate(bIsFuncPressed);
}

void EffectSettingsPresenter::btnFootUpdate(U8 nID)
{

}
