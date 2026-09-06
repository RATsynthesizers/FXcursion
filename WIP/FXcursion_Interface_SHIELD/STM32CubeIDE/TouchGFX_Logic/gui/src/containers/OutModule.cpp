#include <gui/containers/OutModule.hpp>

OutModule::OutModule()
{

}

void OutModule::initialize()
{
    OutModuleBase::initialize();
}

void OutModule::select()
{
	selectIOModule.setVisible(true);
	invalidate();
}

void OutModule::deselect()
{
	selectIOModule.setVisible(false);
	invalidate();
}
