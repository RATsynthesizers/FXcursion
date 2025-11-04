#ifndef STEREOUNAVAILABLE_HPP
#define STEREOUNAVAILABLE_HPP

#include <gui_generated/containers/StereoUnavailableBase.hpp>

class StereoUnavailable : public StereoUnavailableBase
{
public:
    StereoUnavailable();
    virtual ~StereoUnavailable() {}

    virtual void initialize();
protected:
};

#endif // STEREOUNAVAILABLE_HPP
