#ifndef OUTMODULE_HPP
#define OUTMODULE_HPP

#include <gui_generated/containers/OutModuleBase.hpp>

class OutModule : public OutModuleBase
{
public:
    OutModule();
    virtual ~OutModule() {}

    virtual void initialize();

    void select();
    void deselect();
protected:
};

#endif // OUTMODULE_HPP
