#ifndef FXCHAINVIEW_HPP
#define FXCHAINVIEW_HPP

#include <gui_generated/fxchain_screen/FXChainViewBase.hpp>
#include <gui/fxchain_screen/FXChainPresenter.hpp>

class FXChainView : public FXChainViewBase
{
public:
    FXChainView();
    virtual ~FXChainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnUpUpdate(BOOLEAN bIsFuncPressed);
    void btnDownUpdate(BOOLEAN bIsFuncPressed);

protected:
    BOOLEAN bIsMoving = FALSE;
    BOOLEAN bIsSelectingEffect = FALSE;
    U8 nCurrentPos = 0;
    U8 nCurrentEffectNumber = 0;
    FXChainItemInfo menuItemInfo[MAX_EFFECTS_NUM];
    FXChainItemInfo menuItemsBeforeMoving[MAX_EFFECTS_NUM];
    U8 nPosBeforeMoving = 0;
    EffectInfo effectPool[EFFECT_TYPES];
    ChannelType eChannelType = CHANNEL_MONO_1;

    FXChainItem* scrollMenu[MAX_EFFECTS_NUM] =
    {
    	&Item0,
		&Item1,
		&Item2,
		&Item3
    };
};

#endif // FXCHAINVIEW_HPP
