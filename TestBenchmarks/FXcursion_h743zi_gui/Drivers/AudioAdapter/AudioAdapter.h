/*
 * AudioAdapter.h
 *
 *  Created on: 8 июл. 2023 г.
 *      Author: Predtech
 */

#ifndef AUDIOADAPTER_AUDIOADAPTER_H_
#define AUDIOADAPTER_AUDIOADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usart.h"



#define AUDIOUART_INBUF_SIZE  4
#define AUDIOUART_OUTBUF_SIZE 5

typedef enum {
	REQUEST_PARAM,
	CHANGE_PARAM,
	FX_ADD,
	FX_MOVE_UP,
	FX_MOVE_DOWN,
	FX_DELETE,
	CHAIN_PLUG_WIRE,
	CHAIN_UNPLUG_WIRE
}AudioCommand_TypeDef;

typedef struct {
	uint8_t RxBuf[AUDIOUART_INBUF_SIZE];
	uint8_t TxBuf[AUDIOUART_OUTBUF_SIZE];
	uint8_t updateFlag;
}AudioAdapter_TypeDef;

void audioAdapter_Init(AudioAdapter_TypeDef* adapter);

extern AudioAdapter_TypeDef audioAdapter;		// global adapter instance

#ifdef __cplusplus
}
#endif

#endif /* AUDIOADAPTER_AUDIOADAPTER_H_ */
