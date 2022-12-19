#ifndef SCREEN1_1VIEW_HPP
#define SCREEN1_1VIEW_HPP

#include <gui_generated/screen1_1_screen/Screen1_1ViewBase.hpp>
#include <gui/screen1_1_screen/Screen1_1Presenter.hpp>

class Screen1_1View : public Screen1_1ViewBase
{
public:
    Screen1_1View();
    virtual ~Screen1_1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    void scrollWindow(int8_t scrollAmount);
    void changeScreenLeft();
protected:
};

#endif // SCREEN1_1VIEW_HPP
