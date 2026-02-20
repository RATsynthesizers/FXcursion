#include <gui/containers/StereoChain.hpp>

const U8 StereoChain::MODULE_X_POSITIONS[4] =
{
	182,
	124,
	66,
	8
};

StereoChain::StereoChain()
{
	eSelectedModuleNumber = CHAIN_MODULE_1;
}

void StereoChain::initialize()
{
    StereoChainBase::initialize();
}

void StereoChain::select(ChainModuleNumber eModuleNum)
{
	eSelectedModuleName = aCurrentState[eModuleNum];
	eSelectedModuleNumber = eModuleNum;

	switch(eModuleNum)
	{
	case CHAIN_MODULE_1:
		emptyModuleStereo1.select();
		break;

	case CHAIN_MODULE_2:
		emptyModuleStereo2.select();
		break;

	case CHAIN_MODULE_3:
		emptyModuleStereo3.select();
		break;

	case CHAIN_MODULE_4:
		emptyModuleStereo4.select();
		break;

	default:
		break;
	}
}

void StereoChain::deselect(ChainModuleNumber eModuleNum)
{
	switch(eModuleNum)
	{
	case CHAIN_MODULE_1:
		emptyModuleStereo1.deselect();
		break;

	case CHAIN_MODULE_2:
		emptyModuleStereo2.deselect();
		break;

	case CHAIN_MODULE_3:
		emptyModuleStereo3.deselect();
		break;

	case CHAIN_MODULE_4:
		emptyModuleStereo4.deselect();
		break;

	default:
		break;
	}
}

void StereoChain::addModule(ChainModuleNumber eModuleNum, ModuleName eModuleName)
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


void StereoChain::deleteSelectedModule()
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


ChainModuleNumber StereoChain::getSelectedModuleNumber()
{
	return eSelectedModuleNumber;
}


ModuleName StereoChain::getSelectedModuleName()
{
	return eSelectedModuleName;
}


ModuleName StereoChain::getModuleName(ChainModuleNumber eModuleNumber)
{
	return aCurrentState[eModuleNumber];
}
