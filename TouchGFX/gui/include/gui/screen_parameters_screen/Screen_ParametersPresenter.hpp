#ifndef SCREEN_PARAMETERSPRESENTER_HPP
#define SCREEN_PARAMETERSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen_ParametersView;

class Screen_ParametersPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen_ParametersPresenter(Screen_ParametersView& v);

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

    virtual ~Screen_ParametersPresenter() {};

    void scrollWindow(int8_t scrollAmount);
	void changeScreenRight();
	void changeScreenLeft();
	void parameterChange();

private:
    Screen_ParametersPresenter();

    Screen_ParametersView& view;
};

#endif // SCREEN_PARAMETERSPRESENTER_HPP
