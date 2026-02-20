#ifndef MIXERVIEW_HPP
#define MIXERVIEW_HPP

#include <gui_generated/mixer_screen/MixerViewBase.hpp>
#include <gui/mixer_screen/MixerPresenter.hpp>

class MixerView : public MixerViewBase
{
public:
    MixerView();
    virtual ~MixerView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnYesUpdate(S8 nValue);
    void btnNoUpdate(S8 nValue);
    void btnUpUpdate();
    void btnDownUpdate();
protected:
};

#endif // MIXERVIEW_HPP
