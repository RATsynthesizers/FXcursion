#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include "string.h"
#include "stdio.h"
#include "main.h"
#include "cmsis_os.h"

#include "../../../../../Drivers/HW/AudioAdapter/AudioAdapter.h"


#define CHANNELS_NUM 6
#define WIRES_NUM ((CHANNELS_NUM / 2) * (CHANNELS_NUM / 2))
#define MAX_EFFECTS_NUM 4
#define EFFECT_TYPES 10

extern UART_HandleTypeDef huart2;

class ModelListener;

//typedef enum {
//	REQUEST_PARAM,
//	CHANGE_PARAM,
//	CHAIN_REWRITE,
//	LOAD_PROJECT,
//	FX_ADDED,
//	CMD_END
//}GUIcommand_TypeDef;

struct MenuItemInfo {
	TEXTS effectNameID;
	uint16_t bitmapRegular;
	uint16_t bitmapSelected;
};

struct EffectInfo {
	TEXTS effectNameID;
	bool available;
	uint16_t bitmapRegular;
	uint16_t bitmapSelected;
	int associatedNumberRegular;
	int associatedNumberSelected;
};

class Model {
public:
	Model();

	void bind(ModelListener *listener) {
		modelListener = listener;
	}

	void tick();

	/******* Saving and getting global info to and from model *******/

	//scroll position in project settings
	void saveSettingsPosition(uint8_t saveSettingsPos) {
		settingsPosition = saveSettingsPos;
	}
	uint8_t getSettingsPosition() {
		return settingsPosition;
	}

	//project name
	void saveProjectName(const uint8_t *saveProjectName) {
		projectName = saveProjectName;
	}
	const uint8_t* getProjectName() {
		return projectName;
	}

	//channel number
	void saveChannelNum(uint8_t saveChannelNumber) {
		channelNumber = saveChannelNumber;
	}
	uint8_t getChannelNum() {
		return channelNumber;
	}

	//all effects chain on each channel
	void saveFXchain(uint8_t channelNum, MenuItemInfo *menuItemInfoArray) {
		for (int i = 0; i < MAX_EFFECTS_NUM; i++)
			channelEffectsArray[channelNum][i] = menuItemInfoArray[i];
	}
	MenuItemInfo getFXchainItem(uint8_t channelNum, uint8_t chainItem) {
		return channelEffectsArray[channelNum][chainItem];
	}

	//scroll position of chain on each channel
	void saveChannelChainPosition(uint8_t channelNum, uint8_t saveChannelPos) {
		channelChainPosition[channelNum] = saveChannelPos;
	}
	uint8_t getChannelChainPosition(uint8_t channelNum) {
		return channelChainPosition[channelNum];
	}

	//effect info for each channel
	void saveEffectInfo(uint8_t channelNum, EffectInfo *effectInfoArray) {
		for (int i = 0; i < EFFECT_TYPES; i++)
			channelEffectAmountsArray[channelNum][i] = effectInfoArray[i];
	}
	EffectInfo getEffectInfo(uint8_t channelNum, uint8_t effectPosition) {
		return channelEffectAmountsArray[channelNum][effectPosition];
	}

	//save and get wiring
	void saveWire(uint8_t wireID, bool isConnected) {
		wireConnected[wireID] = isConnected;
	}
	bool getWire(uint8_t wireID) {
		return wireConnected[wireID];
	}

protected:
	ModelListener *modelListener;

	MenuItemInfo channelEffectsArray[CHANNELS_NUM][MAX_EFFECTS_NUM];
	EffectInfo channelEffectAmountsArray[CHANNELS_NUM][EFFECT_TYPES];
	uint8_t channelChainPosition[CHANNELS_NUM];
	bool wireConnected[WIRES_NUM];
	uint8_t channelNumber;
	uint8_t settingsPosition;
	const uint8_t *projectName;
};

#endif // MODEL_HPP
