#ifndef LOOPERVIEW_HPP
#define LOOPERVIEW_HPP

#include <gui_generated/looper_screen/LooperViewBase.hpp>
#include <gui/looper_screen/LooperPresenter.hpp>

class LooperView : public LooperViewBase
{
public:
    LooperView();
    virtual ~LooperView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnYesUpdate(S8 nValue);
    void btnNoUpdate(S8 nValue);
    void btnUpUpdate();
    void btnDownUpdate();
protected:
};

#endif // LOOPERVIEW_HPP
