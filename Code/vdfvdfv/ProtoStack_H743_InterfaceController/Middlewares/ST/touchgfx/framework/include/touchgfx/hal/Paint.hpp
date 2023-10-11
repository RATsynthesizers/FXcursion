/******************************************************************************
* Copyright (c) 2018(-2023) STMicroelectronics.
* All rights reserved.
*
* This file is part of the TouchGFX 4.21.3 distribution.
*
* This software is licensed under terms that can be found in the LICENSE file in
* the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
*******************************************************************************/

/**
 * @file touchgfx/hal/Paint.hpp
 *
 * Declares paint functions for widgets
 */
#ifndef TOUCHGFX_PAINT_HPP
#define TOUCHGFX_PAINT_HPP

#include <touchgfx/Color.hpp>

namespace touchgfx
{
namespace paint
{
void setL8Pallette(const uint8_t* const data);
void tearDown(void);

namespace rgb565
{
void lineFromColor(uint16_t* const ptr, const unsigned count, const uint32_t color, const uint8_t alpha, const uint32_t color565);
void lineFromRGB565(uint16_t* const ptr, const uint16_t* const data, const unsigned count, const uint8_t alpha);
void lineFromARGB8888(uint16_t* const ptr, const uint32_t* const data, const unsigned count, const uint8_t alpha);
void lineFromL8RGB888(uint16_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
void lineFromL8ARGB8888(uint16_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
} // namespace rgb565

namespace rgb888
{
void lineFromColor(uint8_t* const ptr, const unsigned count, const uint32_t color, const uint8_t alpha);
void lineFromRGB888(uint8_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
void lineFromARGB8888(uint8_t* const ptr, const uint32_t* const data, const unsigned count, const uint8_t alpha);
void lineFromL8RGB888(uint8_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
void lineFromL8ARGB8888(uint8_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);

} // namespace rgb888
} // namespace paint
} // namespace touchgfx

#endif // TOUCHGFX_PAINT_HPP
