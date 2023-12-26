#include <gui/logostartup_screen/LogoStartupView.hpp>

LogoStartupView::LogoStartupView()
{

}

void LogoStartupView::setupScreen()
{
    LogoStartupViewBase::setupScreen();
}

void LogoStartupView::tearDownScreen()
{
    LogoStartupViewBase::tearDownScreen();
    osDelay(100);
    audioAdapter_Init(&audioAdapter);  // uart interface to audio mcu
}
