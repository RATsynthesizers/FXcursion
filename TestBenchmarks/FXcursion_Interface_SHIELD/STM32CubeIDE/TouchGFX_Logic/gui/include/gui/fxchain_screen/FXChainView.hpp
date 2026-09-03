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

    /*
     * menuItemsBeforeMoving used to sit here: a snapshot of the whole chain,
     * taken on entering move mode and memcpy'd back on cancel.
     *
     * It is gone because the move is now applied to the model on every step
     * rather than at commit, so cancel is just "walk it back" using the same
     * swap the forward move uses. That removes the class of bug where the
     * restore path and the forward path can disagree, and it is what makes
     * editing a parameter mid-move safe.
     */
    U8 nPosBeforeMoving = 0;

    /** Swap two neighbours: display, stored order and parameter blocks. */
    void swapEffects(U8 nFrom, U8 nTo);

    /** Turn one effect off or back on. Repaints, stores and re-sends. */
    void setBypass(U8 nPos, BOOLEAN bBypass);
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
