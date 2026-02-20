#ifndef RECORDERVIEW_HPP
#define RECORDERVIEW_HPP

#include <gui_generated/recorder_screen/RecorderViewBase.hpp>
#include <gui/recorder_screen/RecorderPresenter.hpp>

class RecorderView : public RecorderViewBase
{
public:
    RecorderView();
    virtual ~RecorderView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnYesUpdate(S8 nValue);
    void btnNoUpdate(S8 nValue);
    void btnUpUpdate();
    void btnDownUpdate();
protected:
};

#endif // RECORDERVIEW_HPP
