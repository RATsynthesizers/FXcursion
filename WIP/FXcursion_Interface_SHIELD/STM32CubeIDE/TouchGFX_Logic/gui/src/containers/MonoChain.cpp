#include <gui/containers/MonoChain.hpp>

const U8 MonoChain::MODULE_X_POSITIONS[4] =
{
	182,
	124,
	66,
	8
};

MonoChain::MonoChain()
{
	eSelectedModuleNumber = CHAIN_MODULE_1;

	/*
	 * eSelectedModuleName was left uninitialised here and only ever assigned
	 * by select() or addModule(). No current path reads it first - SystemView
	 * calls select() on the active chain in setupScreen, and consults the
	 * inactive ones through getModuleName(), which reads aCurrentState - but
	 * getSelectedModuleName() is public and one refactor away from being
	 * called on a chain that has never been selected.
	 */
	eSelectedModuleName = MODULE_NONE;
}

void MonoChain::initialize()
{
    MonoChainBase::initialize();
}

void MonoChain::select(ChainModuleNumber eModuleNum)
{
	eSelectedModuleName = aCurrentState[eModuleNum];
	eSelectedModuleNumber = eModuleNum;

	switch(eModuleNum)
	{
	case CHAIN_MODULE_1:
		emptyModuleMono1.select();
		break;

	case CHAIN_MODULE_2:
		emptyModuleMono2.select();
		break;

	case CHAIN_MODULE_3:
		emptyModuleMono3.select();
		break;

	case CHAIN_MODULE_4:
		emptyModuleMono4.select();
		break;

	default:
		break;
	}
}

void MonoChain::deselect(ChainModuleNumber eModuleNum)
{
	switch(eModuleNum)
	{
	case CHAIN_MODULE_1:
		emptyModuleMono1.deselect();
		break;

	case CHAIN_MODULE_2:
		emptyModuleMono2.deselect();
		break;

	case CHAIN_MODULE_3:
		emptyModuleMono3.deselect();
		break;

	case CHAIN_MODULE_4:
		emptyModuleMono4.deselect();
		break;

	default:
		break;
	}
}

void MonoChain::addModule(ChainModuleNumber eModuleNum, ModuleName eModuleName)
{
	aCurrentState[eModuleNum] = eModuleName;
	eSelectedModuleName = eModuleName;

	switch(eModuleName)
	{
	case MODULE_FX:
		fXModule.setXY(MODULE_X_POSITIONS[eModuleNum], MODULE_Y_POS);
		fXModule.setVisible(true);
		break;
	case MODULE_REC:
		recModule.setXY(MODULE_X_POSITIONS[eModuleNum], MODULE_Y_POS);
		recModule.setVisible(true);
		break;
	case MODULE_LOOP:
		loopModule.setXY(MODULE_X_POSITIONS[eModuleNum], MODULE_Y_POS);
		loopModule.setVisible(true);
		break;
	default:
		break;
	}
}

void MonoChain::deleteSelectedModule()
{
	switch(eSelectedModuleName)
	{
	case MODULE_FX:
		fXModule.setVisible(false);
		fXModule.invalidate();
		break;
	case MODULE_REC:
		recModule.setVisible(false);
		recModule.invalidate();
		break;
	case MODULE_LOOP:
		loopModule.setVisible(false);
		loopModule.invalidate();
		break;
	default:
		break;
	}

	aCurrentState[eSelectedModuleNumber] = MODULE_NONE;
	eSelectedModuleName = MODULE_NONE;
}


ChainModuleNumber MonoChain::getSelectedModuleNumber()
{
	return eSelectedModuleNumber;
}


ModuleName MonoChain::getSelectedModuleName()
{
	return eSelectedModuleName;
}


ModuleName MonoChain::getModuleName(ChainModuleNumber eModuleNumber)
{
	return aCurrentState[eModuleNumber];
}
