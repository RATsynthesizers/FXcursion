#ifndef MODEL_HPP
#define MODEL_HPP

#include <texts/TextKeysAndLanguages.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include <string.h>
#include <stdio.h>
#include "common_cfg.h"
#include "cmsis_os.h"

extern "C" {
/* The wire contract, shared byte for byte with the audio controller. Safe to
 * include from C++ - the headers carry their own extern "C" guards. */
#include "fx_protocol.h"
#include "ctrl_link_if.h"
}

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

	/*
	 * Bypassed effects stay in the chain and keep their parameters; the audio
	 * side simply does not process them.
	 *
	 * The flag lives INSIDE this struct rather than in a parallel array on
	 * purpose. Reordering now writes to the model on every encoder step, and
	 * deleting shifts every later effect down a slot - both of which move
	 * whole FXChainItemInfo values around. Keeping bypass here means it is
	 * carried along by construction; a parallel array would have to be kept in
	 * step at exactly the moments that are easiest to get wrong.
	 */
	BOOLEAN bBypassed;

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

	/*
	 * The caller's string is DATA, not a format.
	 *
	 * This used to be `sprintf(projectName, projName, strlen(projName) + 1)`,
	 * which passes the incoming name as the FORMAT STRING. A project name
	 * containing a percent sign would have read arbitrary values off the
	 * stack to satisfy the conversion, and "%n" would have written to it.
	 * There was no length limit either, and projectName is 20 bytes.
	 *
	 * Nothing calls this yet, which is the only reason it never fired - but
	 * naming a project is the whole point of the field, so it was going to.
	 */
	void saveProjectName(const char* const projName)
	{
		if (NULL != projName)
		{
			snprintf(projectName, sizeof(projectName), "%s", projName);
		}
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

	//recorder info
	void saveRecorderInfo(RecorderInfo_t* recInfoNew)
	{
		memcpy(&recInfo, recInfoNew, sizeof(RecorderInfo_t));
	}
	RecorderInfo_t* getRecorderInfo()
	{
		return &recInfo;
	}

	/*
	 * Telemetry from the audio controller - meters, CPU load, loop playhead,
	 * transport state. Arrives every TELEMETRY_PERIOD_MS on a pubsub topic and
	 * is drained in tick(), the same way button and encoder events already are.
	 *
	 * A torn read of a meter is harmless, which is why this is a plain copy and
	 * not guarded: nSeq is there if a view ever needs to know it saw a whole
	 * frame.
	 */
	const PROTO_TELEMETRY* getTelemetry()
	{
		return &tTelemetry;
	}

	/* FALSE until the audio controller has been heard from, and again if it goes
	 * quiet - so the GUI can say so instead of showing meters frozen at their
	 * last value and looking live. */
	BOOLEAN isAudioAlive()
	{
		return CtrlLinkIf_IsPeerAlive();
	}

	/***********************************************************************
	 * TELLING THE AUDIO CONTROLLER WHAT THE GUI DECIDED
	 *
	 * SAFE WITH NO AUDIO BOARD ATTACHED. Every sender in ctrl_link_if.c is a
	 * push into a byte ring plus a DMA kick - there is no handshake, no ACK
	 * wait and no spin anywhere in the path, so with nothing on the other end
	 * of USART2 the bytes are simply clocked out and lost. Nothing here reads
	 * a reply, so nothing here can hang or time out. The only observable
	 * difference between "connected" and "not connected" is that
	 * isAudioAlive() stays FALSE and no ACK ever arrives.
	 *
	 * SEND STATE, NOT EDITS. There is deliberately no "add effect" message.
	 * Any change to the grid re-sends the whole 96-byte PROTO_CFG and the
	 * audio side rebuilds from it, so a dropped or corrupted frame cannot
	 * leave the two boards holding different pictures of the machine.
	 * Parameters are the one exception - they are addressed by (chain, effect
	 * type, index) and sent individually, because a knob moving must not cost
	 * a full rebuild.
	 **********************************************************************/

	/** Build PROTO_CFG from current model state and send it. */
	void pushConfig();

	/**
	 * Ask for a configuration send at the END OF THIS FRAME rather than now.
	 *
	 * For reordering, where the user should hear each step as it happens but
	 * the sends must not outrun the link. Model::tick drains up to
	 * UI_EVENTS_PER_TICK encoder steps in one frame, and a config frame is
	 * over a hundred bytes against a 512-byte transmit ring - eight
	 * back-to-back sends would overflow it, and FxLink_Send drops silently
	 * when it does. Since configuration is only sent on change, a dropped one
	 * leaves the audio side stale for good.
	 *
	 * Coalescing to one send per frame caps it at the frame rate, which is
	 * still immediate to a human hand, and guarantees the LAST state always
	 * goes out.
	 */
	void pushConfigDeferred()
	{
		bConfigDirty = TRUE;
	}

	/**
	 * Static description of the effect in one FX slot, or NULL if it is empty.
	 *
	 * The single source of truth for a parameter's name and for how many an
	 * effect has - both shared with the audio controller and with the VST,
	 * which is the point. The GUI no longer guesses either.
	 */
	const FX_DESC* getFxDesc(ChannelType eChannel, U8 nEffectNum);

	/**
	 * Send one parameter.
	 *
	 * @param nValue8 the GUI's 0..255 gauge value; scaled to the protocol's
	 *                0..65535 normalised range here, in one place.
	 */
	void pushParam(ChannelType eChannel,
	               U8 nEffectNum,
	               U8 nParamIdx,
	               U8 nValue8);

	/** Send tempo and time signature. */
	void pushTempo();

	/**
	 * Protocol chain index for a GUI channel, or -1 when that channel is not
	 * part of the active topology.
	 *
	 * The audio side numbers chains in PLANE order and chain widths always sum
	 * to AUDIO_CH_QTY, so which GUI channel is chain 0 depends on the stereo
	 * flags: with input 1 stereo, CHANNEL_STEREO_1 is chain 0 and
	 * CHANNEL_MONO_1/2 do not exist at all. Public because a view that wants
	 * to address a chain directly - looper transport, recorder arming - needs
	 * the same answer.
	 */
	S8 protoChainForChannel(ChannelType eChannel);

	/** Planes in a chain: 1 for a mono channel, 2 for a stereo one. */
	U8 chainWidthForChannel(ChannelType eChannel);

	/** TOPOLOGY value for the current input configuration. */
	U8 currentTopology();


protected:
    ModelListener* modelListener;

    PROTO_TELEMETRY tTelemetry;

    /*
     * Liveness on the previous tick, so tick() can spot the dead -> alive
     * EDGE and re-send the configuration.
     *
     * Without this, whoever powers up second loses: the interface sends its
     * configuration when the user changes something, and an audio controller
     * that boots later - or is plugged in later, or reboots - never hears any
     * of it and sits with an empty graph until the user happens to touch the
     * grid again. Re-sending on the edge costs one 96-byte frame per
     * reconnection and makes the pairing order stop mattering.
     */
    BOOLEAN bAudioWasAlive;

    /** Set by pushConfigDeferred, flushed at the end of tick(). */
    BOOLEAN bConfigDirty;

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

	/// Recorder state

	RecorderInfo_t recInfo;

};

#endif // MODEL_HPP
