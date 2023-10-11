#ifndef SCREEN_PARAMETERSVIEW_HPP
#define SCREEN_PARAMETERSVIEW_HPP

#include <gui_generated/screen_parameters_screen/Screen_ParametersViewBase.hpp>
#include <gui/screen_parameters_screen/Screen_ParametersPresenter.hpp>

class Screen_ParametersView : public Screen_ParametersViewBase
{
public:
    Screen_ParametersView();
    virtual ~Screen_ParametersView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    void changeScreenLeft();
    void handleTickEvent();
    void parameterChange();
protected:
    int modifier;
};

#endif // SCREEN_PARAMETERSVIEW_HPP
