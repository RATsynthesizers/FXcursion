#include <gui/containers/InModule.hpp>

InModule::InModule()
{

}

void InModule::initialize()
{
    InModuleBase::initialize();
}

void InModule::select()
{
	selectIOModule.setVisible(true);
	invalidate();
}

void InModule::deselect()
{
	selectIOModule.setVisible(false);
	invalidate();
}
