/*
 * MySAIadapter.h
 *
 *  Created on: Jul 3, 2022
 *      Author: romte
 */

#ifndef HW_MYSAIADAPTER_H_
#define HW_MYSAIADAPTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "sai.h"
#include "spi.h"
#include "../../Modules/SysGlobals.h"
#include "MemRegions.h"
#include "wm8731.h"

#define AUDIOBUF_SIZE 2048

#define SAI_HALF_BUF (SAI_IO_BUF_SIZE / 2)

typedef enum {HW_SAI1 = 1, HW_SAI2} HW_SAI_Instance;
typedef enum {HW_SAI_CURRENT_BUF = 0, HW_SAI_SWITCH_BUF} HW_SAI_UpdateStatus;

typedef struct {
	HW_SAI_Instance saiInstance; // can be of HW_SAI_Types
	uint32_t TxFlag;			// status flag
	uint32_t TxHalfFlag;		// status flag
	uint32_t RxFlag;			// status flag
	uint32_t RxHalfFlag;		// status flag
	uint16_t BufOffset;		// offset for double-buffer
	uint16_t rxBufPosition;	// current sample to process, 0..SAI_HALF_BUF
	uint16_t txBufPosition;	// current sample to output,  0..SAI_HALF_BUF
	int16_t RxData[SAI_IO_BUF_SIZE];  // sai rx buffer
	int16_t TxData[SAI_IO_BUF_SIZE];  // sai tx buffer
}SAIadapter_TypeDef;

void saiAdapter_Init(SAIadapter_TypeDef* adapter, HW_SAI_Instance sai_instatnce);
HW_SAI_UpdateStatus saiAdapterCheckUpdate(SAIadapter_TypeDef* adapter);
int16_t saiAdapter_getNextSample(SAIadapter_TypeDef* adapter);
void saiAdapter_setNextSample(SAIadapter_TypeDef* adapter, int16_t sample);


extern SAIadapter_TypeDef sai1adapter;		// global adapter instances
extern SAIadapter_TypeDef sai2adapter;

#ifdef __cplusplus
}
#endif

#endif /* HW_MYSAIADAPTER_H_ */
