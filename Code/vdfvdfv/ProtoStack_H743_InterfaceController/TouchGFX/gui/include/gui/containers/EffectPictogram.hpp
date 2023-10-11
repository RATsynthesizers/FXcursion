#ifndef EFFECTPICTOGRAM_HPP
#define EFFECTPICTOGRAM_HPP

#include <gui_generated/containers/EffectPictogramBase.hpp>

class EffectPictogram : public EffectPictogramBase
{
public:
    EffectPictogram();
    virtual ~EffectPictogram() {}

    virtual void initialize();

    virtual void setEffect(MenuItemInfo newEffectInfo);
    virtual MenuItemInfo getEffect();
    virtual void select(bool select);
    virtual void edit(bool edit);
protected:
    MenuItemInfo effectInfo;
};

#endif // EFFECTPICTOGRAM_HPP
