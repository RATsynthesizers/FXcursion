#ifndef SELECTIOMODULE_HPP
#define SELECTIOMODULE_HPP

#include <gui_generated/containers/SelectIOModuleBase.hpp>

class SelectIOModule : public SelectIOModuleBase
{
public:
    SelectIOModule();
    virtual ~SelectIOModule() {}

    virtual void initialize();
protected:
};

#endif // SELECTIOMODULE_HPP
