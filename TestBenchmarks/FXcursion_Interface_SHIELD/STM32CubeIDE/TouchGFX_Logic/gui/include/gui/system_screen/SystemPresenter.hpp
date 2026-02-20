#ifndef SYSTEMPRESENTER_HPP
#define SYSTEMPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class SystemView;

class SystemPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    SystemPresenter(SystemView& v);

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

    virtual ~SystemPresenter() {}

    void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnUpUpdate(BOOLEAN bIsFuncPressed);
    void btnDownUpdate(BOOLEAN bIsFuncPressed);
    void btnFootUpdate(U8 nID);

    void encMenuUpdate(S8 nValue);
    void encParamUpdate(U8 nID, S8 nValue);

	void saveInputIsStereo1(BOOLEAN inputIsStereo)
	{
		model->saveInputIsStereo1(inputIsStereo);
	}
	BOOLEAN getInputIsStereo1()
	{
		return model->getInputIsStereo1();
	}

	void saveInputIsStereo2(BOOLEAN inputIsStereo)
	{
		model->saveInputIsStereo2(inputIsStereo);
	}
	BOOLEAN getInputIsStereo2()
	{
		return model->getInputIsStereo2();
	}

	void saveMixerPosition(S8 mixerPosition)
	{
		model->saveMixerPosition(mixerPosition);
	}
	S8 getMixerPosition()
	{
		return model->getMixerPosition();
	}

	void saveSelectedModule(ModuleSelectType selectedModule)
	{
		model->saveSelectedModule(selectedModule);
	}
	ModuleSelectType getSelectedModule()
	{
		return model->getSelectedModule();
	}

	void savePrevSelectedModule(ModuleSelectType prevSelectedModule)
	{
		model->savePrevSelectedModule(prevSelectedModule);
	}
	ModuleSelectType getPrevSelectedModule()
	{
		return model->getPrevSelectedModule();
	}

	void saveSelectedChainModule(ChainModuleNumber selectedChainModule)
	{
		model->saveSelectedChainModule(selectedChainModule);
	}
	ChainModuleNumber getSelectedChainModule()
	{
		return model->getSelectedChainModule();
	}

	void saveSelectedFootSwitch(FootSwitches selectedFootSwitch)
	{
		model->saveSelectedFootSwitch(selectedFootSwitch);
	}
	FootSwitches getSelectedFootSwitch()
	{
		return model->getSelectedFootSwitch();
	}

	void saveMonoModulePosition(ModuleName moduleName, U8 monoChainNumber, U8 chainModuleNumber)
	{
		model->saveMonoModulePosition(moduleName, monoChainNumber, chainModuleNumber);
	}
	ModuleName getMonoModuleInPosition(U8 monoChainNumber, U8 chainModuleNumber)
	{
		return model->getMonoModuleInPosition(monoChainNumber, chainModuleNumber);
	}

	void saveStereoModulePosition(ModuleName moduleName, U8 stereoChainNumber, U8 chainModuleNumber)
	{
		model->saveStereoModulePosition(moduleName, stereoChainNumber, chainModuleNumber);
	}
	ModuleName getStereoModuleInPosition(U8 stereoChainNumber, U8 chainModuleNumber)
	{
		return model->getStereoModuleInPosition(stereoChainNumber, chainModuleNumber);
	}

	void saveSelectedChannel(ChannelType channelType)
	{
		model->saveSelectedChannel(channelType);
	}
	ChannelType getSelectedChannel()
	{
		return model->getSelectedChannel();
	}

	void clearFXChain(ChannelType channel)
	{
		model->clearFXChain(channel);
	}


private:
    SystemPresenter();

    SystemView& view;
};

#endif // SYSTEMPRESENTER_HPP
