/******************************************************************************
* Copyright (c) 2018(-2022) STMicroelectronics.
* All rights reserved.
*
* This file is part of the TouchGFX 4.20.0 distribution.
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
const uint16_t RMASK = 0xF800; ///< Mask for red   (1111100000000000)
const uint16_t GMASK = 0x07E0; ///< Mask for green (0000011111100000)
const uint16_t BMASK = 0x001F; ///< Mask for blue  (0000000000011111)

/**
 * Mix colors from a new pixel and a buffer pixel with the given alpha applied to the
 * new pixel, and the inverse alpha applied to the buffer pixel.
 *
 * @param  R      The red color (0-31 shifted into RMASK).
 * @param  G      The green color (0-63 shifted into GMASK).
 * @param  B      The blue color (0-31 shifted into BMASK).
 * @param  bufpix The buffer pixel value.
 * @param  alpha  The alpha of the R,G,B.
 *
 * @return The result of blending the two colors into a new color.
 */
FORCE_INLINE_FUNCTION uint16_t alphaBlend(uint16_t R, uint16_t G, uint16_t B, uint16_t bufpix, uint8_t alpha)
{
    const uint8_t ialpha = 0xFF - alpha;
    return (((R * alpha + (bufpix & RMASK) * ialpha) / 255) & RMASK) |
           (((G * alpha + (bufpix & GMASK) * ialpha) / 255) & GMASK) |
           (((B * alpha + (bufpix & BMASK) * ialpha) / 255) & BMASK);
}

/**
 * Mix colors from a new pixel and a buffer pixel with the given alpha applied to the
 * new pixel, and the inverse alpha applied to the buffer pixel.
 *
 * @param  newpix The new pixel value.
 * @param  bufpix The buffer pixel value.
 * @param  alpha  The alpha to apply to the new pixel.
 *
 * @return The result of blending the two colors into a new color.
 */
FORCE_INLINE_FUNCTION uint16_t alphaBlend(uint16_t newpix, uint16_t bufpix, uint8_t alpha)
{
    return alphaBlend(newpix & RMASK, newpix & GMASK, newpix & BMASK, bufpix, alpha);
}

/**
 * Generates a color representation to be used on the LCD, based on 24 bit RGB values.
 *
 * @param  color The color.
 *
 * @return The color representation depending on LCD color format.
 */
FORCE_INLINE_FUNCTION uint16_t getNativeColor(colortype color)
{
    return ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F);
}

namespace rgb565
{
void setL8Pallette(const uint8_t* const data);
void tearDown(void);

void lineFromColor(uint16_t* const ptr, const unsigned count, const uint32_t color, const uint8_t alpha, const uint32_t color565);
void lineFromRGB565(uint16_t* const ptr, const uint16_t* const data, const unsigned count, const uint8_t alpha);
void lineFromARGB8888(uint16_t* const ptr, const uint32_t* const data, const unsigned count, const uint8_t alpha);
void lineFromL8RGB888(uint16_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
void lineFromL8ARGB8888(uint16_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
} // namespace rgb565

namespace rgb888
{
void setL8Pallette(const uint8_t* const data);
void tearDown(void);

void lineFromColor(uint8_t* const ptr, const unsigned count, const uint32_t color, const uint8_t alpha);
void lineFromRGB888(uint8_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
void lineFromARGB8888(uint8_t* const ptr, const uint32_t* const data, const unsigned count, const uint8_t alpha);
void lineFromL8RGB888(uint8_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
void lineFromL8ARGB8888(uint8_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha);
} // namespace rgb888
} // namespace paint
} // namespace touchgfx

#endif // TOUCHGFX_PAINT_HPP
