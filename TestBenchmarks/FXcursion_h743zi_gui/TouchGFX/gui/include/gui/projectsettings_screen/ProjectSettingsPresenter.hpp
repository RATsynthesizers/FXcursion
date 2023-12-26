#ifndef PROJECTSETTINGSPRESENTER_HPP
#define PROJECTSETTINGSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ProjectSettingsView;

class ProjectSettingsPresenter: public touchgfx::Presenter, public ModelListener {
public:
	ProjectSettingsPresenter(ProjectSettingsView &v);

	/**
	 * The activate function is called automatically when this screen is "switched in"
	 * (ie. made active). Initialization logic can be placed here.
	 */
	virtual void activate();

	/**
	 * The deactivate function is called automatically when this screen is "switched out"
	 * (ie. made inactive). Teardown functionality can be placed here.
	 */
	virtual void deactivate();

	virtual ~ProjectSettingsPresenter() {
	}
	;

	void parameterChange(uint8_t id, int8_t scrollAmount);
	void encScroll_action(int8_t scrollAmount);
	void btnYES_action(void);
	void btnNO_action(void);

	void saveChannelNum(uint8_t saveChannelNumber) {
		model->saveChannelNum(saveChannelNumber);
	}

	void saveSettingsPosition(uint8_t settingsPos) {
		model->saveSettingsPosition(settingsPos);
	}

	uint8_t getSettingsPosition() {
		return model->getSettingsPosition();
	}

	void saveProjectName(const uint8_t* saveProjectName) {
		model->saveProjectName(saveProjectName);
	}

	const uint8_t* getProjectName() {
		return model->getProjectName();
	}

	void saveWire(uint8_t wireID, bool isConnected) {
		model->saveWire(wireID, isConnected);
	}
	bool getWire(uint8_t wireID) {
		return model->getWire(wireID);
	}

private:
	ProjectSettingsPresenter();

	ProjectSettingsView &view;
};

#endif // PROJECTSETTINGSPRESENTER_HPP
