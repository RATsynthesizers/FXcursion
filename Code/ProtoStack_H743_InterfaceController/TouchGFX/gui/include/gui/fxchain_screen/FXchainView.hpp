#ifndef FXCHAINVIEW_HPP
#define FXCHAINVIEW_HPP

#include <gui_generated/fxchain_screen/FXchainViewBase.hpp>
#include <gui/fxchain_screen/FXchainPresenter.hpp>

class FXchainView : public FXchainViewBase
{
public:
    FXchainView();
    virtual ~FXchainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void parameterChange(uint8_t id, int8_t scrollAmount);
    void encScroll_action(int8_t scrollAmount);
    void btnYES_action(void);
    void btnNO_action(void);
protected:
    bool isInSubMenu = false;
    bool isSelectingEffect = false;
    uint8_t currentPos = 0;
    uint8_t currentEffectsNumber = 0;
    MenuItemInfo menuItemInfoArray[MAX_EFFECTS_NUM];
    EffectInfo effectInfoArray[EFFECT_TYPES];
    uint8_t channelNum = 0;

    MenuItem* scrollMenu[MAX_EFFECTS_NUM] =
    {
    	&menuItem0,
		&menuItem1,
		&menuItem2,
		&menuItem3
    };
};

#endif // FXCHAINVIEW_HPP
