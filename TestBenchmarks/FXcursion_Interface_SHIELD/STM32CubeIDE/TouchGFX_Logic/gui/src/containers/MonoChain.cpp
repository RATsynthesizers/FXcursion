#include <gui/containers/MonoChain.hpp>

MonoChain::MonoChain()
{

}

void MonoChain::initialize()
{
    MonoChainBase::initialize();
}

void MonoChain::select(MonoChainModules eModuleNum)
{
	switch(eModuleNum)
	{
	case MONO_CHAIN_MODULE_1:
		eSelectedModule = MONO_CHAIN_MODULE_1;
		emptyModuleMono1.select();
		emptyModuleMono2.deselect();
		emptyModuleMono3.deselect();
		emptyModuleMono4.deselect();
		break;

	case MONO_CHAIN_MODULE_2:
		eSelectedModule = MONO_CHAIN_MODULE_2;
		emptyModuleMono1.deselect();
		emptyModuleMono2.select();
		emptyModuleMono3.deselect();
		emptyModuleMono4.deselect();
		break;

	case MONO_CHAIN_MODULE_3:
		eSelectedModule = MONO_CHAIN_MODULE_3;
		emptyModuleMono1.deselect();
		emptyModuleMono2.deselect();
		emptyModuleMono3.select();
		emptyModuleMono4.deselect();
		break;

	case MONO_CHAIN_MODULE_4:
		eSelectedModule = MONO_CHAIN_MODULE_4;
		emptyModuleMono1.deselect();
		emptyModuleMono2.deselect();
		emptyModuleMono3.deselect();
		emptyModuleMono4.select();
		break;

	default:
		break;
	}
}

void MonoChain::deselect()
{
	emptyModuleMono1.deselect();
	emptyModuleMono2.deselect();
	emptyModuleMono3.deselect();
	emptyModuleMono4.deselect();
}


MonoChainModules MonoChain::getSelectedModule()
{
	return eSelectedModule;
}
