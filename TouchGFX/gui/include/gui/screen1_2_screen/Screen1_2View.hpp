#ifndef SCREEN1_2VIEW_HPP
#define SCREEN1_2VIEW_HPP

#include <gui_generated/screen1_2_screen/Screen1_2ViewBase.hpp>
#include <gui/screen1_2_screen/Screen1_2Presenter.hpp>

class Screen1_2View : public Screen1_2ViewBase
{
public:
    Screen1_2View();
    virtual ~Screen1_2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    void scrollWindow(int8_t scrollAmount);
    void changeScreenLeft();
    void changeScreenRight();
protected:
};

#endif // SCREEN1_2VIEW_HPP
