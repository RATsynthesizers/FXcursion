#ifndef SCREEN1_2PRESENTER_HPP
#define SCREEN1_2PRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class Screen1_2View;

class Screen1_2Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    Screen1_2Presenter(Screen1_2View& v);

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

    virtual ~Screen1_2Presenter() {};

    void scrollWindow(int8_t scrollAmount);

	void changeScreenRight();
	void changeScreenLeft();

	void parameterChange();

private:
    Screen1_2Presenter();

    Screen1_2View& view;
};

#endif // SCREEN1_2PRESENTER_HPP
