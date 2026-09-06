#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>

extern "C" {

#include "pubsub.h"

#include "stdio.h"

}

static SUB_HANDLE xUISurveyHandle;
static SUB_HANDLE xTelemetryHandle;

Model::Model() : modelListener(0)
{
    if (RESULT_NOT_OK == PUBSUB_CreateTopic(PUBSUB_TOPIC_UI, sizeof(UIObjectInfo_t)))
    {
    	printf("UI Survey topic create failed!\n");
    }

	xUISurveyHandle = PUBSUB_Subscribe(PUBSUB_TOPIC_UI, NULL);

	/* The CtrlLink task creates this topic, so subscribing can fail if the GUI
	   wins the race at startup. That is survivable - no telemetry until the
	   next attempt - but silently showing stale meters forever is not, which is
	   what isAudioAlive() is for. */
	xTelemetryHandle = PUBSUB_Subscribe(PUBSUB_TOPIC_TELEMETRY, NULL);

	memset(&tTelemetry, 0, sizeof(tTelemetry));

	/* Nothing has been heard from the audio controller yet, so the first
	   telemetry frame counts as a dead -> alive edge and triggers the initial
	   configuration send. See tick(). */
	bAudioWasAlive = FALSE;
	bConfigDirty   = FALSE;

	/* Initialize model variables */

	snprintf(projectName, sizeof("Untitled"), "Untitled");
	nBatteryState = 100;
	nBPM = 120;
	bIsFuncPressed = FALSE;

	recInfo.mono[0] = FALSE;
	recInfo.mono[1] = FALSE;
	recInfo.mono[2] = FALSE;
	recInfo.mono[3] = FALSE;
	recInfo.stereo1 = FALSE;
	recInfo.stereo2 = FALSE;

	/* Initializing system screen variables */

	// TODO: if loading project, read values from config file or EEPROM
	bInputIsStereo1 = FALSE;
	bInputIsStereo2 = FALSE;
	nMixerPosition = -1;
	eSelectedModule = SELECT_INPUT;
	eSelectedChainModule = CHAIN_MODULE_1;
	eSelectedFootSwitch = FOOT_SWITCH_1;

	for(U8 i = 0; i < 4; i++)
	{
		for(U8 j = 0; j < 4; j++)
		{
			eAddedMonoModule[i][j] = MODULE_NONE;
		}
	}
	for(U8 i = 0; i < 2; i++)
	{
		for(U8 j = 0; j < 4; j++)
		{
			eAddedStereoModule[i][j] = MODULE_NONE;
		}
	}

	/* Initializing global module settings variables */

    eSelectedChannel = CHANNEL_MONO_1;

	/* Initializing FX chain settings variables */

	for (int i = 0; i < CHANNELS_NUM; i++)
	{
		clearFXChain((ChannelType) i);
	}
}

void Model::tick()
{
	/*
	 * Re-send the whole configuration whenever the audio controller comes
	 * back. Covers the audio board booting after this one, being plugged in
	 * while running, and recovering from a reset - in all three cases it
	 * starts with an empty graph and has no way to ask for the current one.
	 *
	 * Edge-triggered, so a healthy link carries no repeats: pushConfig runs
	 * once per transition, not once per frame. With no audio board attached
	 * this never fires at all, because isAudioAlive stays FALSE.
	 */
	{
		const BOOLEAN bAlive = isAudioAlive();

		if ((FALSE != bAlive) && (FALSE == bAudioWasAlive))
		{
			pushConfig();
			pushTempo();
		}

		bAudioWasAlive = bAlive;
	}

	/*
	 * Telemetry first, and only the LATEST one. tick() runs at the frame rate
	 * (~60 Hz) while telemetry arrives every 40 ms, so the two are close enough
	 * that a backlog is possible after any hitch - and a view wants the current
	 * meter reading, not a replay of every reading it missed.
	 */
	{
		PROTO_TELEMETRY tFresh;
		BOOLEAN         bGotOne = FALSE;

		if (NULL != xTelemetryHandle)
		{
			while (RESULT_OK == PUBSUB_Update(xTelemetryHandle,
			                                  &tFresh,
			                                  sizeof(tFresh),
			                                  0))
			{
				bGotOne = TRUE;
			}
		}
		else
		{
			/* Lost the startup race with the CtrlLink task; try again. */
			xTelemetryHandle = PUBSUB_Subscribe(PUBSUB_TOPIC_TELEMETRY, NULL);
		}

		if (FALSE != bGotOne)
		{
			memcpy(&tTelemetry, &tFresh, sizeof(tTelemetry));

			if (0 != modelListener)
			{
				modelListener->telemetryUpdate(tTelemetry);
			}
		}
	}

	/*
	 * Drain the input topic, do not sample it.
	 *
	 * This used to be a single `if`, so at most ONE button or encoder event
	 * reached the GUI per frame - about one every 16 ms. Everything the user
	 * did faster than that queued up behind it and, with the topic only a few
	 * items deep, was eventually thrown away by the publisher. A brisk turn of
	 * the menu encoder lost most of its detents.
	 *
	 * Bounded rather than unbounded: draining is cheap, but each event can run
	 * a full navigation handler and invalidate widgets, and starving the rest
	 * of the frame to service a stuck publisher would trade dropped input for
	 * a frozen screen. UI_EVENTS_PER_TICK at 60 Hz is far more than a hand can
	 * generate; anything still queued is handled on the next frame.
	 */
	static const U8 UI_EVENTS_PER_TICK = 8U;

	UIObjectInfo_t uiObjectInfo;
	U8             nEventsHandled = 0U;

    while ((nEventsHandled < UI_EVENTS_PER_TICK)
           && (RESULT_OK == PUBSUB_Update(xUISurveyHandle,
                                          &uiObjectInfo,
                                          sizeof(UIObjectInfo_t),
                                          0)))
    {
        nEventsHandled++;

		switch (uiObjectInfo.eName) {
		case BTN_YES:
			modelListener->btnYesUpdate(uiObjectInfo.nValue, bIsFuncPressed);
			break;
		case BTN_NO:
			modelListener->btnNoUpdate(uiObjectInfo.nValue, bIsFuncPressed);
			break;
		case BTN_UP:
			modelListener->btnUpUpdate(bIsFuncPressed);
			break;
		case BTN_DOWN:
			modelListener->btnDownUpdate(bIsFuncPressed);
			break;
		case BTN_FOOT:
			modelListener->btnFootUpdate(uiObjectInfo.nID);
			break;
		case BTN_FUNC:
			switch(uiObjectInfo.nValue)
			{
			case 0:
				bIsFuncPressed = FALSE;
				break;
			case 1:
				bIsFuncPressed = TRUE;
				break;
			default:
				break;
			}
			break;
		case BTN_PARAM:
//			modelListener->btnParamUpdate(uiObjectInfo.nID);
			break;
		case BTN_PLAY:
//			modelListener->btnPlayUpdate(bIsFuncPressed);
			break;
		case BTN_STOP:
//			modelListener->btnStopUpdate(bIsFuncPressed);
			break;
		case BTN_REC:
//			modelListener->btnRecUpdate(bIsFuncPressed);
			break;
		case BTN_MENU:
//			modelListener->btnMenuUpdate();
			break;
		case ENC_MENU:
			modelListener->encMenuUpdate(uiObjectInfo.nValue);
			break;
		case ENC_PARAM:
			modelListener->encParamUpdate(uiObjectInfo.nID, uiObjectInfo.nValue);
			break;
		default:
			break;
		}
	}

	/*
	 * One configuration send per frame, after the input drain.
	 *
	 * Everything above may have asked for one - a fast turn of the menu
	 * encoder while reordering can produce several steps in a single frame -
	 * and they collapse into this. The user still hears each change within a
	 * frame, and the transmit ring cannot be overrun by a burst.
	 */
	if (FALSE != bConfigDirty)
	{
		bConfigDirty = FALSE;
		pushConfig();
	}
}

void Model::clearFXChain(ChannelType channel)
{
	channelFXChainPosition[channel] = 0;

	for (int j = 0; j < MAX_EFFECTS_NUM; j++)
	{
		channelFXInfo[channel][j].eEffectNameID = T_EMPTYEFFECT;
		channelFXInfo[channel][j].nBitmapRegular = BITMAP_EMPTYPICT_ID;
		channelFXInfo[channel][j].nBitmapSelected = BITMAP_EMPTYSELECTEDPICT_ID;
		channelFXInfo[channel][j].nEffectsAmount = 0;
		channelFXInfo[channel][j].bBypassed = FALSE;

		for(int k = 0; k < MAX_PARAMETERS; k++)
		{
			effectParameters[channel][j][k] = 0;
		}
	}

	for (int j = 0; j < EFFECT_TYPES; j++)
	{
		channelFXPool[channel][j].bAvailable = TRUE;
		channelFXPool[channel][j].eEffectNameID = (TEXTS) (T_CHORUSEFFECT + j);

		// TODO: change all bitmaps when they are ready
		switch (channelFXPool[channel][j].eEffectNameID)
		{
		case T_CHORUSEFFECT:
		case T_COMPRESSOREFFECT:
		case T_DELAYEFFECT:
		case T_DISTORTIONEFFECT:
		case T_FLANGEREFFECT:
		case T_OVERDRIVEEFFECT:
		case T_PHASEREFFECT:
		case T_REVERBEFFECT:
		case T_TREMOLOEFFECT:
		case T_VIBRATOEFFECT:
		default:
			channelFXPool[channel][j].nBitmapRegular =
					BITMAP_EMPTYPICT_ID;
			channelFXPool[channel][j].nBitmapSelected =
					BITMAP_EMPTYSELECTEDPICT_ID;
			channelFXPool[channel][j].nEffectsAmount =
					MAX_PARAMETERS;
			break;
		}
	}
}


/***************************************************************************************************
* Talking to the audio controller
*
* Everything below turns GUI state into wire structs. It reads the model and
* calls ctrl_link_if; it never touches a widget, and no view needs to know what
* a PROTO_CFG looks like.
***************************************************************************************************/

/*
 * The GUI's effect list and the protocol's effect pool must stay in step.
 *
 * The GUI names effects with TEXTS ids running T_CHORUSEFFECT..T_VIBRATOEFFECT,
 * ten of them in the same order as the protocol's pool - which has eleven,
 * because it also carries FX_AMP that the GUI does not offer. The protocol
 * stores each effect twice, mono at an even id and stereo at the next one up,
 * so the step is two.
 *
 * The assertion below is the point of writing it this way: it pins both ends of
 * the range, so renumbering either enum - adding an effect to the middle of the
 * GUI list, or inserting one into the shared pool - becomes a build error here
 * instead of the wrong effect quietly appearing on the audio board.
 */
FXC_STATIC_ASSERT(((U8)FX_CHORUS_M
                   + 2U * (U8)(T_VIBRATOEFFECT - T_CHORUSEFFECT))
                  == (U8)FX_VIBRATO_M,
                  gui_texts_match_fx_pool);

/**
 * @brief Mono FX_TYPE base id for a GUI effect name, or FX_TYPE_NONE.
 */
static U8 FxTypeBaseFromTexts(const TEXTS eEffectNameID)
{
	if ((eEffectNameID < T_CHORUSEFFECT) || (eEffectNameID > T_VIBRATOEFFECT))
	{
		/* T_EMPTYEFFECT lands here, which is the common case. */
		return (U8)FX_TYPE_NONE;
	}

	return (U8)((U8)FX_CHORUS_M
	            + 2U * (U8)(eEffectNameID - T_CHORUSEFFECT));
}


const FX_DESC* Model::getFxDesc(ChannelType eChannel, U8 nEffectNum)
{
	if (nEffectNum >= MAX_EFFECTS_NUM)
	{
		return 0;
	}

	const U8 nBase = FxTypeBaseFromTexts(channelFXInfo[eChannel][nEffectNum].eEffectNameID);

	if ((U8)FX_TYPE_NONE == nBase)
	{
		return 0;
	}

	/* The mono and stereo variants are separate descriptors with their own
	   parameter tables, so the width has to be resolved here too - a stereo
	   chorus does not necessarily expose what the mono one does. */
	const U8 nFxType = (U8)FX_VARIANT_FOR_WIDTH(nBase,
	                                            chainWidthForChannel(eChannel));

	if (nFxType >= (U8)FX_TYPE_QTY)
	{
		return 0;
	}

	return &g_aFxDesc[nFxType];
}


U8 Model::currentTopology()
{
	if (FALSE != bInputIsStereo1)
	{
		return (FALSE != bInputIsStereo2) ? (U8)TOPO_2_STEREO
		                                  : (U8)TOPO_ST1_2MONO;
	}

	return (FALSE != bInputIsStereo2) ? (U8)TOPO_2MONO_ST2
	                                  : (U8)TOPO_4_MONO;
}


U8 Model::chainWidthForChannel(ChannelType eChannel)
{
	return (eChannel <= CHANNEL_MONO_4) ? 1U : (U8)CHAIN_MAX_WIDTH;
}


S8 Model::protoChainForChannel(ChannelType eChannel)
{
	/*
	 * Chains are numbered in plane order, so the index of a channel depends on
	 * how many planes the groups before it consumed. Group 1 is planes 0..1
	 * (CHANNEL_MONO_1/2 or CHANNEL_STEREO_1), group 2 is planes 2..3
	 * (CHANNEL_MONO_3/4 or CHANNEL_STEREO_2). A stereo group is one chain
	 * where a mono group is two, which is the whole of the arithmetic below.
	 */
	const U8 nGroup1Chains = (FALSE != bInputIsStereo1) ? 1U : 2U;

	switch (eChannel)
	{
	case CHANNEL_MONO_1:
		return (FALSE == bInputIsStereo1) ? (S8)0 : (S8)-1;

	case CHANNEL_MONO_2:
		return (FALSE == bInputIsStereo1) ? (S8)1 : (S8)-1;

	case CHANNEL_STEREO_1:
		return (FALSE != bInputIsStereo1) ? (S8)0 : (S8)-1;

	case CHANNEL_MONO_3:
		return (FALSE == bInputIsStereo2) ? (S8)nGroup1Chains : (S8)-1;

	case CHANNEL_MONO_4:
		return (FALSE == bInputIsStereo2) ? (S8)(nGroup1Chains + 1U) : (S8)-1;

	case CHANNEL_STEREO_2:
		return (FALSE != bInputIsStereo2) ? (S8)nGroup1Chains : (S8)-1;

	default:
		return (S8)-1;
	}
}


void Model::pushConfig()
{
	PROTO_CFG tCfg;

	memset(&tCfg, 0, sizeof(tCfg));

	tCfg.nVersion  = (U8)PROTO_VERSION;
	tCfg.eTopology = currentTopology();
	tCfg.nMixerCol = (nMixerPosition < 0) ? GRID_MIXER_COL_NONE
	                                      : (S8)nMixerPosition;
	tCfg.bAutoGain = TRUE;

	tCfg.nBpmX10      = (U16)((U16)nBPM * 10U);
	tCfg.nBeatsPerBar = 4U;
	tCfg.nBeatUnit    = 4U;

	/*
	 * Defaults for everything the GUI cannot express yet. These are not
	 * placeholders in the sense of "wrong" - they are the values the audio
	 * side should use until a Mixer or Looper screen exists to change them.
	 * Unity gain is mid-scale because aMixGain maps 0..65535 onto 0.0..2.0.
	 */
	for (U8 i = 0U; i < (U8)CHAIN_MAX_QTY; i++)
	{
		/* Cleared here and set per slot below, from bBypassed. */
		tCfg.aFxEnabled[i] = 0x00U;

		for (U8 j = 0U; j < (U8)CHAIN_MAX_QTY; j++)
		{
			tCfg.aMixGain[i][j] = 32768U;       /* 1.0 linear                 */
			tCfg.aMixPan[i][j]  = 0;            /* centre                     */
		}

		for (U8 j = 0U; j < (U8)GRID_SLOT_QTY; j++)
		{
			tCfg.aSlot[i][j] = (U8)BLOCK_NONE;
		}

		for (U8 j = 0U; j < (U8)FXBLOCK_SLOT_QTY; j++)
		{
			tCfg.aFxSlot[i][j] = (U8)FX_TYPE_NONE;
		}
	}

	for (U8 i = 0U; i < (U8)LOOPER_QTY; i++)
	{
		tCfg.aLoopBars[i] = 4U;
	}

	/* Now the grid itself, one GUI channel at a time. Channels that the
	   current topology does not use return -1 and are skipped, so their stale
	   contents cannot leak into the configuration. */
	for (U8 nCh = 0U; nCh < (U8)CHANNELS_NUM; nCh++)
	{
		const ChannelType eChannel = (ChannelType)nCh;
		const S8          nChain   = protoChainForChannel(eChannel);

		if (nChain < 0)
		{
			continue;
		}

		const U8 nWidth = chainWidthForChannel(eChannel);

		for (U8 nSlot = 0U; nSlot < (U8)GRID_SLOT_QTY; nSlot++)
		{
			/* GUI slot 1 is drawn nearest IN and the protocol's slot 0 is IN,
			   so ChainModuleNumber maps straight onto the slot index. */
			const ModuleName eModule =
					(eChannel <= CHANNEL_MONO_4)
					? eAddedMonoModule[nCh][nSlot]
					: eAddedStereoModule[nCh - (U8)CHANNEL_STEREO_1][nSlot];

			switch (eModule)
			{
			case MODULE_FX:
				tCfg.aSlot[nChain][nSlot] = (U8)BLOCK_FX;
				break;
			case MODULE_REC:
				tCfg.aSlot[nChain][nSlot] = (U8)BLOCK_RECORDER;
				break;
			case MODULE_LOOP:
				tCfg.aSlot[nChain][nSlot] = (U8)BLOCK_LOOPER;
				break;
			default:
				break;
			}
		}

		/* Contents of this chain's FX block. Width picks the mono or stereo
		   variant of each effect - the audio side rejects a mismatch with
		   PROTO_RES_BAD_WIDTH rather than guessing. */
		for (U8 nFx = 0U; nFx < (U8)FXBLOCK_SLOT_QTY; nFx++)
		{
			const U8 nBase = FxTypeBaseFromTexts(channelFXInfo[nCh][nFx].eEffectNameID);

			tCfg.aFxSlot[nChain][nFx] =
					((U8)FX_TYPE_NONE == nBase)
					? (U8)FX_TYPE_NONE
					: (U8)FX_VARIANT_FOR_WIDTH(nBase, nWidth);

			/*
			 * Bypass needed no protocol change at all - aFxEnabled has been
			 * in PROTO_CFG from the start, described as a bitmask over the FX
			 * slots, and was simply hardcoded to "all on" until now.
			 *
			 * An empty slot contributes no bit either, so the audio side gets
			 * one consistent answer to "should I run this block".
			 */
			if (((U8)FX_TYPE_NONE != nBase)
			    && (FALSE == channelFXInfo[nCh][nFx].bBypassed))
			{
				tCfg.aFxEnabled[nChain] |= (U8)(1U << nFx);
			}
		}
	}

	/*
	 * The mixer is a WHOLE COLUMN, not a cell, and the audio side validates
	 * that - a partially filled column comes back as PROTO_RES_BAD_GRID. So
	 * it is written last, across every chain the topology uses.
	 *
	 * NOTE a real gap on the GUI side: btnYesUpdate only refuses to ADD a
	 * mixer to a column that already holds a module, and does not refuse to
	 * add a module to a column already holding the mixer. Doing that produces
	 * a configuration the audio side will reject. Worth closing in the
	 * SystemView navigation work.
	 */
	if (tCfg.nMixerCol != GRID_MIXER_COL_NONE)
	{
		for (U8 nCh = 0U; nCh < (U8)CHANNELS_NUM; nCh++)
		{
			const S8 nChain = protoChainForChannel((ChannelType)nCh);

			if (nChain >= 0)
			{
				tCfg.aSlot[nChain][tCfg.nMixerCol] = (U8)BLOCK_MIXER;
			}
		}
	}

	(void)CtrlLinkIf_SendConfig(&tCfg);
}


void Model::pushParam(ChannelType eChannel,
                      U8 nEffectNum,
                      U8 nParamIdx,
                      U8 nValue8)
{
	if ((nEffectNum >= MAX_EFFECTS_NUM) || (nParamIdx >= FX_PARAM_QTY))
	{
		return;
	}

	const S8 nChain = protoChainForChannel(eChannel);

	if (nChain < 0)
	{
		return;
	}

	const U8 nBase = FxTypeBaseFromTexts(channelFXInfo[eChannel][nEffectNum].eEffectNameID);

	if ((U8)FX_TYPE_NONE == nBase)
	{
		/* Nothing in that FX slot - a stray knob turn, not an error. */
		return;
	}

	PROTO_SET_PARAM tCmd;

	memset(&tCmd, 0, sizeof(tCmd));

	/* 0..255 -> 0..65535, exactly, with both endpoints preserved. */
	tCmd.nValue    = (U16)(((U32)nValue8 * 65535U) / 255U);
	tCmd.nChain    = (U8)nChain;
	tCmd.eFxType   = (U8)FX_VARIANT_FOR_WIDTH(nBase,
	                                          chainWidthForChannel(eChannel));
	tCmd.nParamIdx = nParamIdx;

	/*
	 * Always free-running for now. The tempo-sync behaviour exists in the VST
	 * prototype - a syncable parameter snaps to a NOTE_DIV instead of taking a
	 * continuous value - but the GUI has nowhere to express it yet: there is no
	 * sync toggle on the effect screen and CustomGauge has no discrete mode.
	 * When that lands, bSync and eDivision are the two fields to fill, and
	 * g_aFxDesc's FX_PF_SYNCABLE flag says which parameters may offer it.
	 */
	tCmd.bSync     = FALSE;
	tCmd.eDivision = 0U;

	(void)CtrlLinkIf_SetParam(&tCmd);
}


void Model::pushTempo()
{
	(void)CtrlLinkIf_SetTempo((U16)((U16)nBPM * 10U), 4U, 4U);
}
