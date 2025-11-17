#ifndef STEREOCHAIN_HPP
#define STEREOCHAIN_HPP

#include <gui_generated/containers/StereoChainBase.hpp>

typedef enum enSteroChainModules
{
	STEREO_CHAIN_MODULE_1 = 0,
	STEREO_CHAIN_MODULE_2 = 1,
	STEREO_CHAIN_MODULE_3 = 2,
	STEREO_CHAIN_MODULE_4 = 3,

} StereoChainModules;

class StereoChain : public StereoChainBase
{
public:
    StereoChain();
    virtual ~StereoChain() {}

    virtual void initialize();

    void select(StereoChainModules eModuleNum);
    void deselect();
    StereoChainModules getSelectedModule();
protected:

    StereoChainModules eSelectedModule;
};

#endif // STEREOCHAIN_HPP
