#ifndef EMPTYMODULEMONO_HPP
#define EMPTYMODULEMONO_HPP

#include <gui_generated/containers/EmptyModuleMonoBase.hpp>

class EmptyModuleMono : public EmptyModuleMonoBase
{
public:
    EmptyModuleMono();
    virtual ~EmptyModuleMono() {}

    virtual void initialize();
protected:
};

#endif // EMPTYMODULEMONO_HPP
