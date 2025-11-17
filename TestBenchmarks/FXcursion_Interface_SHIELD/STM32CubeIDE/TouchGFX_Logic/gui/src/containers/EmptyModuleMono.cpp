#include <gui/containers/EmptyModuleMono.hpp>

EmptyModuleMono::EmptyModuleMono()
{

}

void EmptyModuleMono::initialize()
{
    EmptyModuleMonoBase::initialize();
}

void EmptyModuleMono::select()
{
	selectModuleMono.setVisible(true);
	invalidate();
}

void EmptyModuleMono::deselect()
{
	selectModuleMono.setVisible(false);
	invalidate();
}
