#ifndef SYSTEMVIEW_HPP
#define SYSTEMVIEW_HPP

#include <gui_generated/system_screen/SystemViewBase.hpp>
#include <gui/system_screen/SystemPresenter.hpp>

typedef enum enModuleSelectType
{
	MODULE_INPUT			= 0,
	MODULE_OUTPUT			= 1,
	MONO_CHAIN_1 			= 2,
	MONO_CHAIN_2 			= 3,
	MONO_CHAIN_3	 		= 4,
	MONO_CHAIN_4 			= 5,
	STEREO_CHAIN_1 			= 6,
	STEREO_CHAIN_2 			= 7,
	STOMP_BOARD	 			= 8,

} ModuleSelectType;

typedef struct stInputType
{
	BOOLEAN bIsStereo1;
	BOOLEAN bIsStereo2;

} InputType_t;

class SystemView : public SystemViewBase
{
public:
    SystemView();
    virtual ~SystemView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnUpUpdate();
    void btnDownUpdate();
protected:


    InputType_t inputType;
    ModuleSelectType ePrevSelect;
    ModuleSelectType eCurrentSelect;
};

#endif // SYSTEMVIEW_HPP
