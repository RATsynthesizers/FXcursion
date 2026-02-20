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
    CustomGauge* customGauges[4] =
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
    U8 editingEffectNum = 0;
    U8 selectedEffectNum = 0;
};

#endif // EFFECTSETTINGSVIEW_HPP
