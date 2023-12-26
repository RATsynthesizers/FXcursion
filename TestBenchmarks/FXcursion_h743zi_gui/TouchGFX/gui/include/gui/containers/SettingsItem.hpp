#ifndef SETTINGSITEM_HPP
#define SETTINGSITEM_HPP

#include <gui_generated/containers/SettingsItemBase.hpp>

class SettingsItem : public SettingsItemBase
{
public:
    SettingsItem();
    virtual ~SettingsItem() {}

    virtual void initialize();

    virtual void setName(const uint8_t* text);
    virtual void selectItem(void);
    virtual void deselectItem(void);
    virtual void selectConnection(void);
    virtual void deselectConnection(void);
protected:
};

#endif // SETTINGSITEM_HPP
