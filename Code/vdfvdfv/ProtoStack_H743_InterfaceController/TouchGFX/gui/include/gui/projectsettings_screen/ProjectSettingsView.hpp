#ifndef PROJECTSETTINGSVIEW_HPP
#define PROJECTSETTINGSVIEW_HPP

#include <gui_generated/projectsettings_screen/ProjectSettingsViewBase.hpp>
#include <gui/projectsettings_screen/ProjectSettingsPresenter.hpp>

enum ConnectionModes {
	NO_ACTION = 0,
	CONNECT,
	DELETE
};

class ProjectSettingsView: public ProjectSettingsViewBase {
public:
	ProjectSettingsView();
	virtual ~ProjectSettingsView() {
	}
	;
	virtual void setupScreen();
	virtual void tearDownScreen();



	void parameterChange(uint8_t id, int8_t scrollAmount);
	void encScroll_action(int8_t scrollAmount);
	void btnYES_action(void);
	void btnNO_action(void);
protected:
	int currentPos = 0;
	int connectingPos = -1;
	int selectedLineNumber = -1;
	ConnectionModes connectDelete = NO_ACTION;
	bool isSelectingAction = false;
	const uint8_t* projectName;

	SettingsItem* ioArray[CHANNELS_NUM] =
	{
			&inputItem0,
			&inputItem1,
			&inputItem2,
			&outputItem0,
			&outputItem1,
			&outputItem2
	};

	touchgfx::Line* lines[WIRES_NUM] =
	{
			&line1_1,
			&line1_2,
			&line1_3,
			&line2_1,
			&line2_2,
			&line2_3,
			&line3_1,
			&line3_2,
			&line3_3
	};

	touchgfx::PainterRGB565* linePainters[WIRES_NUM] = {
			&line1_1Painter,
			&line1_2Painter,
			&line1_3Painter,
			&line2_1Painter,
			&line2_2Painter,
			&line2_3Painter,
			&line3_1Painter,
			&line3_2Painter,
			&line3_3Painter
	};
};

#endif // PROJECTSETTINGSVIEW_HPP
