#ifndef LOGOSTARTUPVIEW_HPP
#define LOGOSTARTUPVIEW_HPP

#include <gui_generated/logostartup_screen/LogoStartupViewBase.hpp>
#include <gui/logostartup_screen/LogoStartupPresenter.hpp>

class LogoStartupView : public LogoStartupViewBase
{
public:
    LogoStartupView();
    virtual ~LogoStartupView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // LOGOSTARTUPVIEW_HPP
