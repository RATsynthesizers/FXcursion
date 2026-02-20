#ifndef EFFECTSETTINGSPRESENTER_HPP
#define EFFECTSETTINGSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class EffectSettingsView;

class EffectSettingsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    EffectSettingsPresenter(EffectSettingsView& v);

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

    virtual ~EffectSettingsPresenter() {}

    void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnUpUpdate(BOOLEAN bIsFuncPressed);
    void btnDownUpdate(BOOLEAN bIsFuncPressed);
    void btnFootUpdate(U8 nID);

    void encMenuUpdate(S8 nValue);
    void encParamUpdate(U8 nID, S8 nValue);

	ChannelType getSelectedChannel()
	{
		return model->getSelectedChannel();
	}

	FXChainItemInfo getFXChainItem(ChannelType channel, U8 effectNum)
	{
		return model->getFXChainItem(channel, effectNum);
	}

	//scroll position of chain on each channel
	void saveChannelChainPosition(ChannelType channel, U8 saveChannelPos)
	{
		model->saveChannelChainPosition(channel, saveChannelPos);
	}
	U8 getChannelChainPosition(ChannelType channel)
	{
		return model->getChannelChainPosition(channel);
	}

	void saveFXParam(ChannelType channel, U8 effectNum, U8 paramNum, U8 paramValue)
	{
		model->saveFXParam(channel, effectNum, paramNum, paramValue);
	}
	U8 getFXParam(ChannelType channel, U8 effectNum, U8 paramNum)
	{
		return model->getFXParam(channel, effectNum, paramNum);
	}
	void moveFXParams(ChannelType channel, U8 effectNum, S8 direction)
	{
		model->moveFXParams(channel, effectNum, direction);
	}

private:
    EffectSettingsPresenter();

    EffectSettingsView& view;
};

#endif // EFFECTSETTINGSPRESENTER_HPP
