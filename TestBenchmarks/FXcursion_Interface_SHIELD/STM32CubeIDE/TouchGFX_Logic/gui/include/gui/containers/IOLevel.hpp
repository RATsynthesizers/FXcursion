#ifndef IOLEVEL_HPP
#define IOLEVEL_HPP

#include <gui_generated/containers/IOLevelBase.hpp>

class IOLevel : public IOLevelBase
{
public:
    IOLevel();
    virtual ~IOLevel() {}

    virtual void initialize();
protected:
};

#endif // IOLEVEL_HPP
