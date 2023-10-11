#ifndef FXCHAINPRESENTER_HPP
#define FXCHAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class FXchainView;

class FXchainPresenter: public touchgfx::Presenter, public ModelListener {
public:
	FXchainPresenter(FXchainView &v);

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

	virtual ~FXchainPresenter() {
	}
	;

	void parameterChange(uint8_t id, int8_t scrollAmount);
	void encScroll_action(int8_t scrollAmount);
	void btnYES_action(void);
	void btnNO_action(void);

	uint8_t getChannelNum() {
		return model->getChannelNum();
	}

	MenuItemInfo getFXchainItem(uint8_t channelNum, uint8_t chainItem) {
		return model->getFXchainItem(channelNum, chainItem);
	}

	void saveFXchain(uint8_t channelNum, MenuItemInfo* menuItemInfoArray) {
		model->saveFXchain(channelNum, menuItemInfoArray);
	}

	void saveEffectInfo(uint8_t channelNum, EffectInfo* effectInfoArray) {
		model->saveEffectInfo(channelNum, effectInfoArray);
	}

	EffectInfo getEffectInfo(uint8_t channelNum, uint8_t effectPosition) {
		return model->getEffectInfo(channelNum, effectPosition);
	}

	void saveChannelChainPosition(uint8_t channelNum, uint8_t saveChannelPos) {
		model->saveChannelChainPosition(channelNum, saveChannelPos);
	}

	uint8_t getChannelChainPosition(uint8_t channelNum) {
		return model->getChannelChainPosition(channelNum);
	}


private:
	FXchainPresenter();

	FXchainView &view;
};

#endif // FXCHAINPRESENTER_HPP
