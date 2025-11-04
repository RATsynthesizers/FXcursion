#ifndef MODULELEVEL_HPP
#define MODULELEVEL_HPP

#include <gui_generated/containers/ModuleLevelBase.hpp>

class ModuleLevel : public ModuleLevelBase
{
public:
    ModuleLevel();
    virtual ~ModuleLevel() {}

    virtual void initialize();
protected:
};

#endif // MODULELEVEL_HPP
