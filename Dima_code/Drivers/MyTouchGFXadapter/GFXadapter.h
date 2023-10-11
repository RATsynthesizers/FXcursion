/*
 * GFXadapter.h
 *
 *  Created on: Dec 3, 2021
 *      Author: rorka
 */

#ifndef MYTOUCHGFXADAPTER_GFXADAPTER_H_
#define MYTOUCHGFXADAPTER_GFXADAPTER_H_

//#ifdef __cplusplus
//extern "C" {
//#endif

int touchgfxDisplayDriverTransmitActive(void);
void touchgfxDisplayDriverTransmitBlock(const uint8_t* pixels, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

//#ifdef __cplusplus
//}
//#endif

#endif /* MYTOUCHGFXADAPTER_GFXADAPTER_H_ */
