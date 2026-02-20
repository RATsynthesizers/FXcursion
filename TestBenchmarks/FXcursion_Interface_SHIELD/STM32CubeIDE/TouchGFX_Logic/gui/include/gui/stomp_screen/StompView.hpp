#ifndef STOMPVIEW_HPP
#define STOMPVIEW_HPP

#include <gui_generated/stomp_screen/StompViewBase.hpp>
#include <gui/stomp_screen/StompPresenter.hpp>

class StompView : public StompViewBase
{
public:
    StompView();
    virtual ~StompView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnYesUpdate(S8 nValue);
    void btnNoUpdate(S8 nValue);
    void btnUpUpdate();
    void btnDownUpdate();
protected:
};

#endif // STOMPVIEW_HPP
