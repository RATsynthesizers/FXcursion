#ifndef INPUTVIEW_HPP
#define INPUTVIEW_HPP

#include <gui_generated/input_screen/InputViewBase.hpp>
#include <gui/input_screen/InputPresenter.hpp>

class InputView : public InputViewBase
{
public:
    InputView();
    virtual ~InputView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnYesUpdate(S8 nValue);
    void btnNoUpdate(S8 nValue);
    void btnUpUpdate();
    void btnDownUpdate();
protected:
};

#endif // INPUTVIEW_HPP
