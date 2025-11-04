#ifndef MIXMODULE_HPP
#define MIXMODULE_HPP

#include <gui_generated/containers/MixModuleBase.hpp>

class MixModule : public MixModuleBase
{
public:
    MixModule();
    virtual ~MixModule() {}

    virtual void initialize();
protected:
};

#endif // MIXMODULE_HPP
