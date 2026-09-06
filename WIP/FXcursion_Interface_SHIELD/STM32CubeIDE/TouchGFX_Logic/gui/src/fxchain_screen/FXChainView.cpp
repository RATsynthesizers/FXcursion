#include <gui/fxchain_screen/FXChainView.hpp>
#include <string.h>
#include <stdio.h>

FXChainView::FXChainView()
{

}

void FXChainView::setupScreen()
{
	eChannelType = presenter->getSelectedChannel();

	for (int i = 0; i < MAX_EFFECTS_NUM; i++)
	{
		menuItemInfo[i] = presenter->getFXChainItem(eChannelType, i);
		if(menuItemInfo[i].eEffectNameID != T_EMPTYEFFECT)
		{
			nCurrentEffectNumber++;
		}

		scrollMenu[i]->setEffect(menuItemInfo[i]);
		if (i > 0 && menuItemInfo[i - 1].eEffectNameID == T_EMPTYEFFECT)
		{
			scrollMenu[i]->setVisible(FALSE);
		}
	}

	effectsList.beginInit();
	for (int i = 0; i < EFFECT_TYPES; i++)
	{
		effectPool[i] = presenter->getEffectInfo(eChannelType, i);
		effectsList.setInfo(i, effectPool[i]);
	}
	effectsList.endInit();

	nCurrentPos = presenter->getChannelChainPosition(eChannelType);
	scrollMenu[nCurrentPos]->select(TRUE);

	/* Not const, and bounded - see the note in StatusBar.cpp: writing through
	   a cast-away const on a const local array is UB and only happens to work
	   at the current optimisation level. */
	U8 channelName[CHANNELNUMTEXT_SIZE] = {0};
	if (eChannelType <= CHANNEL_MONO_4)
	{
		snprintf((char*) channelName, sizeof(channelName),
				"MONO %u", (unsigned)(((U8) eChannelType + 1U) & 0x0FU));
	}
	else
	{
		/*
		 * The mask is a no-op on real data - ChannelType has six values, so
		 * this is 1 or 2 - but it makes the range provable to the compiler.
		 * Without it GCC can only assume 0..255, which is 11 bytes into the
		 * Designer-sized 10-byte buffer, and warns. Two digits fit exactly.
		 */
		snprintf((char*) channelName, sizeof(channelName),
				"STEREO %u", (unsigned)((eChannelType - CHANNEL_MONO_4) & 0x0FU));
	}

	Unicode::fromUTF8(channelName, channelNumTextBuffer, CHANNELNUMTEXT_SIZE);

    FXChainViewBase::setupScreen();
}

void FXChainView::tearDownScreen()
{
    FXChainViewBase::tearDownScreen();
}

void FXChainView::setBypass(U8 nPos, BOOLEAN bBypass)
{
	menuItemInfo[nPos].bBypassed = bBypass;

	/* setEffect repaints from the struct, so the grey follows the data. */
	scrollMenu[nPos]->setEffect(menuItemInfo[nPos]);

	presenter->saveFXChain(eChannelType, menuItemInfo);

	/* aFxEnabled changed, so the audio side needs the new configuration. */
	presenter->pushConfig();
}

void FXChainView::swapEffects(U8 nFrom, U8 nTo)
{
	const FXChainItemInfo tTemp = menuItemInfo[nTo];

	menuItemInfo[nTo]   = menuItemInfo[nFrom];
	menuItemInfo[nFrom] = tTemp;

	scrollMenu[nFrom]->setEffect(menuItemInfo[nFrom]);
	scrollMenu[nTo]->setEffect(menuItemInfo[nTo]);

	/*
	 * Move the parameter block with it, now rather than at commit. A delta of
	 * +-1 through moveFXParams is exactly a swap of neighbours, so the stored
	 * order and the stored parameters never disagree - not even for the
	 * duration of one encoder step.
	 */
	presenter->moveFXParams(eChannelType, nFrom, (S8)((S8)nTo - (S8)nFrom));
	presenter->saveFXChain(eChannelType, menuItemInfo);

	/*
	 * Tell the audio side on every step, so the reorder is audible as it
	 * happens. Deferred rather than immediate: Model::tick can drain several
	 * encoder steps in one frame, and that many configuration frames back to
	 * back would overrun the transmit ring - at which point FxLink_Send drops
	 * silently and the audio side is left stale. See Model::pushConfigDeferred.
	 */
	presenter->pushConfigDeferred();
}

void FXChainView::encMenuUpdate(S8 nValue)
{
	if (bIsSelectingEffect)
	{
		effectsList.scrollContents(nValue);
	}
	else
	{
		S8 newPos = nCurrentPos + nValue;
		if (newPos >= 0
			&& newPos < MAX_EFFECTS_NUM
			&& scrollMenu[newPos]->isVisible())
		{
			if(TRUE == bIsMoving)
			{
				swapEffects(nCurrentPos, (U8)newPos);
			}

			scrollMenu[nCurrentPos]->select(FALSE);
			nCurrentPos = newPos;
			scrollMenu[nCurrentPos]->select(TRUE);
		}
	}
}

void FXChainView::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	if(nValue == 1)
	{
		if (TRUE == bIsSelectingEffect)
		{
			EffectInfo tempEffectInfo = effectsList.getInfo(effectsList.getCurrentPos());

			menuItemInfo[nCurrentPos].eEffectNameID =
					tempEffectInfo.eEffectNameID;
			menuItemInfo[nCurrentPos].nBitmapRegular =
					tempEffectInfo.nBitmapRegular;
			menuItemInfo[nCurrentPos].nBitmapSelected =
					tempEffectInfo.nBitmapSelected;
			menuItemInfo[nCurrentPos].nEffectsAmount =
					tempEffectInfo.nEffectsAmount;

			/* A freshly added effect is on. Without this it would inherit the
			   bypass flag left behind by whatever occupied the slot before. */
			menuItemInfo[nCurrentPos].bBypassed = FALSE;

			effectPool[effectsList.getCurrentPos()].bAvailable = FALSE;

			presenter->saveChannelChainPosition(eChannelType, nCurrentPos);
			presenter->saveFXChain(eChannelType, menuItemInfo);
			presenter->saveEffectInfo(eChannelType, effectPool);

			/* Effect added - the audio graph changed. */
			presenter->pushConfig();

			application().gotoEffectSettingsScreenNoTransition();

		}
		else if (TRUE == modalWindowDelete.isVisible())
		{
			U8 effectTypeID =
					(U8) menuItemInfo[nCurrentPos].eEffectNameID
							- (U8) T_CHORUSEFFECT;

			effectPool[effectTypeID].bAvailable = TRUE;
			effectsList.setInfo(effectTypeID,
								effectPool[effectTypeID]);

			for (int i = nCurrentPos; i < nCurrentEffectNumber - 1; i++)
			{
				menuItemInfo[i] = menuItemInfo[i + 1];
				scrollMenu[i]->setEffect(menuItemInfo[i]);
			}

			menuItemInfo[nCurrentEffectNumber - 1].eEffectNameID =
					T_EMPTYEFFECT;
			menuItemInfo[nCurrentEffectNumber - 1].nBitmapRegular =
					BITMAP_EMPTYPICT_ID;
			menuItemInfo[nCurrentEffectNumber - 1].nBitmapSelected =
					BITMAP_EMPTYSELECTEDPICT_ID;
			menuItemInfo[nCurrentEffectNumber - 1].nEffectsAmount =
					0;
			/* Leave the vacated slot clean, so the next effect added here does
			   not arrive already bypassed. */
			menuItemInfo[nCurrentEffectNumber - 1].bBypassed = FALSE;
			scrollMenu[nCurrentEffectNumber - 1]->setEffect(
					menuItemInfo[nCurrentEffectNumber - 1]);

			if (nCurrentEffectNumber != MAX_EFFECTS_NUM)
			{
				scrollMenu[nCurrentEffectNumber]->setVisible(FALSE);
			}

			presenter->saveEffectInfo(eChannelType, effectPool);
			presenter->saveFXChain(eChannelType, menuItemInfo);

			/*
			 * Effect deleted. This is what the old "send delete command to
			 * audio" TODO wanted, and the answer is that there is no delete
			 * command by design - the whole configuration goes and the audio
			 * side rebuilds. That also covers the shift-down of every effect
			 * after this one, which a per-effect message would have had to
			 * describe separately.
			 */
			presenter->pushConfig();

			nCurrentEffectNumber--;

			modalWindowDelete.setVisible(FALSE);
			modalWindowDelete.invalidate();
		}
		else if (TRUE == bIsMoving)
		{
			bIsMoving = FALSE;

			statusBar.stopMove();

			if(nCurrentEffectNumber < MAX_EFFECTS_NUM)
			{
				scrollMenu[nCurrentEffectNumber]->setVisible(TRUE);
			}

			/*
			 * Nothing to send here: every step already went out as it
			 * happened, so the audio side is holding the final order already.
			 */

			list.invalidate();
		}
		else if (TRUE == bIsFuncPressed
				 && T_EMPTYEFFECT != menuItemInfo[nCurrentPos].eEffectNameID
				 && FALSE != menuItemInfo[nCurrentPos].bBypassed)
		{
			/* FUNC+YES on a bypassed effect turns it back on. Only then can it
			   be moved, because FUNC+YES is the move gesture too. */
			setBypass(nCurrentPos, FALSE);
		}
		else if (TRUE == bIsFuncPressed
				 && T_EMPTYEFFECT != menuItemInfo[nCurrentPos].eEffectNameID)
		{
			bIsMoving = TRUE;

			statusBar.startMove();

			nPosBeforeMoving = nCurrentPos;

			/* No snapshot taken - cancel walks the move back instead. */

			if(nCurrentEffectNumber < MAX_EFFECTS_NUM)
			{
				scrollMenu[nCurrentEffectNumber]->setVisible(FALSE);
			}

			list.invalidate();
		}
		else
		{
			if (menuItemInfo[nCurrentPos].eEffectNameID == T_EMPTYEFFECT)
			{
				bIsSelectingEffect = TRUE;
				effectsList.setVisible(TRUE);
				effectsList.invalidate();
			}
			else
			{
				presenter->saveChannelChainPosition(eChannelType, nCurrentPos);
				application().gotoEffectSettingsScreenNoTransition();
			}
		}
	}
}

void FXChainView::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	if(nValue == 1)
	{
		if (bIsSelectingEffect)
		{
			bIsSelectingEffect = FALSE;
			effectsList.setVisible(FALSE);
			effectsList.invalidate();
		}
		else if (TRUE == modalWindowDelete.isVisible())
		{
			modalWindowDelete.setVisible(FALSE);
			modalWindowDelete.invalidate();
		}
		else if (TRUE == bIsMoving)
		{
			bIsMoving = FALSE;
			statusBar.stopMove();

			/*
			 * Walk it back one neighbour at a time with the same swap the
			 * forward move used, so the inverse cannot drift from it. Any
			 * parameter edited during the move stays as the user left it -
			 * only the position is undone.
			 *
			 * The highlight comes off first: swapEffects moves the contents,
			 * not the selection, so leaving it on would strand it where the
			 * effect used to be.
			 */
			scrollMenu[nCurrentPos]->select(FALSE);

			while(nCurrentPos != nPosBeforeMoving)
			{
				const U8 nNext = (nCurrentPos < nPosBeforeMoving)
				                 ? (U8)(nCurrentPos + 1U)
				                 : (U8)(nCurrentPos - 1U);

				swapEffects(nCurrentPos, nNext);
				nCurrentPos = nNext;
			}

			scrollMenu[nCurrentPos]->select(TRUE);

			if(nCurrentEffectNumber < MAX_EFFECTS_NUM)
			{
				scrollMenu[nCurrentEffectNumber]->setVisible(TRUE);
			}

			/* The walk back already queued a send per step, but ask once more
			   so the restored order is guaranteed to go out even if the move
			   never actually left its starting slot. */
			presenter->pushConfigDeferred();

			list.invalidate();
		}
		else if (TRUE == bIsFuncPressed)
		{
			if(nCurrentPos < nCurrentEffectNumber)
			{
				/*
				 * Two steps to delete, and the first one is reversible: FUNC+NO
				 * on a live effect bypasses it, FUNC+NO again asks whether to
				 * remove it. FUNC+YES turns it back on.
				 *
				 * WORTH KNOWING: this changes what a single FUNC+NO does. It
				 * used to open the delete confirmation directly.
				 */
				if(FALSE == menuItemInfo[nCurrentPos].bBypassed)
				{
					setBypass(nCurrentPos, TRUE);
				}
				else
				{
					modalWindowDelete.setTextUnicode(menuItemInfo[nCurrentPos].eEffectNameID);
					modalWindowDelete.setVisible(TRUE);
					modalWindowDelete.invalidate();
				}
			}
		}
		else
		{
			presenter->saveChannelChainPosition(eChannelType, nCurrentPos);
			application().gotoSystemScreenNoTransition();
		}
	}
}

void FXChainView::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	if(TRUE == bIsFuncPressed)
	{
		//TODO: Cancel last action
		do_nothing();
	}
	else
	{
		encMenuUpdate(-1);
	}
}

void FXChainView::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	if(TRUE == bIsFuncPressed)
	{
		//TODO: Cancel last action
		do_nothing();
	}
	else
	{
		encMenuUpdate(1);
	}
}
