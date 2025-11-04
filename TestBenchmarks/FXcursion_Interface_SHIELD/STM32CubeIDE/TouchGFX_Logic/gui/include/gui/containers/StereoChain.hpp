#ifndef STEREOCHAIN_HPP
#define STEREOCHAIN_HPP

#include <gui_generated/containers/StereoChainBase.hpp>

class StereoChain : public StereoChainBase
{
public:
    StereoChain();
    virtual ~StereoChain() {}

    virtual void initialize();
protected:
};

#endif // STEREOCHAIN_HPP
