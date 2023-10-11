#ifndef SUBSETTINGSCONTAINER_HPP
#define SUBSETTINGSCONTAINER_HPP

#include <gui_generated/containers/SubSettingsContainerBase.hpp>

class SubSettingsContainer : public SubSettingsContainerBase
{
public:
    SubSettingsContainer();
    virtual ~SubSettingsContainer() {}

    virtual void initialize();

	virtual void scrollContents(int8_t scrollAmount);
	virtual void resetSubMenu(uint8_t selectedChain, bool grayDeleteWire, bool grayCreateWire);
	virtual uint8_t getCurrentPos();
protected:

	uint8_t currentPos = 0;

    SubMenuItem* subMenuItems[3] =
	{
		&subMenuItem0,
		&subMenuItem1,
		&subMenuItem2
	};
};

#endif // SUBSETTINGSCONTAINER_HPP
