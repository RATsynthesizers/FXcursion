#include <gui/system_screen/SystemView.hpp>

extern "C" {

#include "pubsub.h"

}

const U8 SystemView::MIXER_X_POSITIONS[4] =
{
	218,
	160,
	102,
	44
};


SystemView::SystemView()
{
	bIsMixerAdded = FALSE;
	eMixerPosition = CHAIN_MODULE_1;

	inputType.bIsStereo1 = FALSE;
	inputType.bIsStereo2 = FALSE;

	ePrevSelect 	= SELECT_INPUT;
	eCurrentSelect 	= SELECT_INPUT;

	nRowQty = 0U;

	for (U8 i = 0U; i < (U8)NAV_ROW_MAX; i++)
	{
		aRow[i].pMono   = 0;
		aRow[i].pStereo = 0;
		aRow[i].eSelect = SELECT_INPUT;
	}
}

void SystemView::setupScreen()
{
	inputType.bIsStereo1 = presenter->getInputIsStereo1();
	inputType.bIsStereo2 = presenter->getInputIsStereo2();

	ModuleName addModule;
	if(TRUE == inputType.bIsStereo1)
	{
		for(U8 j = 0; j < 4; j++)
		{
			addModule = presenter->getStereoModuleInPosition(0, j);
			if(MODULE_NONE != addModule)
			{
				stereoChain1.addModule((ChainModuleNumber) j, addModule);
			}
		}
	}
	else
	{
		for(U8 j = 0; j < 4; j++)
		{
			addModule = presenter->getMonoModuleInPosition(0, j);
			if(MODULE_NONE != addModule)
			{
				monoChain1.addModule((ChainModuleNumber) j, addModule);
			}

			addModule = presenter->getMonoModuleInPosition(1, j);
			if(MODULE_NONE != addModule)
			{
				monoChain2.addModule((ChainModuleNumber) j, addModule);
			}
		}
	}
	if(TRUE == inputType.bIsStereo2)
	{
		for(U8 j = 0; j < 4; j++)
		{
			addModule = presenter->getStereoModuleInPosition(1, j);
			if(MODULE_NONE != addModule)
			{
				stereoChain2.addModule((ChainModuleNumber) j, addModule);
			}
		}
	}
	else
	{
		for(U8 j = 0; j < 4; j++)
		{
			addModule = presenter->getMonoModuleInPosition(2, j);
			if(MODULE_NONE != addModule)
			{
				monoChain3.addModule((ChainModuleNumber) j, addModule);
			}

			addModule = presenter->getMonoModuleInPosition(3, j);
			if(MODULE_NONE != addModule)
			{
				monoChain4.addModule((ChainModuleNumber) j, addModule);
			}
		}
	}

	S8 nMixerPosition = presenter->getMixerPosition();

	if(-1 == nMixerPosition)
	{
		bIsMixerAdded = FALSE;
	}
	else
	{
		bIsMixerAdded = TRUE;
		eMixerPosition = (ChainModuleNumber) nMixerPosition;
		mixModule.setXY(MIXER_X_POSITIONS[eMixerPosition], MIXER_Y_POS);
		mixModule.setVisible(true);
	}

	/* Which containers are rows, and how many, before anything asks. */
	buildRowTable();

	eCurrentSelect = presenter->getSelectedModule();
	ePrevSelect = presenter->getPrevSelectedModule();

	NAV_POS tPos;

	switch(eCurrentSelect)
	{
	case SELECT_OUTPUT:
		tPos.nRow  = NAV_ROW_OUTPUT;
		tPos.nSlot = 0U;
		break;

	case SELECT_STOMP_BOARD:
		tPos.nRow  = NAV_ROW_STOMP;
		tPos.nSlot = (U8)presenter->getSelectedFootSwitch();
		if(tPos.nSlot >= (U8)NAV_FOOT_QTY)
		{
			tPos.nSlot = 0U;
		}
		break;

	case SELECT_MONO_CHAIN_1:
	case SELECT_MONO_CHAIN_2:
	case SELECT_MONO_CHAIN_3:
	case SELECT_MONO_CHAIN_4:
	case SELECT_STEREO_CHAIN_1:
	case SELECT_STEREO_CHAIN_2:
	{
		const S8 nRow = rowIndexForSelect(eCurrentSelect);

		tPos.nRow  = (nRow < 0) ? (S8)0 : nRow;
		tPos.nSlot = (U8)presenter->getSelectedChainModule();
		if(tPos.nSlot >= (U8)NAV_SLOT_QTY)
		{
			tPos.nSlot = 0U;
		}
		break;
	}

	case SELECT_INPUT:
	default:
		tPos.nRow  = NAV_ROW_INPUT;
		tPos.nSlot = 0U;
		break;
	}

	/*
	 * The stored cursor can name a row this topology no longer has - the input
	 * configuration is changed on a different screen, and this view is rebuilt
	 * from the model every time it is entered. rowIndexForSelect collapses it
	 * onto a row that does exist, and eCurrentSelect is rewritten from the
	 * result so the two can never disagree afterwards.
	 */
	eCurrentSelect = selectForPos(tPos);

	applyHighlight(tPos);

	{
		const NAV_CTX tCtx = navCtx();

		if(FALSE != Nav_IsOnMixer(&tCtx, tPos))
		{
			mixModule.select();
		}
	}

    SystemViewBase::setupScreen();
}

void SystemView::tearDownScreen()
{
    SystemViewBase::tearDownScreen();
}


/***************************************************************************************************
* The row table, and moving the cursor over it
*
* See SystemNav.hpp for where the cursor is allowed to go. Everything here is
* the translation between that integer space and the containers on screen.
***************************************************************************************************/

void SystemView::addRow(MonoChain* pMono,
                        StereoChain* pStereo,
                        ModuleSelectType eSelect)
{
	if(nRowQty >= (U8)NAV_ROW_MAX)
	{
		return;
	}

	aRow[nRowQty].pMono   = pMono;
	aRow[nRowQty].pStereo = pStereo;
	aRow[nRowQty].eSelect = eSelect;

	nRowQty++;
}

void SystemView::buildRowTable()
{
	nRowQty = 0U;

	/* Group 1 is the first input pair, group 2 the second. A stereo pair is
	   one row; a mono pair is two. Top to bottom. */
	if(TRUE == inputType.bIsStereo1)
	{
		addRow(0, &stereoChain1, SELECT_STEREO_CHAIN_1);
	}
	else
	{
		addRow(&monoChain1, 0, SELECT_MONO_CHAIN_1);
		addRow(&monoChain2, 0, SELECT_MONO_CHAIN_2);
	}

	if(TRUE == inputType.bIsStereo2)
	{
		addRow(0, &stereoChain2, SELECT_STEREO_CHAIN_2);
	}
	else
	{
		addRow(&monoChain3, 0, SELECT_MONO_CHAIN_3);
		addRow(&monoChain4, 0, SELECT_MONO_CHAIN_4);
	}
}

S8 SystemView::rowIndexForSelect(ModuleSelectType eSelect)
{
	for(U8 i = 0U; i < nRowQty; i++)
	{
		if(aRow[i].eSelect == eSelect)
		{
			return (S8)i;
		}
	}

	/*
	 * Not a row under this topology. Collapse onto the first row of the same
	 * input group, which is what the old code's long
	 * "if (FALSE == bIsStereo1) monoChain2 else stereoChain1" chains added up
	 * to - one of which tested the wrong flag.
	 */
	if((SELECT_MONO_CHAIN_3 == eSelect)
		|| (SELECT_MONO_CHAIN_4 == eSelect)
		|| (SELECT_STEREO_CHAIN_2 == eSelect))
	{
		const U8 nGroup1Rows = (TRUE == inputType.bIsStereo1) ? 1U : 2U;

		return (nGroup1Rows < nRowQty) ? (S8)nGroup1Rows : (S8)0;
	}

	return (S8)0;
}

ModuleSelectType SystemView::selectForPos(const NAV_POS& tPos)
{
	switch(tPos.nRow)
	{
	case NAV_ROW_INPUT:
		return SELECT_INPUT;
	case NAV_ROW_OUTPUT:
		return SELECT_OUTPUT;
	case NAV_ROW_STOMP:
		return SELECT_STOMP_BOARD;
	default:
		break;
	}

	return ((U8)tPos.nRow < nRowQty) ? aRow[tPos.nRow].eSelect : SELECT_INPUT;
}

NAV_CTX SystemView::navCtx()
{
	NAV_CTX tCtx;

	tCtx.nRowQty     = nRowQty;
	tCtx.bMixerAdded = bIsMixerAdded;
	tCtx.nMixerCol   = (TRUE == bIsMixerAdded) ? (S8)eMixerPosition : (S8)-1;

	return tCtx;
}

NAV_POS SystemView::currentPos()
{
	NAV_POS tPos;

	switch(eCurrentSelect)
	{
	case SELECT_OUTPUT:
		tPos.nRow  = NAV_ROW_OUTPUT;
		tPos.nSlot = 0U;
		break;

	case SELECT_STOMP_BOARD:
		tPos.nRow  = NAV_ROW_STOMP;
		tPos.nSlot = (U8)stompBoard.getSelectedFootSwitch();
		break;

	case SELECT_INPUT:
		tPos.nRow  = NAV_ROW_INPUT;
		tPos.nSlot = 0U;
		break;

	default:
	{
		const S8 nRow = rowIndexForSelect(eCurrentSelect);

		tPos.nRow  = (nRow < 0) ? (S8)0 : nRow;
		tPos.nSlot = (U8)rowSelectedSlot((U8)tPos.nRow);
		break;
	}
	}

	return tPos;
}

void SystemView::rowSelect(U8 nRow, ChainModuleNumber eSlot)
{
	if(nRow >= nRowQty)
	{
		return;
	}

	if(0 != aRow[nRow].pMono)
	{
		aRow[nRow].pMono->select(eSlot);
	}
	else if(0 != aRow[nRow].pStereo)
	{
		aRow[nRow].pStereo->select(eSlot);
	}
}

void SystemView::rowDeselect(U8 nRow, ChainModuleNumber eSlot)
{
	if(nRow >= nRowQty)
	{
		return;
	}

	if(0 != aRow[nRow].pMono)
	{
		aRow[nRow].pMono->deselect(eSlot);
	}
	else if(0 != aRow[nRow].pStereo)
	{
		aRow[nRow].pStereo->deselect(eSlot);
	}
}

ChainModuleNumber SystemView::rowSelectedSlot(U8 nRow)
{
	if(nRow < nRowQty)
	{
		if(0 != aRow[nRow].pMono)
		{
			return aRow[nRow].pMono->getSelectedModuleNumber();
		}

		if(0 != aRow[nRow].pStereo)
		{
			return aRow[nRow].pStereo->getSelectedModuleNumber();
		}
	}

	return CHAIN_MODULE_1;
}

void SystemView::applyHighlight(const NAV_POS& tPos)
{
	switch(tPos.nRow)
	{
	case NAV_ROW_INPUT:
		inModule.select();
		break;
	case NAV_ROW_OUTPUT:
		outModule.select();
		break;
	case NAV_ROW_STOMP:
		stompBoard.select((FootSwitches)tPos.nSlot);
		break;
	default:
		rowSelect((U8)tPos.nRow, (ChainModuleNumber)tPos.nSlot);
		break;
	}
}

void SystemView::clearHighlight(const NAV_POS& tPos)
{
	switch(tPos.nRow)
	{
	case NAV_ROW_INPUT:
		inModule.deselect();
		break;
	case NAV_ROW_OUTPUT:
		outModule.deselect();
		break;
	case NAV_ROW_STOMP:
		stompBoard.deselect();
		break;
	default:
		rowDeselect((U8)tPos.nRow, (ChainModuleNumber)tPos.nSlot);
		break;
	}
}

void SystemView::moveCursor(const NAV_POS& tOld, const NAV_POS& tNew)
{
	if((tOld.nRow == tNew.nRow) && (tOld.nSlot == tNew.nSlot))
	{
		/* The walk refused to move - an edge, or the mixer blocking a
		   vertical step. Nothing to repaint. */
		return;
	}

	const NAV_CTX tCtx        = navCtx();
	const BOOLEAN bWasOnMixer = Nav_IsOnMixer(&tCtx, tOld);
	const BOOLEAN bNowOnMixer = Nav_IsOnMixer(&tCtx, tNew);

	/* StompBoard::select already hides the other two switches, so sliding
	   along the stomp row does not need a deselect first - and skipping it
	   saves three invalidates. */
	if((NAV_ROW_STOMP != tOld.nRow) || (NAV_ROW_STOMP != tNew.nRow))
	{
		clearHighlight(tOld);
	}

	applyHighlight(tNew);

	/* MixModule::select and deselect both invalidate unconditionally, so only
	   touch it when the answer actually changed. */
	if(bNowOnMixer != bWasOnMixer)
	{
		if(FALSE != bNowOnMixer)
		{
			mixModule.select();
		}
		else
		{
			mixModule.deselect();
		}
	}

	/*
	 * ePrevSelect records the last thing the cursor was ON, not the last cell
	 * it occupied - so moving within a row must leave it alone, exactly as
	 * the old code did by only assigning it in the branches that changed
	 * eCurrentSelect. That is what makes "leave the chain at the input, come
	 * back, land on the row you left" work.
	 */
	const ModuleSelectType eNewSelect = selectForPos(tNew);

	if(eNewSelect != eCurrentSelect)
	{
		ePrevSelect    = eCurrentSelect;
		eCurrentSelect = eNewSelect;
	}
}


/***************************************************************************************************
* Input handlers
***************************************************************************************************/

void SystemView::encMenuUpdate(S8 nValue)
{
	if(TRUE == modalWindowDelete.isVisible())
	{
		/*
		 * The encoder used to keep walking the grid behind the delete
		 * confirmation: only btnUp and btnDown guarded against it. Now all
		 * three agree.
		 */
		do_nothing();
		return;
	}

	if(true == addModuleWindow.isVisible())
	{
		/* Tested by sign rather than against 1, so a zero cannot be taken for
		   a reverse turn. The encoder no longer emits one - see encoder.hpp -
		   but this is the place the old bug was visible. */
		if(nValue > 0)
		{
			addModuleWindow.selectDown();
		}
		else if(nValue < 0)
		{
			addModuleWindow.selectUp();
		}

		return;
	}

	const NAV_CTX tCtx = navCtx();
	const NAV_POS tOld = currentPos();

	moveCursor(tOld, Nav_Horizontal(&tCtx,
	                                tOld,
	                                nValue,
	                                rowIndexForSelect(ePrevSelect)));
}

void SystemView::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	// Button pressed
	if(1 == nValue)
	{
		if(TRUE == modalWindowDelete.isVisible())
		{
			switch(eCurrentSelect)
			{
			case SELECT_MONO_CHAIN_1:
			case SELECT_MONO_CHAIN_2:
			case SELECT_MONO_CHAIN_3:
			case SELECT_MONO_CHAIN_4:
				if(TRUE == bIsMixerAdded
						&& eMixerPosition == monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber())
				{
					addModuleWindow.unblockSelect(MODULE_MIX);
					mixModule.setVisible(false);
					mixModule.deselect();
					monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->select(
							monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());
					bIsMixerAdded = FALSE;

					presenter->saveMixerPosition(-1);
				}
				else
				{
					addModuleWindow.unblockSelect(monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleName());

					if(MODULE_REC == monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleName())
					{
						presenter->getRecorderInfo()->mono[eCurrentSelect - SELECT_MONO_CHAIN_1] = FALSE;
						PUBSUB_Publish(PUBSUB_TOPIC_REC, presenter->getRecorderInfo(), sizeof(RecorderInfo_t));
					}

					monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->deleteSelectedModule();

					presenter->saveMonoModulePosition(MODULE_NONE, eCurrentSelect - SELECT_MONO_CHAIN_1, monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());
				}

				presenter->clearFXChain((ChannelType) (eCurrentSelect - SELECT_MONO_CHAIN_1));
				break;

			case SELECT_STEREO_CHAIN_1:
			case SELECT_STEREO_CHAIN_2:
				if(TRUE == bIsMixerAdded
						&& eMixerPosition == stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber())
				{
					addModuleWindow.unblockSelect(MODULE_MIX);
					mixModule.setVisible(false);
					mixModule.deselect();
					stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->select(
							stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());
					bIsMixerAdded = FALSE;

					presenter->saveMixerPosition(-1);
				}
				else
				{
					addModuleWindow.unblockSelect(stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleName());

					if(MODULE_REC == stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleName())
					{
						if(SELECT_STEREO_CHAIN_1 == eCurrentSelect)
						{
							presenter->getRecorderInfo()->stereo1 = FALSE;
						}
						else if(SELECT_STEREO_CHAIN_2 == eCurrentSelect)
						{
							presenter->getRecorderInfo()->stereo2 = FALSE;
						}

						PUBSUB_Publish(PUBSUB_TOPIC_REC, presenter->getRecorderInfo(), sizeof(RecorderInfo_t));
					}

					stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->deleteSelectedModule();

					presenter->saveStereoModulePosition(MODULE_NONE, eCurrentSelect - SELECT_STEREO_CHAIN_1, stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());
				}

				presenter->clearFXChain((ChannelType) (eCurrentSelect - SELECT_MONO_CHAIN_1));
				break;

			default:
				break;
			}

			/* The grid changed - a module or the mixer was removed. Both
			   branches above have already written it to the model, so one
			   whole-configuration send covers every case. */
			presenter->pushConfig();

			modalWindowDelete.setVisible(FALSE);
			modalWindowDelete.invalidate();
		}
		else if(TRUE == addModuleWindow.isVisible())
		{
			if(MODULE_MIX == addModuleWindow.getAddModuleName())
			{
				switch(eCurrentSelect)
				{
				case SELECT_MONO_CHAIN_1:
				case SELECT_MONO_CHAIN_2:
				case SELECT_MONO_CHAIN_3:
				case SELECT_MONO_CHAIN_4:
					eMixerPosition = monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber();
					presenter->saveSelectedChannel(
							(ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule(
							(ChainModuleNumber) monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());
					break;
				case SELECT_STEREO_CHAIN_1:
				case SELECT_STEREO_CHAIN_2:
					eMixerPosition = stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber();
					presenter->saveSelectedChannel(
							(ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule(
							(ChainModuleNumber) stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());

					break;
				default:
					break;
				}
				presenter->saveMixerPosition(eMixerPosition);

				/* Mixer placed - the grid gained a whole column. */
				presenter->pushConfig();

				application().gotoMixerScreenNoTransition();
			}
			else
			{
				switch(eCurrentSelect)
				{
				case SELECT_MONO_CHAIN_1:
				case SELECT_MONO_CHAIN_2:
				case SELECT_MONO_CHAIN_3:
				case SELECT_MONO_CHAIN_4:

					monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->addModule(
							monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber(),
							addModuleWindow.getAddModuleName());
					presenter->saveMonoModulePosition(
							addModuleWindow.getAddModuleName(),
							eCurrentSelect - SELECT_MONO_CHAIN_1,
							monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());
					presenter->saveSelectedChannel(
							(ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule(
							(ChainModuleNumber) monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber());

					/* Module added to a mono chain. */
					presenter->pushConfig();

					switch(addModuleWindow.getAddModuleName())
					{
					case MODULE_FX:
						application().gotoFXChainScreenNoTransition();
						break;
					case MODULE_LOOP:
						application().gotoLooperScreenNoTransition();
						break;
					case MODULE_REC:

						presenter->getRecorderInfo()->mono[eCurrentSelect - SELECT_MONO_CHAIN_1] = TRUE;

						PUBSUB_Publish(PUBSUB_TOPIC_REC, presenter->getRecorderInfo(), sizeof(RecorderInfo_t));

						application().gotoRecorderScreenNoTransition();
						break;
					default:
						break;
					}
					break;
				case SELECT_STEREO_CHAIN_1:
				case SELECT_STEREO_CHAIN_2:
					stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->addModule(
							stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber(),
							addModuleWindow.getAddModuleName());
					presenter->saveStereoModulePosition(
							addModuleWindow.getAddModuleName(),
							eCurrentSelect - SELECT_STEREO_CHAIN_1,
							stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());
					presenter->saveSelectedChannel(
							(ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule(
							(ChainModuleNumber) stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber());

					/* Module added to a stereo chain. */
					presenter->pushConfig();

					switch(addModuleWindow.getAddModuleName())
					{
					case MODULE_FX:
						application().gotoFXChainScreenNoTransition();
						break;
					case MODULE_LOOP:
						application().gotoLooperScreenNoTransition();
						break;
					case MODULE_REC:

						if(SELECT_STEREO_CHAIN_1 == eCurrentSelect)
						{
							presenter->getRecorderInfo()->stereo1 = TRUE;
						}
						else if(SELECT_STEREO_CHAIN_2 == eCurrentSelect)
						{
							presenter->getRecorderInfo()->stereo2 = TRUE;
						}

						PUBSUB_Publish(PUBSUB_TOPIC_REC, presenter->getRecorderInfo(), sizeof(RecorderInfo_t));

						application().gotoRecorderScreenNoTransition();
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
			}
		}
		else
		{
			ModuleName eSelectedModuleName = MODULE_NONE;
			S8 eSelectedModuleNumber = -1;

			ModuleName aCurrentChainModules[4] = {};

			switch(eCurrentSelect)
			{
			case SELECT_MONO_CHAIN_1:
				eSelectedModuleName = monoChain1.getSelectedModuleName();
				eSelectedModuleNumber = monoChain1.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = monoChain1.getModuleName(i);
				}

				break;
			case SELECT_MONO_CHAIN_2:
				eSelectedModuleName = monoChain2.getSelectedModuleName();
				eSelectedModuleNumber = monoChain2.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = monoChain2.getModuleName(i);
				}

				break;
			case SELECT_MONO_CHAIN_3:
				eSelectedModuleName = monoChain3.getSelectedModuleName();
				eSelectedModuleNumber = monoChain3.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = monoChain3.getModuleName(i);
				}

				break;
			case SELECT_MONO_CHAIN_4:
				eSelectedModuleName = monoChain4.getSelectedModuleName();
				eSelectedModuleNumber = monoChain4.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = monoChain4.getModuleName(i);
				}

				break;
			case SELECT_STEREO_CHAIN_1:
				eSelectedModuleName = stereoChain1.getSelectedModuleName();
				eSelectedModuleNumber = stereoChain1.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = stereoChain1.getModuleName(i);
				}

				break;
			case SELECT_STEREO_CHAIN_2:
				eSelectedModuleName = stereoChain2.getSelectedModuleName();
				eSelectedModuleNumber = stereoChain2.getSelectedModuleNumber();

				for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
				{
					aCurrentChainModules[i] = stereoChain2.getModuleName(i);
				}

				break;
			case SELECT_INPUT:
				eSelectedModuleName = MODULE_INPUT;
				break;
			case SELECT_OUTPUT:
				eSelectedModuleName = MODULE_OUTPUT;
				break;
			case SELECT_STOMP_BOARD:
				eSelectedModuleName = MODULE_STOMP;
				eSelectedModuleNumber = stompBoard.getSelectedFootSwitch();
				break;
			default:
				break;
			}

			presenter->saveSelectedModule(eCurrentSelect);
			presenter->savePrevSelectedModule(ePrevSelect);

			if(TRUE == bIsMixerAdded
				&& eMixerPosition == eSelectedModuleNumber
				&& SELECT_STOMP_BOARD != eCurrentSelect)
			{
				presenter->saveSelectedChainModule((ChainModuleNumber) eSelectedModuleNumber);
				application().gotoMixerScreenNoTransition();
			}
			else
			{
				switch(eSelectedModuleName)
				{
				case MODULE_NONE:

					for(ChainModuleNumber i = CHAIN_MODULE_1; i <= CHAIN_MODULE_4; i = (ChainModuleNumber)((U8)i + 1))
					{
						if(MODULE_NONE != aCurrentChainModules[i])
						{
							addModuleWindow.blockSelect(aCurrentChainModules[i]);
						}
					}

					if(TRUE == bIsMixerAdded
						|| MODULE_NONE != monoChain1.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != monoChain2.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != monoChain3.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != monoChain4.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != stereoChain1.getModuleName((ChainModuleNumber) eSelectedModuleNumber)
						|| MODULE_NONE != stereoChain2.getModuleName((ChainModuleNumber) eSelectedModuleNumber))
					{
						addModuleWindow.blockSelect(MODULE_MIX);
					}

					addModuleWindow.setVisible(TRUE);
					addModuleWindow.selectFirst();
					addModuleWindow.invalidate();

					break;
				case MODULE_INPUT:
					application().gotoInputScreenNoTransition();
					break;
				case MODULE_OUTPUT:
					application().gotoOutputScreenNoTransition();
					break;
				case MODULE_FX:
					presenter->saveSelectedChannel((ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule((ChainModuleNumber) eSelectedModuleNumber);
					application().gotoFXChainScreenNoTransition();
					break;
				case MODULE_LOOP:
					presenter->saveSelectedChannel((ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule((ChainModuleNumber) eSelectedModuleNumber);
					application().gotoLooperScreenNoTransition();
					break;
				case MODULE_REC:
					presenter->saveSelectedChannel((ChannelType)(eCurrentSelect - SELECT_MONO_CHAIN_1));
					presenter->saveSelectedChainModule((ChainModuleNumber) eSelectedModuleNumber);
					application().gotoRecorderScreenNoTransition();
					break;
				case MODULE_STOMP:
					presenter->saveSelectedFootSwitch((FootSwitches) eSelectedModuleNumber);
					application().gotoStompScreenNoTransition();
					break;
				default:
					break;
				}
			}
		}
	}
	else if(2 == nValue)	// Button long press
	{

	}
}

void SystemView::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	// Button pressed
	if(1 == nValue)
	{
		if(TRUE == modalWindowDelete.isVisible())
		{
			modalWindowDelete.setVisible(FALSE);
			modalWindowDelete.invalidate();
		}
		else if(TRUE == addModuleWindow.isVisible())
		{
			addModuleWindow.unblockSelect(MODULE_FX);
			addModuleWindow.unblockSelect(MODULE_REC);
			addModuleWindow.unblockSelect(MODULE_LOOP);
			addModuleWindow.unblockSelect(MODULE_MIX);

			addModuleWindow.setVisible(FALSE);
			addModuleWindow.invalidate();
		}
		else
		{
			if(TRUE == bIsFuncPressed)
			{
				U8 modalText[20] = {0};
				switch(eCurrentSelect)
				{
				case SELECT_MONO_CHAIN_1:
				case SELECT_MONO_CHAIN_2:
				case SELECT_MONO_CHAIN_3:
				case SELECT_MONO_CHAIN_4:
					if(TRUE == bIsMixerAdded
							&& eMixerPosition == monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleNumber())
					{
						sprintf((char*)modalText, "Mixer");
						modalWindowDelete.setText(modalText);
						modalWindowDelete.setVisible(TRUE);
						modalWindowDelete.invalidate();
					}
					else
					{
						switch(monoChain[eCurrentSelect - SELECT_MONO_CHAIN_1]->getSelectedModuleName())
						{
						case MODULE_FX:
							sprintf((char*)modalText, "Mono %d %s", (eCurrentSelect - SELECT_MONO_CHAIN_1) + 1, "FX chain");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						case MODULE_LOOP:
							sprintf((char*)modalText, "Mono %d %s", (eCurrentSelect - SELECT_MONO_CHAIN_1) + 1, "Looper");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						case MODULE_REC:
							sprintf((char*)modalText, "Mono %d %s", (eCurrentSelect - SELECT_MONO_CHAIN_1) + 1, "Recorder");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						default:
							break;
						}
					}
					break;

				case SELECT_STEREO_CHAIN_1:
				case SELECT_STEREO_CHAIN_2:
					if(TRUE == bIsMixerAdded
							&& eMixerPosition == stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleNumber())
					{
						sprintf((char*)modalText, "Mixer");
						modalWindowDelete.setText(modalText);
						modalWindowDelete.setVisible(TRUE);
						modalWindowDelete.invalidate();
					}
					else
					{
						switch(stereoChain[eCurrentSelect - SELECT_STEREO_CHAIN_1]->getSelectedModuleName())
						{
						case MODULE_FX:
							sprintf((char*)modalText, "Stereo %d %s", (eCurrentSelect - SELECT_STEREO_CHAIN_1) + 1, "FX chain");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						case MODULE_LOOP:
							sprintf((char*)modalText, "Stereo %d %s", (eCurrentSelect - SELECT_STEREO_CHAIN_1) + 1, "Looper");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						case MODULE_REC:
							sprintf((char*)modalText, "Stereo %d %s", (eCurrentSelect - SELECT_STEREO_CHAIN_1) + 1, "Recorder");
							modalWindowDelete.setText(modalText);
							modalWindowDelete.setVisible(TRUE);
							modalWindowDelete.invalidate();
							break;
						default:
							break;
						}
					}
					break;

				default:
					break;
				}
			}
		}
	}
	else if(2 == nValue)	// Button long press
	{

	}
}

void SystemView::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	if(true == modalWindowDelete.isVisible())
	{
		do_nothing();
		return;
	}

	if(true == addModuleWindow.isVisible())
	{
		addModuleWindow.selectUp();
		return;
	}

	const NAV_CTX tCtx = navCtx();
	const NAV_POS tOld = currentPos();

	moveCursor(tOld, Nav_Vertical(&tCtx, tOld, -1));
}

void SystemView::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	if(true == modalWindowDelete.isVisible())
	{
		do_nothing();
		return;
	}

	if(true == addModuleWindow.isVisible())
	{
		addModuleWindow.selectDown();
		return;
	}

	const NAV_CTX tCtx = navCtx();
	const NAV_POS tOld = currentPos();

	moveCursor(tOld, Nav_Vertical(&tCtx, tOld, +1));
}
