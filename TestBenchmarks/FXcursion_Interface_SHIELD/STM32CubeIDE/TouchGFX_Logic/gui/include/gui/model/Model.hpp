#ifndef MODEL_HPP
#define MODEL_HPP

#include <texts/TextKeysAndLanguages.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include <string.h>
#include <stdio.h>
#include "common_cfg.h"
#include "cmsis_os.h"

#define CHANNELS_NUM 6
#define MAX_EFFECTS_NUM 4
#define EFFECT_TYPES 10
#define MAX_PARAMETERS 8

typedef enum enModuleName
{
	MODULE_NONE				= 0,
	MODULE_INPUT			= 1,
	MODULE_OUTPUT			= 2,
	MODULE_FX				= 3,
	MODULE_LOOP				= 4,
	MODULE_REC				= 5,
	MODULE_MIX				= 6,
	MODULE_STOMP			= 7,

} ModuleName;

typedef enum enModuleSelectType
{
	SELECT_INPUT					= 0,
	SELECT_OUTPUT					= 1,
	SELECT_MONO_CHAIN_1 			= 2,
	SELECT_MONO_CHAIN_2 			= 3,
	SELECT_MONO_CHAIN_3	 			= 4,
	SELECT_MONO_CHAIN_4 			= 5,
	SELECT_STEREO_CHAIN_1 			= 6,
	SELECT_STEREO_CHAIN_2 			= 7,
	SELECT_STOMP_BOARD	 			= 8,

} ModuleSelectType;

typedef enum enChannelType
{
	CHANNEL_MONO_1 					= 0,
	CHANNEL_MONO_2 					= 1,
	CHANNEL_MONO_3	 				= 2,
	CHANNEL_MONO_4 					= 3,
	CHANNEL_STEREO_1 				= 4,
	CHANNEL_STEREO_2 				= 5,

} ChannelType;

typedef enum enChainModuleNumber
{
	CHAIN_MODULE_1 = 0,
	CHAIN_MODULE_2 = 1,
	CHAIN_MODULE_3 = 2,
	CHAIN_MODULE_4 = 3,

} ChainModuleNumber;

typedef enum enFootSwitches
{
	FOOT_SWITCH_1 = 0,
	FOOT_SWITCH_2 = 1,
	FOOT_SWITCH_3 = 2,

} FootSwitches;

struct FXChainItemInfo {
	TEXTS eEffectNameID;
	U8 nEffectsAmount;
	U16 nBitmapRegular;
	U16 nBitmapSelected;
};

struct EffectInfo {
	TEXTS eEffectNameID;
	U8 nEffectsAmount;
	BOOLEAN bAvailable;
	U16 nBitmapRegular;
	U16 nBitmapSelected;
};

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

	/******* Clearing functions for modules *******/

    void clearFXChain(ChannelType channel);

	/******* Saving and getting global info to and from model *******/

	void saveProjectName(char* const projName)
	{
		sprintf(projectName, projName, strlen(projName) + 1);
	}
	char* getProjectName()
	{
		return projectName;
	}

	void saveBatteryState(U8 batteryState)
	{
		nBatteryState = batteryState;
	}
	U8 getBatteryState()
	{
		return nBatteryState;
	}

	void saveBPM(U8 BPM)
	{
		nBPM = BPM;
	}
	U8 getBPM()
	{
		return nBPM;
	}

	void saveInputIsStereo1(BOOLEAN inputIsStereo)
	{
		bInputIsStereo1 = inputIsStereo;
	}
	BOOLEAN getInputIsStereo1()
	{
		return bInputIsStereo1;
	}

	void saveInputIsStereo2(BOOLEAN inputIsStereo)
	{
		bInputIsStereo2 = inputIsStereo;
	}
	BOOLEAN getInputIsStereo2()
	{
		return bInputIsStereo2;
	}

	void saveMixerPosition(S8 mixerPosition)
	{
		nMixerPosition = mixerPosition;
	}
	S8 getMixerPosition()
	{
		return nMixerPosition;
	}

	void saveSelectedModule(ModuleSelectType selectedModule)
	{
		eSelectedModule = selectedModule;
	}
	ModuleSelectType getSelectedModule()
	{
		return eSelectedModule;
	}

	void savePrevSelectedModule(ModuleSelectType prevSelectedModule)
	{
		ePrevSelectedModule = prevSelectedModule;
	}
	ModuleSelectType getPrevSelectedModule()
	{
		return ePrevSelectedModule;
	}

	void saveSelectedChainModule(ChainModuleNumber selectedChainModule)
	{
		eSelectedChainModule = selectedChainModule;
	}
	ChainModuleNumber getSelectedChainModule()
	{
		return eSelectedChainModule;
	}

	void saveSelectedFootSwitch(FootSwitches selectedFootSwitch)
	{
		eSelectedFootSwitch = selectedFootSwitch;
	}
	FootSwitches getSelectedFootSwitch()
	{
		return eSelectedFootSwitch;
	}

	void saveMonoModulePosition(ModuleName moduleName, U8 monoChainNumber, U8 chainModuleNumber)
	{
		if(monoChainNumber >= 0
			&& monoChainNumber < 4
			&& chainModuleNumber >= 0
			&& chainModuleNumber < 4)
		{
			eAddedMonoModule[monoChainNumber][chainModuleNumber] = moduleName;
		}
	}
	ModuleName getMonoModuleInPosition(U8 monoChainNumber, U8 chainModuleNumber)
	{
		if(monoChainNumber >= 0
			&& monoChainNumber < 4
			&& chainModuleNumber >= 0
			&& chainModuleNumber < 4)
		{
			return eAddedMonoModule[monoChainNumber][chainModuleNumber];
		}
		else
		{
			return MODULE_NONE;
		}
	}


	void saveStereoModulePosition(ModuleName moduleName, U8 stereoChainNumber, U8 chainModuleNumber)
	{
		if(stereoChainNumber >= 0
			&& stereoChainNumber < 2
			&& chainModuleNumber >= 0
			&& chainModuleNumber < 4)
		{
			eAddedStereoModule[stereoChainNumber][chainModuleNumber] = moduleName;
		}
	}
	ModuleName getStereoModuleInPosition(U8 stereoChainNumber, U8 chainModuleNumber)
	{
		if(stereoChainNumber >= 0
			&& stereoChainNumber < 2
			&& chainModuleNumber >= 0
			&& chainModuleNumber < 4)
		{
			return eAddedStereoModule[stereoChainNumber][chainModuleNumber];
		}
		else
		{
			return MODULE_NONE;
		}
	}

	void saveSelectedChannel(ChannelType channel)
	{
		eSelectedChannel = channel;
	}
	ChannelType getSelectedChannel()
	{
		return eSelectedChannel;
	}

	void saveFXChain(ChannelType channel, FXChainItemInfo* menuItemInfoArray)
	{
		for(int i = 0; i < MAX_EFFECTS_NUM; i++)
		{
			channelFXInfo[channel][i] = menuItemInfoArray[i];
		}
	}
	FXChainItemInfo getFXChainItem(ChannelType channel, U8 effectNum)
	{
		return channelFXInfo[channel][effectNum];
	}

	void saveFXParam(ChannelType channel, U8 effectNum, U8 paramNum, U8 paramValue)
	{
		effectParameters[channel][effectNum][paramNum] = paramValue;
	}
	U8 getFXParam(ChannelType channel, U8 effectNum, U8 paramNum)
	{
		return effectParameters[channel][effectNum][paramNum];
	}
	void moveFXParams(ChannelType channel, U8 effectNum, S8 direction)
	{
	    int target = (int)effectNum + direction;

	    // Clamp target index
	    if (target < 0)
	        target = 0;
	    if (target >= MAX_EFFECTS_NUM)
	        target = MAX_EFFECTS_NUM - 1;

	    // Nothing to do
	    if (target == effectNum)
	        return;

	    // Temporary storage for parameters
	    U8 temp[MAX_PARAMETERS];

	    // Copy moving block
	    memcpy(temp,
	           effectParameters[channel][effectNum],
	           MAX_PARAMETERS * sizeof(U8));

	    if (target < effectNum)
	    {
	        // Moving UP (towards lower index)

	        // Shift elements down
	        for (int i = effectNum; i > target; --i)
	        {
	            memcpy(effectParameters[channel][i],
	                   effectParameters[channel][i - 1],
	                   MAX_PARAMETERS * sizeof(U8));
	        }
	    }
	    else
	    {
	        // Moving DOWN (towards higher index)

	        // Shift elements up
	        for (int i = effectNum; i < target; ++i)
	        {
	            memcpy(effectParameters[channel][i],
	                   effectParameters[channel][i + 1],
	                   MAX_PARAMETERS * sizeof(U8));
	        }
	    }

	    // Insert stored block
	    memcpy(effectParameters[channel][target],
	           temp,
	           MAX_PARAMETERS * sizeof(U8));
	}

	void saveEffectInfo(ChannelType channel, EffectInfo *effectInfoArray)
	{
		for (int i = 0; i < EFFECT_TYPES; i++)
		{
			channelFXPool[channel][i] = effectInfoArray[i];
		}
	}
	EffectInfo getEffectInfo(ChannelType channel, U8 effectPosition)
	{
		return channelFXPool[channel][effectPosition];
	}

	//scroll position of chain on each channel
	void saveChannelChainPosition(ChannelType channel, U8 saveChannelPos)
	{
		channelFXChainPosition[channel] = saveChannelPos;
	}
	U8 getChannelChainPosition(ChannelType channel)
	{
		return channelFXChainPosition[channel];
	}


protected:
    ModelListener* modelListener;

    /// Model variables

    char projectName[20];
    U8 nBatteryState;
    U8 nBPM;
    BOOLEAN bIsFuncPressed;

    /// System screen variables

    BOOLEAN bInputIsStereo1;
    BOOLEAN bInputIsStereo2;
    S8 nMixerPosition;

    ModuleSelectType eSelectedModule;
    ModuleSelectType ePrevSelectedModule;
    ChainModuleNumber eSelectedChainModule;
    FootSwitches eSelectedFootSwitch;

    ModuleName eAddedMonoModule[4][4];
    ModuleName eAddedStereoModule[2][4];

    /// Global module settings variables

    ChannelType eSelectedChannel;

    /// FX chain settings variables

	FXChainItemInfo channelFXInfo[CHANNELS_NUM][MAX_EFFECTS_NUM];
	EffectInfo channelFXPool[CHANNELS_NUM][EFFECT_TYPES];
	U8 channelFXChainPosition[CHANNELS_NUM];

	/// FX parameters store

	U8 effectParameters[CHANNELS_NUM][MAX_EFFECTS_NUM][MAX_PARAMETERS];

};

#endif // MODEL_HPP
