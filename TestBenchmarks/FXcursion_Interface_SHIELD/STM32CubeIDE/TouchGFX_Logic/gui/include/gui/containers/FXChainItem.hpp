#ifndef FXCHAINITEM_HPP
#define FXCHAINITEM_HPP

#include <gui_generated/containers/FXChainItemBase.hpp>

class FXChainItem : public FXChainItemBase
{
public:
    FXChainItem();
    virtual ~FXChainItem() {}

    virtual void initialize();

    void setEffect(FXChainItemInfo newEffectInfo);
    void select(bool select);
protected:

    FXChainItemInfo effectInfo;
    bool isSelected = false;
};

#endif // FXCHAINITEM_HPP
