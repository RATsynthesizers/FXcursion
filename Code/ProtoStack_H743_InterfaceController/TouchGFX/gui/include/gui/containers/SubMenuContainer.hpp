#ifndef SUBMENUCONTAINER_HPP
#define SUBMENUCONTAINER_HPP

#include <gui_generated/containers/SubMenuContainerBase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

class SubMenuContainer: public SubMenuContainerBase {
public:
	SubMenuContainer();
	virtual ~SubMenuContainer() {
	}

	virtual void initialize();

	virtual void scrollContents(int8_t scrollAmount);
	virtual void resetSubMenu(bool isFirst, bool isLast);

	virtual void openDeletingEffect(TEXTS effectNameID);
	virtual void closeDeletingEffect();

	virtual uint8_t getCurrentPos();
	virtual bool getIsDeletingEffect();


protected:

	bool isDeletingEffect = false;
	uint8_t currentPos = 0;

    SubMenuItem* subMenuItems[4] =
	{
		&subMenuItem0,
		&subMenuItem1,
		&subMenuItem2,
		&subMenuItem3
	};
};

#endif // SUBMENUCONTAINER_HPP
