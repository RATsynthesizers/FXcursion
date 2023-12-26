#include <gui/projectsettings_screen/ProjectSettingsView.hpp>
#include <gui/projectsettings_screen/ProjectSettingsPresenter.hpp>

ProjectSettingsPresenter::ProjectSettingsPresenter(ProjectSettingsView& v)
    : view(v)
{

}

void ProjectSettingsPresenter::activate()
{

}

void ProjectSettingsPresenter::deactivate()
{

}

void ProjectSettingsPresenter::parameterChange(uint8_t id, int8_t scrollAmount) {
	view.parameterChange(id, scrollAmount);
}

void ProjectSettingsPresenter::encScroll_action(int8_t scrollAmount) {
	view.encScroll_action(scrollAmount);
}

void ProjectSettingsPresenter::btnYES_action(void) {
	view.btnYES_action();
}

void ProjectSettingsPresenter::btnNO_action(void) {
	view.btnNO_action();
}

