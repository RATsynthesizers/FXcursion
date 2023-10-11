/*
 * GUIadapter.h
 *
 *  Created on: May 6, 2023
 *      Author: romte
 */

#ifndef HW_GUIADAPTER_H_
#define HW_GUIADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "usart.h"
#include "MemRegions.h"
#include "../../Modules/SysGlobals.h"

#define GUIUART_INBUF_SIZE  32  // bytes
#define GUIUART_OUTBUF_SIZE 32

typedef enum {
	REQUEST_PARAM,
	CHANGE_PARAM,
	CHAIN_REWRITE,
	LOAD_PROJECT,
	FX_ADDED,
	CMD_END
}GUIcommand_TypeDef;

typedef struct {
	GUIfxName_TypeDef name;
	uint8_t instance;
	uint8_t order_index;
}GUIfx_TypeDef;

typedef struct {
	GUIcommand_TypeDef cmd;
	uint8_t chain;
	GUIfx_TypeDef fxs[MAX_MODULES_IN_CHAIN];
	uint8_t fxsIdx;
	uint8_t parameterIndex;
	uint8_t parameterValue;
}GUImsg_TypeDef;

typedef struct {
	uint8_t RxBuf[GUIUART_INBUF_SIZE];
	uint8_t TxBuf[GUIUART_OUTBUF_SIZE];
	GUImsg_TypeDef msgBuf[MAX_MODULES_NUM];
	uint8_t msgBufIdx;	// up to MAX_MODULES_NUM
	uint8_t sync;
}GUIadapter_TypeDef;

void guiAdapter_Init(GUIadapter_TypeDef* adapter);
inline void MyGUIcallback(GUIadapter_TypeDef *adapter);
inline void putGUImsg(GUIadapter_TypeDef* adapter);
void transmitGUImsg(GUIadapter_TypeDef *adapter, uint8_t size);
uint8_t guiAdapterCheckUpdate(GUIadapter_TypeDef *adapter);






extern GUIadapter_TypeDef guiadapter;		// global adapter instance

#ifdef __cplusplus
}
#endif

#endif /* HW_GUIADAPTER_H_ */
