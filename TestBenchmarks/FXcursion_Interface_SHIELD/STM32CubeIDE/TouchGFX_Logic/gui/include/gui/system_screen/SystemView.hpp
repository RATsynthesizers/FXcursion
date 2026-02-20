#ifndef SYSTEMVIEW_HPP
#define SYSTEMVIEW_HPP

#include <gui_generated/system_screen/SystemViewBase.hpp>
#include <gui/system_screen/SystemPresenter.hpp>

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
    void btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed);
    void btnUpUpdate(BOOLEAN bIsFuncPressed);
    void btnDownUpdate(BOOLEAN bIsFuncPressed);
protected:

    static const U8 MIXER_Y_POS = 40;
    static const U8 MIXER_X_POSITIONS[4];

    BOOLEAN bIsMixerAdded;
    ChainModuleNumber eMixerPosition;

    InputType_t inputType;
    ModuleSelectType ePrevSelect;
    ModuleSelectType eCurrentSelect;

    MonoChain* monoChain[4] =
    {
    	&monoChain1,
		&monoChain2,
		&monoChain3,
		&monoChain4
    };

    StereoChain* stereoChain[2] =
    {
    	&stereoChain1,
		&stereoChain2
    };
};

#endif // SYSTEMVIEW_HPP
