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

    void btnYesUpdate(void);
    void btnNoUpdate(void);
    void btnUpUpdate(void);
    void btnDownUpdate(void);
    void btnFootUpdate(U8 nID);

    void encMenuUpdate(S8 nValue);
    void encParamUpdate(U8 nID, S8 nValue);

private:
    SystemPresenter();

    SystemView& view;
};

#endif // SYSTEMPRESENTER_HPP
