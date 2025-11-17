#ifndef MONOCHAIN_HPP
#define MONOCHAIN_HPP

#include <gui_generated/containers/MonoChainBase.hpp>

typedef enum enMonoChainModules
{
	MONO_CHAIN_MODULE_1 = 0,
	MONO_CHAIN_MODULE_2 = 1,
	MONO_CHAIN_MODULE_3 = 2,
	MONO_CHAIN_MODULE_4 = 3,

} MonoChainModules;

class MonoChain : public MonoChainBase
{
public:
    MonoChain();
    virtual ~MonoChain() {}

    virtual void initialize();

    void select(MonoChainModules eModuleNum);
    void deselect();
    MonoChainModules getSelectedModule();
protected:

    MonoChainModules eSelectedModule;
};

#endif // MONOCHAIN_HPP
