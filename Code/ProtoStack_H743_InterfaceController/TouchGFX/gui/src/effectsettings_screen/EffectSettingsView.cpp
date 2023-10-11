#include <gui/effectsettings_screen/EffectSettingsView.hpp>

EffectSettingsView::EffectSettingsView() {

}

IN_RAM_D1_DMA uint8_t audioParamsTXbuf[3];

void EffectSettingsView::setupScreen() {
	EffectSettingsViewBase::setupScreen();

	channelNum = presenter->getChannelNum();

	for (int i = 0; i < MAX_EFFECTS_NUM; i++) {
		menuItemInfoArray[i] = presenter->getFXchainItem(channelNum, i);
		effectPictograms[i]->setEffect(menuItemInfoArray[i]);
		if (menuItemInfoArray[i].effectNameID == T_EMPTYEFFECT)
			effectPictograms[i]->setVisible(false);
	}

	editingEffectNum = presenter->getChannelChainPosition(channelNum);
	selectedEffectNum = editingEffectNum;

	effectPictograms[selectedEffectNum]->select(true);
	effectPictograms[editingEffectNum]->edit(true);

	Unicode::snprintf(effectNameBuffer, EFFECTNAME_SIZE, "%s",
			touchgfx::TypedText(
					menuItemInfoArray[editingEffectNum].effectNameID).getText());

	const uint8_t io[10] = "";
	if (channelNum < 3) {
		strcpy((char*) io, "In ");
		char buf[2];
		sprintf(buf, "%d", channelNum + 1);
		strcat((char*) io, buf);
	} else {
		strcpy((char*) io, "Out ");
		char buf[2];
		sprintf(buf, "%d", (channelNum - 3) + 1);
		strcat((char*) io, buf);
	}

	Unicode::fromUTF8(io, chNum_textBuffer, CHNUM_TEXT_SIZE);

	//ОТПРАВКА:
	//1) Тип сообщения (Запрос параметров),Номер канала, Имя эффекта
//	audioParamsTXbuf[0] = REQUEST_PARAM;
//	audioParamsTXbuf[1] = channelNum;
//	audioParamsTXbuf[2] = (uint8_t) (menuItemInfoArray[editingEffectNum].effectNameID
//			- T_CHORUSEFFECT);

//	HAL_USART_Transmit_DMA(&husart2, audioParamsTXbuf, 3);

	//ПРИЕМ:
	//

	for (int i = 0; i < 4; i++) {

		//customGauges[i]->setValue(/*TODO: get values from UART*/);

		switch (menuItemInfoArray[editingEffectNum].effectNameID) {
		case T_REVERBEFFECT:
			customGauges[i]->setParamName((TEXTS) (T_DECAYREVERBPARAM + i));
			break;
		default:
			customGauges[i]->setParamName((TEXTS) (T_RESOURCEID1 + i));
			break;
		}
	}
}

void EffectSettingsView::tearDownScreen() {
	EffectSettingsViewBase::tearDownScreen();
}

void EffectSettingsView::parameterChange(uint8_t id, int8_t scrollAmount) {

	//TEST. DELETE UPON UART WORKING
	audioParamsTXbuf[0] = REQUEST_PARAM;
	audioParamsTXbuf[1] = channelNum;
	audioParamsTXbuf[2] = (uint8_t) (menuItemInfoArray[editingEffectNum].effectNameID
			- T_CHORUSEFFECT);
	HAL_UART_Transmit_DMA(&huart2, audioParamsTXbuf, 3);

	customGauges[id]->changeValue(scrollAmount);
	//TODO: send changed parameter to UART
	//(channelNum, effectName, effectIndexInAudioSystem, scrollAmount)
}

void EffectSettingsView::encScroll_action(int8_t scrollAmount) {
	int8_t newPos = selectedEffectNum + scrollAmount;
	if (newPos >= 0 && newPos < MAX_EFFECTS_NUM
			&& effectPictograms[newPos]->isVisible()) {
		effectPictograms[selectedEffectNum]->select(false);
		selectedEffectNum = newPos;
		effectPictograms[selectedEffectNum]->select(true);
	}
}

void EffectSettingsView::btnYES_action(void) {
	if (selectedEffectNum != editingEffectNum) {

		effectPictograms[editingEffectNum]->edit(false);
		editingEffectNum = selectedEffectNum;
		effectPictograms[editingEffectNum]->edit(true);

		Unicode::snprintf(effectNameBuffer, EFFECTNAME_SIZE, "%s",
				touchgfx::TypedText(
						menuItemInfoArray[editingEffectNum].effectNameID).getText());

		for (int i = 0; i < 4; i++) {

			//customGauges[i]->setValue(/*TODO: get values from UART*/);

			switch (menuItemInfoArray[editingEffectNum].effectNameID) {
			case T_REVERBEFFECT:
				customGauges[i]->setParamName((TEXTS) (T_DECAYREVERBPARAM + i));
				break;
			default:
				customGauges[i]->setParamName((TEXTS) (T_RESOURCEID1 + i));
				break;
			}
		}

		invalidate();
	}
}

void EffectSettingsView::btnNO_action(void) {
	//TODO: change for COVER transition in TouchGFX Designer when switch to SDRAM
	application().gotoFXchainScreenBlockTransition();
}
