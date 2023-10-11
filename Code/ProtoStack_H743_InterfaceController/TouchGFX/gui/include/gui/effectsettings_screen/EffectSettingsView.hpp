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

    void parameterChange(uint8_t id, int8_t scrollAmount);
    void encScroll_action(int8_t scrollAmount);
    void btnYES_action(void);
    void btnNO_action(void);
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

    MenuItemInfo menuItemInfoArray[MAX_EFFECTS_NUM];

    uint8_t channelNum;
    uint8_t editingEffectNum;
    uint8_t selectedEffectNum;
};

#endif // EFFECTSETTINGSVIEW_HPP
