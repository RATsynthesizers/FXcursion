#ifndef MONOCHAIN_HPP
#define MONOCHAIN_HPP

#include <gui_generated/containers/MonoChainBase.hpp>

class MonoChain : public MonoChainBase
{
public:
    MonoChain();
    virtual ~MonoChain() {}

    virtual void initialize();
protected:
};

#endif // MONOCHAIN_HPP
