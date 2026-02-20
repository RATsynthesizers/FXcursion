#ifndef FXCHAINPRESENTER_HPP
#define FXCHAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class FXChainView;

class FXChainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    FXChainPresenter(FXChainView& v);

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

    virtual ~FXChainPresenter() {}

    void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnUpUpdate(BOOLEAN bIsFuncPressed);
    void btnDownUpdate(BOOLEAN bIsFuncPressed);
    void btnFootUpdate(U8 nID);

    void encMenuUpdate(S8 nValue);
    void encParamUpdate(U8 nID, S8 nValue);

	void saveFXChain(ChannelType channel, FXChainItemInfo* menuItemInfoArray)
	{
		model->saveFXChain(channel, menuItemInfoArray);
	}
	FXChainItemInfo getFXChainItem(ChannelType channel, U8 effectNum)
	{
		return model->getFXChainItem(channel, effectNum);
	}

	void saveEffectInfo(ChannelType channel, EffectInfo *effectInfoArray)
	{
		model->saveEffectInfo(channel, effectInfoArray);
	}
	EffectInfo getEffectInfo(ChannelType channel, U8 effectPosition)
	{
		return model->getEffectInfo(channel, effectPosition);
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

	void saveSelectedChannel(ChannelType channelType)
	{
		model->saveSelectedChannel(channelType);
	}
	ChannelType getSelectedChannel()
	{
		return model->getSelectedChannel();
	}

	void moveFXParams(ChannelType channel, U8 effectNum, S8 direction)
	{
		model->moveFXParams(channel, effectNum, direction);
	}

private:
    FXChainPresenter();

    FXChainView& view;
};

#endif // FXCHAINPRESENTER_HPP
