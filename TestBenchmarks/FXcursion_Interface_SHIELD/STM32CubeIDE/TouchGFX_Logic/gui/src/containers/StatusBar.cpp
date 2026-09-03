#include <gui/containers/StatusBar.hpp>
#include <stdio.h>

/*
 * A note on the scratch buffers below.
 *
 * They used to be declared `const U8 x[N] = {0}` and then written through
 * `sprintf((char*) x, ...)`, casting the const away. That is undefined
 * behaviour: a const-qualified aggregate with a constant initialiser is
 * exactly the sort of object a compiler may place in .rodata, and a write to
 * it then either faults or is quietly discarded. It worked only because at
 * -Og this GCC happens to keep them on the stack - verified in the
 * disassembly, `add r0, sp, #12` - and that is precisely the sort of thing
 * that changes with the optimisation level or the toolchain. The const is
 * gone, and sprintf is snprintf so the Designer-generated buffer size is
 * respected rather than assumed.
 *
 * The sizes are tight but correct: BPMTEXT_SIZE and BATTEXT_SIZE are 4, and
 * both values are U8, so "255" plus a terminator is the worst case.
 */

StatusBar::StatusBar()
{

}

void StatusBar::initialize()
{
	/* Prime the buffers only - the container has not been drawn yet, so
	   there is nothing to invalidate. */
	formatProjectName();
	formatBPM();
	formatBattery();

    StatusBarBase::initialize();
}

void StatusBar::formatProjectName()
{
	U8 projectName[NAMETEXT_SIZE] = {0};
	snprintf((char*) projectName, sizeof(projectName), "%s",
			application().getModel()->getProjectName());
	Unicode::fromUTF8(projectName, nameTextBuffer, NAMETEXT_SIZE);
}

void StatusBar::formatBattery()
{
	U8 batteryState[BATTEXT_SIZE] = {0};
	snprintf((char*) batteryState, sizeof(batteryState), "%d",
			application().getModel()->getBatteryState());
	Unicode::fromUTF8(batteryState, batTextBuffer, BATTEXT_SIZE);
}

void StatusBar::formatBPM()
{
	U8 bpm[BPMTEXT_SIZE] = {0};
	snprintf((char*) bpm, sizeof(bpm), "%d",
			application().getModel()->getBPM());
	Unicode::fromUTF8(bpm, bpmTextBuffer, BPMTEXT_SIZE);
}

void StatusBar::startMove()
{
	U8 moveText[NAMETEXT_SIZE] = {0};
	snprintf((char*) moveText, sizeof(moveText), "%s", "Move");
	Unicode::fromUTF8(moveText, nameTextBuffer, NAMETEXT_SIZE);

	moveBox.setVisible(TRUE);

	invalidate();
}

void StatusBar::stopMove()
{
	formatProjectName();

	moveBox.setVisible(FALSE);

	invalidate();
}

void StatusBar::updateProjectName()
{
	formatProjectName();

	invalidate();
}

void StatusBar::updateBattery()
{
	formatBattery();

	invalidate();
}

void StatusBar::updateBPM()
{
	formatBPM();

	invalidate();
}
