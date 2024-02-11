#ifndef HW_GUIADAPTER_H_
#define HW_GUIADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usart.h"
#include "MemRegions.h"
#include "../../Modules/SysGlobals.h"

#define GUIUART_INBUF_SIZE  5
#define GUIUART_OUTBUF_SIZE 4

typedef enum {
	REQUEST_PARAM,
	CHANGE_PARAM,
	FX_ADD,
	FX_MOVE_UP,
	FX_MOVE_DOWN,
	FX_DELETE,
	CHAIN_PLUG_WIRE,
	CHAIN_UNPLUG_WIRE
}GUIcommand_TypeDef;

typedef struct {
	uint8_t RxBuf[GUIUART_INBUF_SIZE];
	uint8_t TxBuf[GUIUART_OUTBUF_SIZE];
	uint8_t updateFlag;
}GUIadapter_TypeDef;

void guiAdapter_Init(GUIadapter_TypeDef* adapter);

extern GUIadapter_TypeDef guiadapter;		// global adapter instance

#ifdef __cplusplus
}
#endif

#endif /* HW_GUIADAPTER_H_ */
