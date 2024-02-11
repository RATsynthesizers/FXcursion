/*
 * GFXadapter.h
 *
 *  Created on: Dec 3, 2021
 *      Author: rorka
 */

#ifndef HW_MYTOUCHGFXADAPTER_GFXADAPTER_H_
#define HW_MYTOUCHGFXADAPTER_GFXADAPTER_H_



#include "../../../Core/Inc/main.h"
#include "../ili9341/ili9341.h"
#include "../W9812G6JH/w9812g6jh.h"


//#ifdef __cplusplus
//extern "C" {
//#endif

int touchgfxDisplayDriverTransmitActive(void);
void touchgfxDisplayDriverTransmitBlock(uint8_t* pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

//#ifdef __cplusplus
//}
//#endif

#endif /* HW_MYTOUCHGFXADAPTER_GFXADAPTER_H_ */
