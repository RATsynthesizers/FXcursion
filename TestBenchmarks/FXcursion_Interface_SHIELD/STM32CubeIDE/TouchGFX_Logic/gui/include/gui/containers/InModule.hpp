#ifndef INMODULE_HPP
#define INMODULE_HPP

#include <gui_generated/containers/InModuleBase.hpp>

class InModule : public InModuleBase
{
public:
    InModule();
    virtual ~InModule() {}

    virtual void initialize();
protected:
};

#endif // INMODULE_HPP
