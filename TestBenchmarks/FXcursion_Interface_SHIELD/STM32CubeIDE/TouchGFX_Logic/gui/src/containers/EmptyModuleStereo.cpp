#include <gui/containers/EmptyModuleStereo.hpp>

EmptyModuleStereo::EmptyModuleStereo()
{

}

void EmptyModuleStereo::initialize()
{
    EmptyModuleStereoBase::initialize();
}

void EmptyModuleStereo::select()
{
	selectModuleStereo.setVisible(true);
	invalidate();
}

void EmptyModuleStereo::deselect()
{
	selectModuleStereo.setVisible(false);
	invalidate();
}
