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

	const U8 channelName[CHANNELNUMTEXT_SIZE] = {0};
	if (eChannelType <= CHANNEL_MONO_4)
	{
		sprintf((char*) channelName, "MONO %d", (U8) eChannelType + 1);
	}
	else
	{
		sprintf((char*) channelName, "STEREO %d", (U8)(eChannelType - CHANNEL_MONO_4));
	}

	Unicode::fromUTF8(channelName, channelNumTextBuffer, CHANNELNUMTEXT_SIZE);

    FXChainViewBase::setupScreen();
}

void FXChainView::tearDownScreen()
{
    FXChainViewBase::tearDownScreen();
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
				FXChainItemInfo tempMenuItemInfo = menuItemInfo[nCurrentPos + nValue];
				menuItemInfo[nCurrentPos + nValue] = menuItemInfo[nCurrentPos];
				menuItemInfo[nCurrentPos] = tempMenuItemInfo;

				scrollMenu[nCurrentPos]->setEffect(menuItemInfo[nCurrentPos]);
				scrollMenu[nCurrentPos + nValue]->setEffect(
						menuItemInfo[nCurrentPos + nValue]);
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

			effectPool[effectsList.getCurrentPos()].bAvailable = FALSE;

			presenter->saveChannelChainPosition(eChannelType, nCurrentPos);
			presenter->saveFXChain(eChannelType, menuItemInfo);
			presenter->saveEffectInfo(eChannelType, effectPool);

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
			scrollMenu[nCurrentEffectNumber - 1]->setEffect(
					menuItemInfo[nCurrentEffectNumber - 1]);

			if (nCurrentEffectNumber != MAX_EFFECTS_NUM)
			{
				scrollMenu[nCurrentEffectNumber]->setVisible(FALSE);
			}

			presenter->saveEffectInfo(eChannelType, effectPool);
			presenter->saveFXChain(eChannelType, menuItemInfo);

			//TODO: send delete command to audio

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

			presenter->moveFXParams(eChannelType, nPosBeforeMoving, nCurrentPos - nPosBeforeMoving);
			presenter->saveFXChain(eChannelType, menuItemInfo);

			list.invalidate();
		}
		else if (TRUE == bIsFuncPressed
				 && T_EMPTYEFFECT != menuItemInfo[nCurrentPos].eEffectNameID)
		{
			bIsMoving = TRUE;

			statusBar.startMove();

			nPosBeforeMoving = nCurrentPos;

			for(int i = 0; i < nCurrentEffectNumber; i++)
			{
				menuItemsBeforeMoving[i] = menuItemInfo[i];
			}

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
			for(int i = 0; i < nCurrentEffectNumber; i++)
			{
				menuItemInfo[i] = menuItemsBeforeMoving[i];
				scrollMenu[i]->setEffect(menuItemInfo[i]);
			}

			scrollMenu[nCurrentPos]->select(FALSE);
			nCurrentPos = nPosBeforeMoving;
			scrollMenu[nCurrentPos]->select(TRUE);

			if(nCurrentEffectNumber < MAX_EFFECTS_NUM)
			{
				scrollMenu[nCurrentEffectNumber]->setVisible(TRUE);
			}

			list.invalidate();
		}
		else if (TRUE == bIsFuncPressed)
		{
			if(nCurrentPos < nCurrentEffectNumber)
			{
				modalWindowDelete.setTextUnicode(menuItemInfo[nCurrentPos].eEffectNameID);
				modalWindowDelete.setVisible(TRUE);
				modalWindowDelete.invalidate();
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
