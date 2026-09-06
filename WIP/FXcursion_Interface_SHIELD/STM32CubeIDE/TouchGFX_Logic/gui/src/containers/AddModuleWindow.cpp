#include <gui/containers/AddModuleWindow.hpp>
#include <touchgfx/Color.hpp>

const ModuleName AddModuleWindow::eModuleNames[4] =
{
    MODULE_FX,
    MODULE_REC,
    MODULE_LOOP,
    MODULE_MIX,
};

inline uint32_t getAddFxColor() {
    return touchgfx::Color::getColorFromRGB(255, 215, 140);
}

inline uint32_t getAddRecorderColor() {
    return touchgfx::Color::getColorFromRGB(255, 145, 145);
}

inline uint32_t getAddLooperColor() {
    return touchgfx::Color::getColorFromRGB(232, 138, 255);
}

inline uint32_t getAddMixerColor() {
    return touchgfx::Color::getColorFromRGB(127, 255, 122);
}

AddModuleWindow::AddModuleWindow()
{
	eCursor = ADD_MODULE_FX;

	for(U8 i = 0; i < 4; i++)
	{
		bIsModuleAvailable[i] = TRUE;
	}
}

void AddModuleWindow::initialize()
{
    cursorSelect(eCursor);

    AddModuleWindowBase::initialize();
}

void AddModuleWindow::selectFirst()
{
	cursorDeselect(eCursor);

	for(U8 i = 0; i < 4; i++)
	{
		if(TRUE == bIsModuleAvailable[i])
		{
			eCursor = (AddModuleType) i;
			break;
		}
	}

	cursorSelect(eCursor);
	invalidate();
}

void AddModuleWindow::selectUp()
{
	if(eCursor > ADD_MODULE_FX)
	{
		for(U8 i = 1; i < (U8)eCursor + 1; i++)
		{
			if(TRUE == bIsModuleAvailable[(U8)eCursor - i])
			{
				cursorDeselect(eCursor);
				eCursor = (AddModuleType) ((U8)eCursor - i);
				cursorSelect(eCursor);
				invalidate();
				break;
			}
		}
	}
}

void AddModuleWindow::selectDown()
{
	if(eCursor < ADD_MODULE_MIX)
	{
		for(U8 i = 1; i < (U8)ADD_MODULE_MIX - (U8)eCursor + 1; i++)
		{
			if(TRUE == bIsModuleAvailable[(U8)eCursor + i])
			{
				cursorDeselect(eCursor);
				eCursor = (AddModuleType) ((U8)eCursor + i);
				cursorSelect(eCursor);
				invalidate();
				break;
			}
		}
	}
}

void AddModuleWindow::blockSelect(ModuleName eModuleName)
{
	switch(eModuleName)
	{
	case MODULE_FX:
		bIsModuleAvailable[ADD_MODULE_FX] = FALSE;
		effectBox.setColor(touchgfx::Color::getColorFromRGB(128, 128, 128));
		effectBox.setBorderColor(touchgfx::Color::getColorFromRGB(92, 92, 92));
		break;
	case MODULE_REC:
		bIsModuleAvailable[ADD_MODULE_REC] = FALSE;
		recorderBox.setColor(touchgfx::Color::getColorFromRGB(128, 128, 128));
		recorderBox.setBorderColor(touchgfx::Color::getColorFromRGB(92, 92, 92));
		break;
	case MODULE_LOOP:
		bIsModuleAvailable[ADD_MODULE_LOOP] = FALSE;
		looperBox.setColor(touchgfx::Color::getColorFromRGB(128, 128, 128));
		looperBox.setBorderColor(touchgfx::Color::getColorFromRGB(92, 92, 92));
		break;
	case MODULE_MIX:
		bIsModuleAvailable[ADD_MODULE_MIX] = FALSE;
		mixerBox.setColor(touchgfx::Color::getColorFromRGB(128, 128, 128));
	    mixerBox.setBorderColor(touchgfx::Color::getColorFromRGB(92, 92, 92));
		break;
	default:
		break;
	}
}

void AddModuleWindow::unblockSelect(ModuleName eModuleName)
{
	switch(eModuleName)
	{
	case MODULE_FX:
		bIsModuleAvailable[ADD_MODULE_FX] = TRUE;
		effectBox.setColor(getAddFxColor());
		effectBox.setBorderColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		break;
	case MODULE_REC:
		bIsModuleAvailable[ADD_MODULE_REC] = TRUE;
		recorderBox.setColor(getAddRecorderColor());
		recorderBox.setBorderColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		break;
	case MODULE_LOOP:
		bIsModuleAvailable[ADD_MODULE_LOOP] = TRUE;
		looperBox.setColor(getAddLooperColor());
		looperBox.setBorderColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		break;
	case MODULE_MIX:
		bIsModuleAvailable[ADD_MODULE_MIX] = TRUE;
		mixerBox.setColor(getAddMixerColor());
	    mixerBox.setBorderColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		break;
	default:
		break;
	}
}

void AddModuleWindow::cursorSelect(AddModuleType eModuleType)
{
	eCurrentModuleName = eModuleNames[(U8) eModuleType];

	switch(eModuleType)
	{
	case ADD_MODULE_FX:
		fxText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));

		effectBox.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		effectBox.setBorderColor(touchgfx::Color::getColorFromRGB(255, 255, 255));

		break;
	case ADD_MODULE_REC:
		recorderText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));

		recorderBox.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		recorderBox.setBorderColor(touchgfx::Color::getColorFromRGB(255, 255, 255));

		break;
	case ADD_MODULE_LOOP:
		looperText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));

		looperBox.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
		looperBox.setBorderColor(touchgfx::Color::getColorFromRGB(255, 255, 255));

		break;
	case ADD_MODULE_MIX:
		mixerText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));

		mixerBox.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
	    mixerBox.setBorderColor(touchgfx::Color::getColorFromRGB(255, 255, 255));

		break;
	default:
		break;
	}
}

void AddModuleWindow::cursorDeselect(AddModuleType eModuleType)
{
    // Define the blocked colors matching your blockSelect logic
    const uint32_t blockedColor = touchgfx::Color::getColorFromRGB(128, 128, 128);
    const uint32_t blockedBorder = touchgfx::Color::getColorFromRGB(92, 92, 92);
    const uint32_t normalBorder = touchgfx::Color::getColorFromRGB(0, 0, 0);

    // Check if the specific module is available
    bool isAvailable = bIsModuleAvailable[(U8)eModuleType];

	switch(eModuleType)
	{
	case ADD_MODULE_FX:
		fxText.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));

        if(isAvailable)
        {
		    effectBox.setColor(getAddFxColor());
		    effectBox.setBorderColor(normalBorder);
        }
        else
        {
            effectBox.setColor(blockedColor);
            effectBox.setBorderColor(blockedBorder);
        }
		break;

	case ADD_MODULE_REC:
		recorderText.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));

        if(isAvailable)
        {
		    recorderBox.setColor(getAddRecorderColor());
		    recorderBox.setBorderColor(normalBorder);
        }
        else
        {
            recorderBox.setColor(blockedColor);
            recorderBox.setBorderColor(blockedBorder);
        }
		break;

	case ADD_MODULE_LOOP:
		looperText.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));

        if(isAvailable)
        {
		    looperBox.setColor(getAddLooperColor());
		    looperBox.setBorderColor(normalBorder);
        }
        else
        {
            looperBox.setColor(blockedColor);
            looperBox.setBorderColor(blockedBorder);
        }
		break;

	case ADD_MODULE_MIX:
		mixerText.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));

        if(isAvailable)
        {
		    mixerBox.setColor(getAddMixerColor());
	        mixerBox.setBorderColor(normalBorder);
        }
        else
        {
            mixerBox.setColor(blockedColor);
            mixerBox.setBorderColor(blockedBorder);
        }
		break;

	default:
		break;
	}
}

ModuleName AddModuleWindow::getAddModuleName()
{
	return eCurrentModuleName;
}
