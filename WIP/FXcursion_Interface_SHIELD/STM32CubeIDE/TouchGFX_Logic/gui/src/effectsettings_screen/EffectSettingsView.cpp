#include <gui/effectsettings_screen/EffectSettingsView.hpp>

/*
 * THERE IS ONE SELECTION ON THIS SCREEN, NOT TWO.
 *
 * There used to be a cursor (selectedEffectNum) that the menu encoder moved,
 * and a separately committed "editing" effect that YES set from it. That made
 * every FUNC gesture ambiguous - bypass could only sensibly act on the effect
 * whose name was on screen, while move could only sensibly act on the cursor,
 * and the two were not always the same effect.
 *
 * Now the menu encoder changes the edited effect directly. editingEffectNum is
 * the only position there is, so bypass, move and the parameter page all
 * unambiguously refer to the same thing.
 */

EffectSettingsView::EffectSettingsView()
{
	/* Whatever the Designer configured. Captured rather than hardcoded so
	   restyling the screen does not leave this file disagreeing with it. */
	nameBoxActiveColor      = nameBox.getColor();
	effectNameActiveColor   = effectName.getColor();

	/* Bypassed: the plate sinks into the screen background and the title
	   inverts, so the effect reads as "not part of the sound" without the
	   pictograms having to change - the colour scheme up there still needs
	   thinking about. */
	nameBoxBypassedColor    = background.getColor();
	effectNameBypassedColor = touchgfx::Color::getColorFromRGB(255, 255, 255);
}

void EffectSettingsView::setupScreen()
{
	eChannelType     = presenter->getSelectedChannel();
	editingEffectNum = presenter->getChannelChainPosition(eChannelType);

	if (editingEffectNum >= (U8)MAX_EFFECTS_NUM)
	{
		editingEffectNum = 0U;
	}

	for (U8 i = 0U; i < (U8)MAX_EFFECTS_NUM; i++)
	{
		fxChainItemInfoArray[i] = presenter->getFXChainItem(eChannelType, i);
		effectPictograms[i]->setEffect(fxChainItemInfoArray[i]);

		if (T_EMPTYEFFECT == fxChainItemInfoArray[i].eEffectNameID)
		{
			effectPictograms[i]->setVisible(false);
		}
	}

	/* Not const, and bounded - see the note in StatusBar.cpp. The masks make
	   the 1..2 / 1..4 range provable so the 10-byte buffer is provably
	   enough; see the fuller note in FXChainView.cpp. */
	U8 channelName[CHANNELNAME_SIZE] = {0};
	if (eChannelType <= CHANNEL_MONO_4)
	{
		snprintf((char*) channelName, sizeof(channelName),
				"MONO %u", (unsigned)(((U8) eChannelType + 1U) & 0x0FU));
	}
	else
	{
		snprintf((char*) channelName, sizeof(channelName),
				"STEREO %u", (unsigned)((eChannelType - CHANNEL_MONO_4) & 0x0FU));
	}

	Unicode::fromUTF8(channelName, channelNameBuffer, CHANNELNAME_SIZE);

	refreshEditedEffect();

    EffectSettingsViewBase::setupScreen();
}

void EffectSettingsView::tearDownScreen()
{
    EffectSettingsViewBase::tearDownScreen();
}


/***************************************************************************************************
* Painting what the edited effect currently is
***************************************************************************************************/

void EffectSettingsView::refreshEditedEffect()
{
	Unicode::snprintf(effectNameBuffer, EFFECTNAME_SIZE, "%s",
			touchgfx::TypedText(
					fxChainItemInfoArray[editingEffectNum].eEffectNameID).getText());
	effectName.invalidate();

	refreshBypassVisual();
	refreshPictogramStates();

	/*
	 * Page count comes from the shared descriptor now, not from a hardcoded
	 * MAX_PARAMETERS. Pages::setAmount already shows one page at four or
	 * fewer, so an effect with four parameters simply stops offering a second
	 * page - the behaviour that used to be on the "future work" list turns on
	 * by feeding it the real number.
	 */
	const FX_DESC* pDesc = presenter->getFxDesc(eChannelType, editingEffectNum);

	pages.setAmount((0 != pDesc) ? pDesc->nParamQty : 0U);

	refreshParamPage();
}

void EffectSettingsView::refreshBypassVisual()
{
	const BOOLEAN bBypassed = fxChainItemInfoArray[editingEffectNum].bBypassed;

	nameBox.setColor((FALSE != bBypassed) ? nameBoxBypassedColor
	                                      : nameBoxActiveColor);
	effectName.setColor((FALSE != bBypassed) ? effectNameBypassedColor
	                                         : effectNameActiveColor);

	nameBox.invalidate();
	effectName.invalidate();
}

void EffectSettingsView::refreshPictogramStates()
{
	for (U8 i = 0U; i < (U8)MAX_EFFECTS_NUM; i++)
	{
		const bool bHere = (i == editingEffectNum);

		effectPictograms[i]->setState(bHere,
		                              bHere,
		                              bHere && (FALSE != bIsMoving));
	}
}

void EffectSettingsView::refreshParamPage()
{
	const FX_DESC* pDesc      = presenter->getFxDesc(eChannelType, editingEffectNum);
	const U8       nParamQty  = (0 != pDesc) ? pDesc->nParamQty : 0U;
	const U8       nFirstParam = (U8)(pages.getPage() * PARAMS_PER_PAGE);

	for (U8 i = 0U; i < PARAMS_PER_PAGE; i++)
	{
		const U8 nParamIndex = (U8)(nFirstParam + i);

		/*
		 * Parameters that do not exist are not drawn at all, rather than shown
		 * as knobs that do nothing. Effects have between two and eight
		 * parameters today, so most pages are partly empty - a dead knob is
		 * worse than a missing one, because the user cannot tell it is dead
		 * without turning it.
		 */
		if ((nParamIndex >= nParamQty) || (nParamIndex >= (U8)MAX_PARAMETERS))
		{
			customGauges[i]->setVisible(false);
			customGauges[i]->invalidate();
			continue;
		}

		customGauges[i]->setVisible(true);
		customGauges[i]->setValue(
				presenter->getFXParam(eChannelType, editingEffectNum, nParamIndex));
		customGauges[i]->setParamName(pDesc->pParam[nParamIndex].pName);
	}
}


/***************************************************************************************************
* Reordering and bypass
***************************************************************************************************/

BOOLEAN EffectSettingsView::canMoveSelected()
{
	if (T_EMPTYEFFECT == fxChainItemInfoArray[editingEffectNum].eEffectNameID)
	{
		return FALSE;
	}

	/* FUNC+YES on a bypassed effect means "turn it back on", so it cannot also
	   mean "move". Park it until the user re-enables it. */
	if (FALSE != fxChainItemInfoArray[editingEffectNum].bBypassed)
	{
		return FALSE;
	}

	/* Nothing to swap with. */
	U8 nVisible = 0U;

	for (U8 i = 0U; i < (U8)MAX_EFFECTS_NUM; i++)
	{
		if (effectPictograms[i]->isVisible())
		{
			nVisible++;
		}
	}

	return (nVisible > 1U) ? TRUE : FALSE;
}

void EffectSettingsView::swapEffects(U8 nFrom, U8 nTo)
{
	const FXChainItemInfo tTemp = fxChainItemInfoArray[nTo];

	fxChainItemInfoArray[nTo]   = fxChainItemInfoArray[nFrom];
	fxChainItemInfoArray[nFrom] = tTemp;

	effectPictograms[nFrom]->setEffect(fxChainItemInfoArray[nFrom]);
	effectPictograms[nTo]->setEffect(fxChainItemInfoArray[nTo]);

	/*
	 * Move the parameter block in the same breath. moveFXParams with a delta
	 * of +-1 is exactly a swap of neighbours, so after this call the stored
	 * order and the stored parameters agree again - which is why a knob can be
	 * turned in the middle of a move without corrupting anything.
	 */
	presenter->moveFXParams(eChannelType, nFrom, (S8)((S8)nTo - (S8)nFrom));
	presenter->saveFXChain(eChannelType, fxChainItemInfoArray);

	/*
	 * Tell the audio side on every step, so the reorder is audible as it
	 * happens. Deferred rather than immediate: several encoder steps can be
	 * drained in one frame, and that many configuration frames back to back
	 * would overrun the transmit ring. See Model::pushConfigDeferred.
	 */
	presenter->pushConfigDeferred();
}

void EffectSettingsView::setBypass(U8 nPos, BOOLEAN bBypass)
{
	fxChainItemInfoArray[nPos].bBypassed = bBypass;

	presenter->saveFXChain(eChannelType, fxChainItemInfoArray);

	refreshBypassVisual();

	/* aFxEnabled changed. */
	presenter->pushConfig();
}


/***************************************************************************************************
* Input handlers
***************************************************************************************************/

void EffectSettingsView::encMenuUpdate(S8 nValue)
{
	const S8 newPos = (S8)editingEffectNum + nValue;

	if ((newPos < 0) || (newPos >= (S8)MAX_EFFECTS_NUM)
		|| (false == effectPictograms[newPos]->isVisible()))
	{
		return;
	}

	if (FALSE != bIsMoving)
	{
		swapEffects(editingEffectNum, (U8)newPos);

		/* The same effect, now one slot along - its parameters travelled with
		   it, so only the pictogram row needs repainting. */
		editingEffectNum = (U8)newPos;
		refreshPictogramStates();

		return;
	}

	editingEffectNum = (U8)newPos;

	presenter->saveChannelChainPosition(eChannelType, editingEffectNum);

	/* A different effect: name, bypass state, page count and parameters all
	   change together. */
	refreshEditedEffect();
}

void EffectSettingsView::encParamUpdate(U8 nID, S8 nValue)
{
	if (nID >= PARAMS_PER_PAGE)
	{
		return;
	}

	const U8       nParamIndex = (U8)(nID + pages.getPage() * PARAMS_PER_PAGE);
	const FX_DESC* pDesc       = presenter->getFxDesc(eChannelType, editingEffectNum);

	/*
	 * An encoder under a gauge that is not drawn does nothing. Without this
	 * the hidden knobs would still write parameters the effect does not have.
	 */
	if ((0 == pDesc)
		|| (nParamIndex >= pDesc->nParamQty)
		|| (nParamIndex >= (U8)MAX_PARAMETERS))
	{
		return;
	}

	customGauges[nID]->changeValue(nValue);

	const U8 nNewValue = customGauges[nID]->getValue();

	presenter->saveFXParam(eChannelType,
	                       editingEffectNum,
	                       nParamIndex,
	                       nNewValue);

	/*
	 * Straight out to the audio controller, on every detent. One 8-byte
	 * payload per step, addressed by effect TYPE rather than by slot - so it
	 * stays correct even in the middle of a reorder.
	 */
	presenter->pushParam(eChannelType,
	                     editingEffectNum,
	                     nParamIndex,
	                     nNewValue);
}

void EffectSettingsView::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	if (1 != nValue)
	{
		return;
	}

	if (FALSE != bIsMoving)
	{
		/*
		 * Commit. Nothing to send: every step already went out as it happened,
		 * so the audio side is holding the final order already.
		 */
		bIsMoving = FALSE;
		statusBar.stopMove();
		refreshPictogramStates();

		presenter->saveChannelChainPosition(eChannelType, editingEffectNum);

		return;
	}

	if (TRUE == bIsFuncPressed)
	{
		if ((T_EMPTYEFFECT != fxChainItemInfoArray[editingEffectNum].eEffectNameID)
			&& (FALSE != fxChainItemInfoArray[editingEffectNum].bBypassed))
		{
			/* Turn it back on. Checked before the move branch, because
			   FUNC+YES carries both meanings and un-bypass wins here. */
			setBypass(editingEffectNum, FALSE);
		}
		else if (FALSE != canMoveSelected())
		{
			bIsMoving        = TRUE;
			nPosBeforeMoving = editingEffectNum;

			statusBar.startMove();
			refreshPictogramStates();
		}
		else
		{
			/* Empty slot, or nothing to move it past. */
		}
	}
}

void EffectSettingsView::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	/*
	 * The guard is new. Without it this fired on press AND release AND long
	 * press, so a single tap ran the screen change up to three times - which
	 * survived only because the later events landed on the screen that had
	 * already replaced this one.
	 */
	if (1 != nValue)
	{
		return;
	}

	if (FALSE != bIsMoving)
	{
		/*
		 * Cancel: walk it back one neighbour at a time with the same routine
		 * that moved it forward, so the inverse cannot disagree with the
		 * forward path. Anything the user changed on a gauge along the way
		 * stays changed - only the position is restored.
		 */
		while (editingEffectNum != nPosBeforeMoving)
		{
			const U8 nNext = (editingEffectNum < nPosBeforeMoving)
			                 ? (U8)(editingEffectNum + 1U)
			                 : (U8)(editingEffectNum - 1U);

			swapEffects(editingEffectNum, nNext);
			editingEffectNum = nNext;
		}

		bIsMoving = FALSE;
		statusBar.stopMove();
		refreshPictogramStates();

		presenter->saveChannelChainPosition(eChannelType, editingEffectNum);

		/* The walk back already queued a send per step, but ask once more so
		   the restored order is guaranteed to go out even if the move never
		   actually left its starting slot. */
		presenter->pushConfigDeferred();

		return;
	}

	if (TRUE == bIsFuncPressed)
	{
		/*
		 * FUNC+NO turns the edited effect off. There is no delete from this
		 * screen, so pressing it again on an already-bypassed effect does
		 * nothing - FUNC+YES is how you turn it back on.
		 */
		if ((T_EMPTYEFFECT != fxChainItemInfoArray[editingEffectNum].eEffectNameID)
			&& (FALSE == fxChainItemInfoArray[editingEffectNum].bBypassed))
		{
			setBypass(editingEffectNum, TRUE);
		}

		return;
	}

	application().gotoFXChainScreenNoTransition();
}

void EffectSettingsView::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	if(TRUE == pages.pageUp())
	{
		refreshParamPage();
	}
}

void EffectSettingsView::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	if(TRUE == pages.pageDown())
	{
		refreshParamPage();
	}
}
