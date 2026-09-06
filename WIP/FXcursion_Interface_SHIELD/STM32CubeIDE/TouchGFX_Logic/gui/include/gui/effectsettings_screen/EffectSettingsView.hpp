#ifndef EFFECTSETTINGSVIEW_HPP
#define EFFECTSETTINGSVIEW_HPP

#include <gui_generated/effectsettings_screen/EffectSettingsViewBase.hpp>
#include <gui/effectsettings_screen/EffectSettingsPresenter.hpp>

class EffectSettingsView : public EffectSettingsViewBase
{
public:
    EffectSettingsView();
    virtual ~EffectSettingsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnUpUpdate(BOOLEAN bIsFuncPressed);
    void btnDownUpdate(BOOLEAN bIsFuncPressed);

    void encParamUpdate(U8 nID, S8 nValue);
    void encMenuUpdate(S8 nValue);

protected:

    /* Gauges on screen at once, and therefore parameters per page. The
       parameter store is MAX_PARAMETERS deep, so page * PARAMS_PER_PAGE +
       gauge must stay inside it: 2 pages * 4 = 8 = MAX_PARAMETERS. */
    static const U8 PARAMS_PER_PAGE = 4;

    /*
     * Reload all four gauges from the model for the current effect and page.
     *
     * Four identical copies of this loop used to sit in setupScreen,
     * btnYesUpdate, btnUpUpdate and btnDownUpdate. Three of them also called
     * customGauges[i]->invalidate() afterwards, which was redundant -
     * CustomGauge::setValue and setParamName both invalidate already.
     */
    void refreshParamPage();

    /** Everything that changes when the edited effect changes: name, bypass
        plate, pictogram row, page count and the parameter page. */
    void refreshEditedEffect();

    /***********************************************************************
     * MOVE MODE
     *
     * FUNC+YES arms it, the menu encoder reorders, YES commits, NO walks it
     * back to where it started.
     *
     * Every step writes the new order to the model IMMEDIATELY - the swap and
     * the parameter block move happen together in swapEffects. That is what
     * makes editing a parameter mid-move safe: there is never a moment where
     * the displayed order and the stored parameters disagree, so
     * encParamUpdate always writes to the right place.
     *
     * No snapshot array. Cancel replays the same swaps in reverse using the
     * same routine, so the inverse cannot drift from the forward path. A
     * parameter the user changed on the way stays changed - only the position
     * is restored.
     **********************************************************************/
    BOOLEAN bIsMoving        = FALSE;
    U8      nPosBeforeMoving = 0U;

    /** Swap two neighbours: display, stored order and parameter blocks. */
    void swapEffects(U8 nFrom, U8 nTo);

    /** Re-apply select / edit / move to the whole row. Cheap - each pictogram
        skips the repaint when its own state did not change. */
    void refreshPictogramStates();

    /** FALSE when there is nothing to move, nothing to move it past, or the
        effect is bypassed - FUNC+YES means "turn it back on" then. */
    BOOLEAN canMoveSelected();

    /***********************************************************************
     * BYPASS
     *
     * FUNC+NO turns the edited effect off, FUNC+YES turns it back on. There
     * is no delete from this screen, so a second FUNC+NO does nothing here.
     *
     * The gauges deliberately stay live while bypassed: dialling an effect in
     * silently and then enabling it is a normal way to work, and greying them
     * would suggest they are inert.
     **********************************************************************/
    void setBypass(U8 nPos, BOOLEAN bBypass);

    /** Repaint the name plate for the edited effect's bypass state. */
    void refreshBypassVisual();

    /*
     * Captured from the Base so the Designer stays the source of truth for
     * the normal colours. Bypassed draws the name plate in the screen's own
     * background colour and the title in white.
     */
    touchgfx::colortype nameBoxActiveColor;
    touchgfx::colortype nameBoxBypassedColor;
    touchgfx::colortype effectNameActiveColor;
    touchgfx::colortype effectNameBypassedColor;

    CustomGauge* customGauges[PARAMS_PER_PAGE] =
    {
    	&customGauge0,
    	&customGauge1,
		&customGauge2,
		&customGauge3
    };

    EffectPictogram* effectPictograms[4] =
    {
    	&effectPictogram0,
		&effectPictogram1,
		&effectPictogram2,
		&effectPictogram3
    };

    FXChainItemInfo fxChainItemInfoArray[MAX_EFFECTS_NUM];

    ChannelType eChannelType = CHANNEL_MONO_1;

    /*
     * The ONLY position on this screen. selectedEffectNum used to sit
     * alongside it as a separate cursor that YES committed into this one, and
     * that is what made the FUNC gestures ambiguous: bypass could only
     * sensibly act on the effect whose name was displayed, move could only
     * sensibly act on the cursor, and they were not always the same effect.
     * The menu encoder now changes this directly.
     */
    U8 editingEffectNum = 0;
};

#endif // EFFECTSETTINGSVIEW_HPP
