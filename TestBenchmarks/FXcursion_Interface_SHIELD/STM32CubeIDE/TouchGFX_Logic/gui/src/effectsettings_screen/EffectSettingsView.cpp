#include <gui/effectsettings_screen/EffectSettingsView.hpp>

EffectSettingsView::EffectSettingsView()
{

}

void EffectSettingsView::setupScreen()
{
	eChannelType = presenter->getSelectedChannel();
	editingEffectNum = presenter->getChannelChainPosition(eChannelType);
	selectedEffectNum = editingEffectNum;

	for (U8 i = 0; i < MAX_EFFECTS_NUM; i++)
	{
		fxChainItemInfoArray[i] = presenter->getFXChainItem(eChannelType, i);
		effectPictograms[i]->setEffect(fxChainItemInfoArray[i]);

		if(editingEffectNum == i)
		{
			pages.setAmount(fxChainItemInfoArray[i].nEffectsAmount);
		}

		if (fxChainItemInfoArray[i].eEffectNameID == T_EMPTYEFFECT)
		{
			effectPictograms[i]->setVisible(false);
		}
	}

	effectPictograms[selectedEffectNum]->select(true);
	effectPictograms[editingEffectNum]->edit(true);

	Unicode::snprintf(effectNameBuffer, EFFECTNAME_SIZE, "%s",
			touchgfx::TypedText(
					fxChainItemInfoArray[editingEffectNum].eEffectNameID).getText());

	const U8 channelName[CHANNELNAME_SIZE] = {0};
	if (eChannelType <= CHANNEL_MONO_4)
	{
		sprintf((char*) channelName, "MONO %d", (U8) eChannelType + 1);
	}
	else
	{
		sprintf((char*) channelName, "STEREO %d", (U8)(eChannelType - CHANNEL_MONO_4));
	}

	Unicode::fromUTF8(channelName, channelNameBuffer, CHANNELNAME_SIZE);

	for (int i = 0; i < 4; i++) {

		U8 nParamValue = presenter->getFXParam(eChannelType, editingEffectNum, i + pages.getPage() * 4);

		customGauges[i]->setValue(nParamValue);

		switch (fxChainItemInfoArray[editingEffectNum].eEffectNameID) {
		default:
			customGauges[i]->setParamName((TEXTS) (T_RESOURCEID1 + i + pages.getPage() * 4));
			break;
		}
	}

    EffectSettingsViewBase::setupScreen();
}

void EffectSettingsView::tearDownScreen()
{
    EffectSettingsViewBase::tearDownScreen();
}

void EffectSettingsView::encMenuUpdate(S8 nValue)
{
	S8 newPos = selectedEffectNum + nValue;
	if (newPos >= 0 && newPos < MAX_EFFECTS_NUM
			&& effectPictograms[newPos]->isVisible()) {
		effectPictograms[selectedEffectNum]->select(false);
		selectedEffectNum = newPos;
		effectPictograms[selectedEffectNum]->select(true);
	}
}

void EffectSettingsView::encParamUpdate(U8 nID, S8 nValue)
{
	customGauges[nID]->changeValue(nValue);
	presenter->saveFXParam(eChannelType, editingEffectNum, nID + pages.getPage() * 4, customGauges[nID]->getValue());
}

void EffectSettingsView::btnYesUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	if (selectedEffectNum != editingEffectNum) {

		effectPictograms[editingEffectNum]->edit(false);
		editingEffectNum = selectedEffectNum;
		effectPictograms[editingEffectNum]->edit(true);

		presenter->saveChannelChainPosition(eChannelType, editingEffectNum);

		Unicode::snprintf(effectNameBuffer, EFFECTNAME_SIZE, "%s",
				touchgfx::TypedText(
						fxChainItemInfoArray[editingEffectNum].eEffectNameID).getText());
		effectName.invalidate();

		pages.setAmount(fxChainItemInfoArray[editingEffectNum].nEffectsAmount);

		for (int i = 0; i < 4; i++) {

			U8 nParamValue = presenter->getFXParam(eChannelType, editingEffectNum, i + pages.getPage() * 4);

			customGauges[i]->setValue(nParamValue);

			switch (fxChainItemInfoArray[editingEffectNum].eEffectNameID) {
			default:
				customGauges[i]->setParamName((TEXTS) (T_RESOURCEID1 + i + pages.getPage() * 4));
				break;
			}

			customGauges[i]->invalidate();
		}
	}
}

void EffectSettingsView::btnNoUpdate(S8 nValue, BOOLEAN bIsFuncPressed)
{
	application().gotoFXChainScreenNoTransition();
}

void EffectSettingsView::btnUpUpdate(BOOLEAN bIsFuncPressed)
{
	if(TRUE == pages.pageUp())
	{
		for (int i = 0; i < 4; i++) {

			U8 nParamValue = presenter->getFXParam(eChannelType, editingEffectNum, i + pages.getPage() * 4);

			customGauges[i]->setValue(nParamValue);

			switch (fxChainItemInfoArray[editingEffectNum].eEffectNameID) {
			default:
				customGauges[i]->setParamName((TEXTS) (T_RESOURCEID1 + i + pages.getPage() * 4));
				break;
			}

			customGauges[i]->invalidate();
		}
	}
}

void EffectSettingsView::btnDownUpdate(BOOLEAN bIsFuncPressed)
{
	if(TRUE == pages.pageDown())
	{
		for (int i = 0; i < 4; i++) {

			U8 nParamValue = presenter->getFXParam(eChannelType, editingEffectNum, i + pages.getPage() * 4);

			customGauges[i]->setValue(nParamValue);

			switch (fxChainItemInfoArray[editingEffectNum].eEffectNameID) {
			default:
				customGauges[i]->setParamName((TEXTS) (T_RESOURCEID1 + i + pages.getPage() * 4));
				break;
			}

			customGauges[i]->invalidate();
		}
	}
}
