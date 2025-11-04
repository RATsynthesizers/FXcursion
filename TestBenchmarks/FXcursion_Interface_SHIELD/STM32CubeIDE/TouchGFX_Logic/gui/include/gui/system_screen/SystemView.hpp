#ifndef SYSTEMVIEW_HPP
#define SYSTEMVIEW_HPP

#include <gui_generated/system_screen/SystemViewBase.hpp>
#include <gui/system_screen/SystemPresenter.hpp>

class SystemView : public SystemViewBase
{
public:
    SystemView();
    virtual ~SystemView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // SYSTEMVIEW_HPP
