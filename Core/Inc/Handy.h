/*
 * globals.h
 *
 *  Created on: Nov 10, 2020
 *      Author: romte
 */

#ifndef INC_HANDY_H_
#define INC_HANDY_H_

#include <stdio.h>
#include <stdint.h>  // uintXX_t support
#include <limits>
#include <math.h>
#include "DWT.h"
#include "MemRegions.h"


using u8 = uint8_t;       // shortcuts
using u16 = uint16_t;
using u32 = uint32_t;


#define MY_FLOAT_MAX (std::numeric_limits<float>::max())  // float max value (use -FLOAT_MAX for negative min)
#define MY_INT16_MAX (std::numeric_limits<int16_t>::max())

#define STEREO 2
#define MONO   1

#define LEFT  0
#define RIGHT 1


#endif /* INC_HANDY_H_ */
