#ifndef STEREOCABLE_HPP
#define STEREOCABLE_HPP

#include <gui_generated/containers/StereoCableBase.hpp>

class StereoCable : public StereoCableBase
{
public:
    StereoCable();
    virtual ~StereoCable() {}

    virtual void initialize();
protected:
};

#endif // STEREOCABLE_HPP
