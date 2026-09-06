#ifndef MONOCHAIN_HPP
#define MONOCHAIN_HPP

#include <gui_generated/containers/MonoChainBase.hpp>

class MonoChain : public MonoChainBase
{
public:
    MonoChain();
    virtual ~MonoChain() {}

    virtual void initialize();

    void select(ChainModuleNumber eModuleNum);
    void deselect(ChainModuleNumber eModuleNum);
    void addModule(ChainModuleNumber eModuleNum, ModuleName eModuleName);
    void deleteSelectedModule();

    ChainModuleNumber getSelectedModuleNumber();
    ModuleName getSelectedModuleName();
    ModuleName getModuleName(ChainModuleNumber eModuleNumber);

protected:

    static const U8 MODULE_Y_POS = 4;
    static const U8 MODULE_X_POSITIONS[4];

    ModuleName aCurrentState[4] = {};

    ChainModuleNumber eSelectedModuleNumber;
    ModuleName		 eSelectedModuleName;
};

#endif // MONOCHAIN_HPP
