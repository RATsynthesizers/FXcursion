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

    /*
     * whiteBox is the item's fill when it is NOT selected, so this is the
     * "main colour" that goes grey on bypass.
     *
     * NOTE a gap worth deciding on: while an item IS selected, whiteBox is
     * hidden and blackBox shows instead, so a selected bypassed effect looks
     * exactly like a selected active one. Greying blackBox's white border
     * would close it, but that is a colour-scheme decision rather than a
     * mechanical one, so it is left alone for now.
     */
    touchgfx::colortype activeColor;
    touchgfx::colortype bypassedColor;
};

#endif // FXCHAINITEM_HPP
