#include <gui/effectsettings_screen/EffectSettingsView.hpp>
#include <gui/effectsettings_screen/EffectSettingsPresenter.hpp>

EffectSettingsPresenter::EffectSettingsPresenter(EffectSettingsView &v) :
		view(v) {

}

void EffectSettingsPresenter::activate() {

}

void EffectSettingsPresenter::deactivate() {

}

void EffectSettingsPresenter::parameterChange(uint8_t id, int8_t scrollAmount) {
	view.parameterChange(id, scrollAmount);
}

void EffectSettingsPresenter::encScroll_action(int8_t scrollAmount) {
	view.encScroll_action(scrollAmount);
}

void EffectSettingsPresenter::btnYES_action(void) {
	view.btnYES_action();
}

void EffectSettingsPresenter::btnNO_action(void) {
	view.btnNO_action();
}

