#ifndef ADDMODULEWINDOW_HPP
#define ADDMODULEWINDOW_HPP

#include <gui_generated/containers/AddModuleWindowBase.hpp>



typedef enum enAddModuleType
{
	ADD_MODULE_FX					= 0,
	ADD_MODULE_REC					= 1,
	ADD_MODULE_LOOP					= 2,
	ADD_MODULE_MIX 					= 3,

} AddModuleType;

class AddModuleWindow : public AddModuleWindowBase
{
public:
    AddModuleWindow();
    virtual ~AddModuleWindow() {}

    virtual void initialize();

	void selectFirst();
    void selectUp();
	void selectDown();

	void blockSelect(ModuleName eModuleName);
	void unblockSelect(ModuleName eModuleName);

	ModuleName getAddModuleName();
protected:

	void cursorSelect(AddModuleType eModuleType);
	void cursorDeselect(AddModuleType eModuleType);

	static const ModuleName eModuleNames[4];

	BOOLEAN bIsModuleAvailable[4];

	AddModuleType eCursor;
	ModuleName eCurrentModuleName;
};

#endif // ADDMODULEWINDOW_HPP
