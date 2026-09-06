#include <gui/containers/MixModule.hpp>

MixModule::MixModule()
{

}

void MixModule::initialize()
{
    MixModuleBase::initialize();
}

void MixModule::select()
{
	selectMixer.setVisible(true);
	invalidate();
}

void MixModule::deselect()
{
	selectMixer.setVisible(false);
	invalidate();
}
