/*
 * MemRegions.h
 *
 *  Created on: 13 авг. 2022 г.
 *      Author: romte
 */

#ifndef INC_MEMREGIONS_H_
#define INC_MEMREGIONS_H_

#define IN_DTCMRAM 		__attribute__ ((section(".dtcm"), used))
#define IN_ITCMRAM 		__attribute__ ((section(".itcm"), used))
#define IN_RAM_D1_DMA 	__attribute__ ((section(".ramd1dma"), used))
#define IN_RAMD1 		__attribute__ ((section(".ramd1"), used))
#define IN_RAMD2 		__attribute__ ((section(".ramd2"), used))
#define IN_RAMD3 		__attribute__ ((section(".ramd3"), used))

#endif /* INC_MEMREGIONS_H_ */
