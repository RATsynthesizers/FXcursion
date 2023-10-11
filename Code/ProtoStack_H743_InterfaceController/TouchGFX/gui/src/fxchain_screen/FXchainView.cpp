#include <gui/fxchain_screen/FXchainView.hpp>

FXchainView::FXchainView() {

}

void FXchainView::setupScreen() {

	channelNum = presenter->getChannelNum();

	for (int i = 0; i < MAX_EFFECTS_NUM; i++) {
		menuItemInfoArray[i] = presenter->getFXchainItem(channelNum, i);
		if(menuItemInfoArray[i].effectNameID != T_EMPTYEFFECT)
			currentEffectsNumber++;
		scrollMenu[i]->setEffect(menuItemInfoArray[i]);
		if (i > 0 && menuItemInfoArray[i - 1].effectNameID == T_EMPTYEFFECT)
			scrollMenu[i]->setVisible(false);
	}

	for (int i = 0; i < EFFECT_TYPES; i++) {
		effectInfoArray[i] = presenter->getEffectInfo(channelNum, i);
		effectsList.setInfo(i, effectInfoArray[i]);
	}

	currentPos = presenter->getChannelChainPosition(channelNum);
	scrollMenu[currentPos]->select(true);

	FXchainViewBase::setupScreen();

	const uint8_t io[10] = "";
	if (channelNum < 3) {
		strcpy((char*) io, "Input ");
		char buf[2];
		sprintf(buf, "%d", channelNum + 1);
		strcat((char*) io, buf);
	} else {
		strcpy((char*) io, "Output ");
		char buf[2];
		sprintf(buf, "%d", (channelNum - 3) + 1);
		strcat((char*) io, buf);
	}

	Unicode::fromUTF8(io, channelNumFX_textBuffer, CHANNELNUMFX_TEXT_SIZE);
}

void FXchainView::tearDownScreen() {
	FXchainViewBase::tearDownScreen();
}

void FXchainView::parameterChange(uint8_t id, int8_t scrollAmount) {

}

void FXchainView::encScroll_action(int8_t scrollAmount) {
	if (isSelectingEffect) {
		effectsList.scrollContents(scrollAmount);
	} else if (isInSubMenu) {
		subMenu.scrollContents(scrollAmount);
	} else {
		int8_t newPos = currentPos + scrollAmount;
		if (newPos >= 0 && newPos < MAX_EFFECTS_NUM
				&& scrollMenu[newPos]->isVisible()) {
			scrollMenu[currentPos]->select(false);
			currentPos = newPos;
			scrollMenu[currentPos]->select(true);
		}
	}
}

void FXchainView::btnYES_action(void) {
	if (isSelectingEffect) {

		EffectInfo tempEffectInfo = effectsList.getInfo(
				effectsList.getCurrentPos());

		if (tempEffectInfo.available == false)
			return;

		isSelectingEffect = false;
		effectsList.setVisible(false);
		effectsList.invalidate();

		menuItemInfoArray[currentPos].effectNameID =
				tempEffectInfo.effectNameID;
		menuItemInfoArray[currentPos].bitmapRegular =
				tempEffectInfo.bitmapRegular;
		menuItemInfoArray[currentPos].bitmapSelected =
				tempEffectInfo.bitmapSelected;

		uint8_t effectTypeID =
				(uint8_t) menuItemInfoArray[currentPos].effectNameID
						- (uint8_t) T_CHORUSEFFECT;

		effectInfoArray[effectTypeID].available = false;
		effectsList.setInfo(effectTypeID, effectInfoArray[effectTypeID]);

		scrollMenu[currentPos]->setEffect(menuItemInfoArray[currentPos]);

		if (currentPos < MAX_EFFECTS_NUM - 1) {
			scrollMenu[currentPos + 1]->setVisible(true);
			scrollMenu[currentPos + 1]->invalidate();
		}

		currentEffectsNumber++;

	} else if (isInSubMenu) {

		MenuItemInfo tempMenuItemInfo;

		switch (subMenu.getCurrentPos()) {
		case 0:

			presenter->saveChannelChainPosition(channelNum, currentPos);
			presenter->saveFXchain(channelNum, menuItemInfoArray);
			presenter->saveEffectInfo(channelNum, effectInfoArray);
			//TODO: change for COVER transition in TouchGFX Designer when switch to SDRAM
			application().gotoEffectSettingsScreenBlockTransition();

			break;
		case 1:
			//TODO: rewire in audio

			isInSubMenu = false;
			subMenu.setVisible(isInSubMenu);
			subMenu.invalidate();

			tempMenuItemInfo = menuItemInfoArray[currentPos - 1];
			menuItemInfoArray[currentPos - 1] = menuItemInfoArray[currentPos];
			menuItemInfoArray[currentPos] = tempMenuItemInfo;

			scrollMenu[currentPos]->setEffect(menuItemInfoArray[currentPos]);
			scrollMenu[currentPos - 1]->setEffect(
					menuItemInfoArray[currentPos - 1]);

			encScroll_action(-1);

			break;
		case 2:
			//TODO: rewire in audio

			isInSubMenu = false;
			subMenu.setVisible(isInSubMenu);
			subMenu.invalidate();

			tempMenuItemInfo = menuItemInfoArray[currentPos + 1];
			menuItemInfoArray[currentPos + 1] = menuItemInfoArray[currentPos];
			menuItemInfoArray[currentPos] = tempMenuItemInfo;

			scrollMenu[currentPos]->setEffect(menuItemInfoArray[currentPos]);
			scrollMenu[currentPos + 1]->setEffect(
					menuItemInfoArray[currentPos + 1]);

			encScroll_action(1);

			break;
		case 3:
			if (subMenu.getIsDeletingEffect()) {

				subMenu.closeDeletingEffect();

				uint8_t effectTypeID =
						(uint8_t) menuItemInfoArray[currentPos].effectNameID
								- (uint8_t) T_CHORUSEFFECT;

				effectInfoArray[effectTypeID].available = true;
				effectsList.setInfo(effectTypeID,
						effectInfoArray[effectTypeID]);

				isInSubMenu = false;
				subMenu.setVisible(isInSubMenu);
				subMenu.invalidate();

				for (int i = currentPos; i < currentEffectsNumber - 1; i++) {
					menuItemInfoArray[i] = menuItemInfoArray[i + 1];
					scrollMenu[i]->setEffect(menuItemInfoArray[i]);
				}
				menuItemInfoArray[currentEffectsNumber - 1].effectNameID =
						T_EMPTYEFFECT;
				menuItemInfoArray[currentEffectsNumber - 1].bitmapRegular =
						BITMAP_EMPTYPICT_ID;
				menuItemInfoArray[currentEffectsNumber - 1].bitmapSelected =
						BITMAP_EMPTYSELECTEDPICT_ID;
				scrollMenu[currentEffectsNumber - 1]->setEffect(
						menuItemInfoArray[currentEffectsNumber - 1]);

				if (currentEffectsNumber != MAX_EFFECTS_NUM)
					scrollMenu[currentEffectsNumber]->setVisible(false);

				currentEffectsNumber--;

			} else {
				subMenu.openDeletingEffect(
						menuItemInfoArray[currentPos].effectNameID);
			}
			break;
		default:
			break;
		}
	} else {
		if (menuItemInfoArray[currentPos].effectNameID == T_EMPTYEFFECT) {
			isSelectingEffect = true;
			effectsList.setVisible(true);
			effectsList.invalidate();
		} else {
			isInSubMenu = true;

			bool isFirst = (currentPos == 0);
			bool isLast = (currentPos == (currentEffectsNumber - 1));

			subMenu.resetSubMenu(isFirst, isLast);
			subMenu.setVisible(isInSubMenu);
			subMenu.invalidate();
		}
	}
}

void FXchainView::btnNO_action(void) {

	if (isSelectingEffect) {
		isSelectingEffect = false;
		effectsList.setVisible(false);
		effectsList.invalidate();
	} else if (subMenu.getIsDeletingEffect()) {
		subMenu.closeDeletingEffect();
	} else if (isInSubMenu) {
		isInSubMenu = false;
		subMenu.setVisible(isInSubMenu);
		subMenu.invalidate();
	} else {
		presenter->saveChannelChainPosition(channelNum, currentPos);
		presenter->saveFXchain(channelNum, menuItemInfoArray);
		presenter->saveEffectInfo(channelNum, effectInfoArray);
		application().gotoProjectSettingsScreenBlockTransition();
	}
}

