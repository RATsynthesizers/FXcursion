#ifndef SYSTEMPRESENTER_HPP
#define SYSTEMPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class SystemView;

class SystemPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    SystemPresenter(SystemView& v);

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

    virtual ~SystemPresenter() {}

    void encParam_action(uint8_t id, int8_t scrollAmount);
    void encSelect_action(int8_t scrollAmount);
    void btnYES_action(void);
    void btnNO_action(void);
    void btnUP_action(void);
    void btnDOWN_action(void);

private:
    SystemPresenter();

    SystemView& view;
};

#endif // SYSTEMPRESENTER_HPP
