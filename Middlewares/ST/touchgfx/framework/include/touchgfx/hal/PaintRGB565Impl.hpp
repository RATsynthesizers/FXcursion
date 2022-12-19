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
 * @file touchgfx/hal/PaintRGB565Impl.hpp
 *
 * Implements RGB565 software painter functions for widgets
 */
#ifndef TOUCHGFX_PAINTRGB565IMPL_HPP
#define TOUCHGFX_PAINTRGB565IMPL_HPP

#include <touchgfx/hal/Paint.hpp>

namespace touchgfx
{
namespace paint
{
namespace rgb565
{
const uint8_t* blendL8CLUT = 0;

void setL8Pallette(const uint8_t* const data)
{
    blendL8CLUT = data;
}

void tearDown(void)
{
}

void lineFromColor(uint16_t* const ptr, const unsigned count, const uint32_t color, const uint8_t alpha, const uint32_t color565)
{
    uint16_t* framebuffer = ptr;
    const uint16_t* const lineEnd = framebuffer + count;
    if (alpha == 0xFF)
    {
        do
        {
            *framebuffer = color565;
        } while (++framebuffer < lineEnd);
    }
    else
    {
        do
        {
            *framebuffer = alphaBlend(color565, *framebuffer, alpha);
        } while (++framebuffer < lineEnd);
    }
}

void lineFromRGB565(uint16_t* const ptr, const uint16_t* const data, const unsigned count, const uint8_t alpha)
{
    uint16_t* framebuffer = ptr;
    const uint16_t* bitmapPointer = data;
    const uint16_t* const chunkend = framebuffer + count;

    if (alpha == 0xFF)
    {
        do
        {
            *framebuffer = *bitmapPointer++;
        } while (++framebuffer < chunkend);
    }
    else
    {
        do
        {
            *framebuffer = alphaBlend(*bitmapPointer++, *framebuffer, alpha);
        } while (++framebuffer < chunkend);
    }
}

void lineFromARGB8888(uint16_t* const ptr, const uint32_t* const data, const unsigned count, const uint8_t alpha)
{
    uint16_t* framebuffer = ptr;
    const uint32_t* bitmapPointer = data;
    const uint16_t* const chunkend = framebuffer + count;
    do
    {
        const uint8_t srcAlpha = (*bitmapPointer) >> 24;
        const uint8_t a = LCD::div255(alpha * srcAlpha);
        if (a == 0xFF)
        {
            *framebuffer = getNativeColor(*bitmapPointer);
        }
        else if (a)
        {
            const uint32_t newpix = *bitmapPointer;
            *framebuffer = alphaBlend((newpix >> 8) & RMASK, (newpix >> 5) & GMASK, (newpix >> 3) & BMASK, *framebuffer, a);
        }
        bitmapPointer++;
    } while (++framebuffer < chunkend);
}

void lineFromL8RGB888(uint16_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha)
{
    uint16_t* framebuffer = ptr;
    const uint8_t* bitmapPointer = data;
    const uint16_t* const chunkend = framebuffer + count;
    if (alpha == 0xFF)
    {
        do
        {
            const uint8_t* src = &blendL8CLUT[*bitmapPointer++ * 3];
            // Use alpha from covers directly
            const uint8_t blue = *src++;
            const uint8_t green = *src++;
            const uint8_t red = *src;
            *framebuffer = ((red << 8) & RMASK) | ((green << 3) & GMASK) | ((blue >> 3) & BMASK);
        } while (++framebuffer < chunkend);
    }
    else
    {
        do
        {
            const uint8_t* src = &blendL8CLUT[*bitmapPointer++ * 3];
            // Use alpha from covers directly
            const uint8_t blue = *src++;
            const uint8_t green = *src++;
            const uint8_t red = *src;
            const uint8_t ialpha = 0xFF - alpha;
            const uint16_t bufpix = *framebuffer;
            uint8_t fbr = Color::getRedFromRGB565(bufpix);
            uint8_t fbg = Color::getGreenFromRGB565(bufpix);
            uint8_t fbb = Color::getBlueFromRGB565(bufpix);
            *framebuffer = ((LCD::div255(red * alpha + fbr * ialpha) << 8) & RMASK) | ((LCD::div255(green * alpha + fbg * ialpha) << 3) & GMASK) | ((LCD::div255(blue * alpha + fbb * ialpha) >> 3) & BMASK);
        } while (++framebuffer < chunkend);
    }
}

void lineFromL8ARGB8888(uint16_t* const ptr, const uint8_t* const data, const unsigned count, const uint8_t alpha)
{
    uint16_t* framebuffer = ptr;
    const uint8_t* bitmapPointer = data;
    const uint16_t* const chunkend = framebuffer + count;
    do
    {
        const uint32_t newpix = reinterpret_cast<const uint32_t*>(blendL8CLUT)[*bitmapPointer++];
        const uint8_t srcAlpha = newpix >> 24;
        const uint8_t a = LCD::div255(alpha * srcAlpha);
        if (a == 0xFF)
        {
            *framebuffer = getNativeColor(newpix);
        }
        else if (a)
        {
            *framebuffer = alphaBlend((newpix >> 8) & RMASK, (newpix >> 5) & GMASK, (newpix >> 3) & BMASK, *framebuffer, a);
        }
    } while (++framebuffer < chunkend);
}
} // namespace rgb565
} // namespace paint
} // namespace touchgfx

#endif // TOUCHGFX_PAINTRGB565IMPL_HPP
