#include <gui/containers/StatusBar.hpp>

StatusBar::StatusBar()
{

}

void StatusBar::initialize()
{
	const U8 projectName[NAMETEXT_SIZE] = {0};
	sprintf((char*) projectName, "%s", application().getModel()->getProjectName());
	Unicode::fromUTF8(projectName, nameTextBuffer, NAMETEXT_SIZE);

	const U8 bpm[BPMTEXT_SIZE] = {0};
	sprintf((char*) bpm, "%d", application().getModel()->getBPM());
	Unicode::fromUTF8(bpm, bpmTextBuffer, BPMTEXT_SIZE);

	const U8 batteryState[BATTEXT_SIZE] = {0};
	sprintf((char*) batteryState, "%d", application().getModel()->getBatteryState());
	Unicode::fromUTF8(batteryState, batTextBuffer, BATTEXT_SIZE);

    StatusBarBase::initialize();
}

void StatusBar::startMove()
{
	const U8 moveText[NAMETEXT_SIZE] = {0};
	sprintf((char*) moveText, "%s", "Move");
	Unicode::fromUTF8(moveText, nameTextBuffer, NAMETEXT_SIZE);

	moveBox.setVisible(TRUE);

	invalidate();
}

void StatusBar::stopMove()
{
	const U8 projectName[NAMETEXT_SIZE] = {0};
	sprintf((char*) projectName, "%s", application().getModel()->getProjectName());
	Unicode::fromUTF8(projectName, nameTextBuffer, NAMETEXT_SIZE);

	moveBox.setVisible(FALSE);

	invalidate();
}

void StatusBar::updateProjectName()
{
	const U8 projectName[NAMETEXT_SIZE] = {0};
	sprintf((char*) projectName, "%s", application().getModel()->getProjectName());
	Unicode::fromUTF8(projectName, nameTextBuffer, NAMETEXT_SIZE);

	invalidate();
}

void StatusBar::updateBattery()
{
	const U8 batteryState[BATTEXT_SIZE] = {0};
	sprintf((char*) batteryState, "%d", application().getModel()->getBatteryState());
	Unicode::fromUTF8(batteryState, batTextBuffer, BATTEXT_SIZE);

	invalidate();
}

void StatusBar::updateBPM()
{

	const U8 bpm[BPMTEXT_SIZE] = {0};
	sprintf((char*) bpm, "%d", application().getModel()->getBPM());
	Unicode::fromUTF8(bpm, bpmTextBuffer, BPMTEXT_SIZE);

	invalidate();
}
