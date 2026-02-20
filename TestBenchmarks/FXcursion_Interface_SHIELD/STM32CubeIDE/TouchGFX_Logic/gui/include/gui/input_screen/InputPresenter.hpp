#ifndef INPUTPRESENTER_HPP
#define INPUTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class InputView;

class InputPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    InputPresenter(InputView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~InputPresenter() {}

    void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnUpUpdate(BOOLEAN bIsFuncPressed);
    void btnDownUpdate(BOOLEAN bIsFuncPressed);
    void btnFootUpdate(U8 nID);

    void encMenuUpdate(S8 nValue);
    void encParamUpdate(U8 nID, S8 nValue);

private:
    InputPresenter();

    InputView& view;
};

#endif // INPUTPRESENTER_HPP
