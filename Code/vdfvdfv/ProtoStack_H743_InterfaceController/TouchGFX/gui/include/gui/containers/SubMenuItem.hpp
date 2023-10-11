#ifndef SUBMENUITEM_HPP
#define SUBMENUITEM_HPP

#include <gui_generated/containers/SubMenuItemBase.hpp>

class SubMenuItem : public SubMenuItemBase
{
public:
    SubMenuItem();
    virtual ~SubMenuItem() {}

    virtual void initialize();

    virtual void select();
    virtual void deSelect();
    virtual void setGray();
    virtual void resetGray();
    virtual void setText(const uint8_t* text);

    bool isGray = false;
    bool isSelected = false;
protected:
};

#endif // SUBMENUITEM_HPP
