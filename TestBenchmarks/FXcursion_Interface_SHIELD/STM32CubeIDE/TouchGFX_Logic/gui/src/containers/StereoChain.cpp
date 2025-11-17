#include <gui/containers/StereoChain.hpp>

StereoChain::StereoChain()
{
	eSelectedModule = STEREO_CHAIN_MODULE_1;
}

void StereoChain::initialize()
{
    StereoChainBase::initialize();
}

void StereoChain::select(StereoChainModules eModuleNum)
{
	switch(eModuleNum)
	{
	case STEREO_CHAIN_MODULE_1:
		eSelectedModule = STEREO_CHAIN_MODULE_1;
		emptyModuleStereo1.select();
		emptyModuleStereo2.deselect();
		emptyModuleStereo3.deselect();
		emptyModuleStereo4.deselect();
		break;

	case STEREO_CHAIN_MODULE_2:
		eSelectedModule = STEREO_CHAIN_MODULE_2;
		emptyModuleStereo1.deselect();
		emptyModuleStereo2.select();
		emptyModuleStereo3.deselect();
		emptyModuleStereo4.deselect();
		break;

	case STEREO_CHAIN_MODULE_3:
		eSelectedModule = STEREO_CHAIN_MODULE_3;
		emptyModuleStereo1.deselect();
		emptyModuleStereo2.deselect();
		emptyModuleStereo3.select();
		emptyModuleStereo4.deselect();
		break;

	case STEREO_CHAIN_MODULE_4:
		eSelectedModule = STEREO_CHAIN_MODULE_4;
		emptyModuleStereo1.deselect();
		emptyModuleStereo2.deselect();
		emptyModuleStereo3.deselect();
		emptyModuleStereo4.select();
		break;

	default:
		break;
	}
}

void StereoChain::deselect()
{
	emptyModuleStereo1.deselect();
	emptyModuleStereo2.deselect();
	emptyModuleStereo3.deselect();
	emptyModuleStereo4.deselect();
}

StereoChainModules StereoChain::getSelectedModule()
{
	return eSelectedModule;
}
