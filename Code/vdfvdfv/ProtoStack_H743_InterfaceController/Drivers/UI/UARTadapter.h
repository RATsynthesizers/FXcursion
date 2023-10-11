#ifndef HW_UARTADAPTER_H_
#define HW_UARTADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "usart.h"
#include "MemRegions.h"

#define UART_INMSG_SIZE  4  // incoming bytes
#define UART_OUTMSG_SIZE 5  // outgoing bytes

typedef enum {
	REQUEST_PARAM,
	CHANGE_PARAM,
	CHAIN_REWRITE,
	LOAD_PROJECT,
	FX_ADDED,
	CMD_END
}GUIcmdType_TypeDef;

// GUI message format
typedef struct {
	GUIcmdType_TypeDef cmd;
	uint8_t chain;
	uint8_t fxIdx;
	uint8_t parameterIndex;
	uint8_t parameterValue;
}GuiMsg_TypeDef;

// audio parameters message format
typedef struct {
	uint8_t params[UART_OUTMSG_SIZE];
}AudioParamMsg_TypeDef;

typedef struct {
	uint8_t RxBuf[UART_INMSG_SIZE];
	uint8_t TxBuf[UART_OUTMSG_SIZE];
	GuiMsg_TypeDef msgOut;
	AudioParamMsg_TypeDef msgIn;
	uint8_t update_flag;
}UARTadapter_TypeDef;

void UARTadapter_Init(UARTadapter_TypeDef* adapter);
void getUARTMsgRx(UARTadapter_TypeDef* adapter);
void prepareUARTMsgTx(UARTadapter_TypeDef* adapter);
void transmitUARTmsg(UARTadapter_TypeDef *adapter);
uint8_t UARTadapterCheckUpdate(UARTadapter_TypeDef *adapter);

extern UARTadapter_TypeDef audio_uart_adapter;		// global adapter instance


#ifdef __cplusplus
}
#endif

#endif /* HW_UARTADAPTER_H_ */
