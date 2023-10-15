#ifndef MENUITEM_HPP
#define MENUITEM_HPP

#include <gui_generated/containers/MenuItemBase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class MenuItem : public MenuItemBase
{
public:
    MenuItem();
    virtual ~MenuItem() {}

    virtual void initialize();

    virtual void setEffect(MenuItemInfo newEffectInfo);
    virtual void select(bool select);
protected:
    MenuItemInfo effectInfo;
    bool isSelected = false;
};

#endif // MENUITEM_HPP
