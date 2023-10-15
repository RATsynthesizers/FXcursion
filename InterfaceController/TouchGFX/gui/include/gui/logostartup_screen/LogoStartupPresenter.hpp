#ifndef LOGOSTARTUPPRESENTER_HPP
#define LOGOSTARTUPPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class LogoStartupView;

class LogoStartupPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    LogoStartupPresenter(LogoStartupView& v);

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

    virtual ~LogoStartupPresenter() {};

    void parameterChange(uint8_t id, int8_t scrollAmount);
    void encScroll_action(int8_t scrollAmount);
    void btnYES_action(void);
    void btnNO_action(void);

private:
    LogoStartupPresenter();

    LogoStartupView& view;
};

#endif // LOGOSTARTUPPRESENTER_HPP
