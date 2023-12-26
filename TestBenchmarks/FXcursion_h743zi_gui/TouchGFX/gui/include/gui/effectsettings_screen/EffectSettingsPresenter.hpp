#ifndef EFFECTSETTINGSPRESENTER_HPP
#define EFFECTSETTINGSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class EffectSettingsView;

class EffectSettingsPresenter: public touchgfx::Presenter, public ModelListener {
public:
	EffectSettingsPresenter(EffectSettingsView &v);

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

	virtual ~EffectSettingsPresenter() {
	}
	;

	void parameterChange(uint8_t id, int8_t scrollAmount);
	void encScroll_action(int8_t scrollAmount);
	void btnYES_action(void);
	void btnNO_action(void);

	uint8_t getChannelNum() {
		return model->getChannelNum();
	}

	void saveChannelChainPosition(uint8_t channelNum, uint8_t saveChannelPos) {
		model->saveChannelChainPosition(channelNum, saveChannelPos);
	}

	uint8_t getChannelChainPosition(uint8_t channelNum) {
		return model->getChannelChainPosition(channelNum);
	}

	MenuItemInfo getFXchainItem(uint8_t channelNum, uint8_t chainItem) {
		return model->getFXchainItem(channelNum, chainItem);
	}

private:
	EffectSettingsPresenter();

	EffectSettingsView &view;
};

#endif // EFFECTSETTINGSPRESENTER_HPP
