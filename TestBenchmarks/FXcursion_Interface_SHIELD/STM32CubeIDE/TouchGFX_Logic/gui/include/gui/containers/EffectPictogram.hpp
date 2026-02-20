#ifndef EFFECTPICTOGRAM_HPP
#define EFFECTPICTOGRAM_HPP

#include <gui_generated/containers/EffectPictogramBase.hpp>

class EffectPictogram : public EffectPictogramBase
{
public:
    EffectPictogram();
    virtual ~EffectPictogram() {}

    virtual void initialize();

    virtual void setEffect(FXChainItemInfo newEffectInfo);
    virtual FXChainItemInfo getEffect();
    virtual void select(bool select);
    virtual void edit(bool edit);
protected:
    FXChainItemInfo effectInfo;
};

#endif // EFFECTPICTOGRAM_HPP
