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

#include <touchgfx/Bitmap.hpp>
#include <touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp>
#include <touchgfx/canvas_widget_renderer/Rasterizer.hpp>
#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/transforms/DisplayTransformation.hpp>
#include <touchgfx/widgets/canvas/Canvas.hpp>

namespace touchgfx
{
Canvas::Canvas(const CanvasWidget* _widget, const Rect& invalidatedArea)
    : widget(_widget),
      invalidatedAreaX(0),
      invalidatedAreaY(0),
      invalidatedAreaWidth(0),
      invalidatedAreaHeight(0),
      rasterizer(),
      penUp(true),
      penHasBeenDown(false),
      previousX(0),
      previousY(0),
      previousOutside(0),
      penDownOutside(0),
      initialX(0),
      initialY(0)
{
    assert(CanvasWidgetRenderer::hasBuffer() && "No buffer allocated for CanvasWidgetRenderer drawing");
    assert(Rasterizer::POLY_BASE_SHIFT == 5 && "CanvasWidget assumes Q5 but Rasterizer uses a different setting");

    // Area to redraw (relative coordinates)
    Rect dirtyArea = Rect(0, 0, widget->getWidth(), widget->getHeight()) & invalidatedArea;

    // Absolute position of the scalableImage.
    dirtyAreaAbsolute = dirtyArea;
    widget->translateRectToAbsolute(dirtyAreaAbsolute);

    // Transform rects to match framebuffer coordinates
    // This is needed if the display is rotated compared to the framebuffer
    DisplayTransformation::transformDisplayToFrameBuffer(dirtyArea, widget->getRect());
    DisplayTransformation::transformDisplayToFrameBuffer(dirtyAreaAbsolute);

    // Re-size buffers for optimum memory buffer layout.
    rasterizer.reset(dirtyArea.x, dirtyArea.y);

    invalidatedAreaX = CWRUtil::toQ5<int>(dirtyArea.x);
    invalidatedAreaY = CWRUtil::toQ5<int>(dirtyArea.y);
    invalidatedAreaWidth = CWRUtil::toQ5<int>(dirtyArea.width);
    invalidatedAreaHeight = CWRUtil::toQ5<int>(dirtyArea.height);

    rasterizer.setMaxRender(dirtyAreaAbsolute.width, dirtyAreaAbsolute.height);
}

void Canvas::moveTo(CWRUtil::Q5 x, CWRUtil::Q5 y)
{
    if (!penUp)
    {
        if (!close())
        {
            return;
        }
    }

    transformFrameBufferToDisplay(x, y);
    x -= invalidatedAreaX;
    y -= invalidatedAreaY;

    const uint8_t outside = isOutside(x, y, invalidatedAreaWidth, invalidatedAreaHeight);

    if (outside)
    {
        penUp = true;
    }
    else
    {
        penDownOutside = outside;
        rasterizer.moveTo(x, y);
        penUp = false;
        penHasBeenDown = true;
    }

    initialX = x;
    initialY = y;

    previousX = x;
    previousY = y;
    previousOutside = outside;
}

void Canvas::lineTo(CWRUtil::Q5 x, CWRUtil::Q5 y)
{
    transformFrameBufferToDisplay(x, y);
    x -= invalidatedAreaX;
    y -= invalidatedAreaY;

    uint8_t outside = isOutside(x, y, invalidatedAreaWidth, invalidatedAreaHeight);

    if (!previousOutside)
    {
        rasterizer.lineTo(x, y);
    }
    else
    {
        if (!outside || !(previousOutside & outside))
        {
            // x,y is inside, or on another side compared to previous
            if (penUp)
            {
                penDownOutside = previousOutside;
                rasterizer.moveTo(previousX, previousY);
                penUp = false;
                penHasBeenDown = true;
            }
            else
            {
                rasterizer.lineTo(previousX, previousY);
            }
            rasterizer.lineTo(x, y);
        }
        else
        {
            // Restrict "outside" to the same side as previous point.
            outside &= previousOutside;
        }
    }
    previousX = x;
    previousY = y;
    previousOutside = outside;
}

bool Canvas::render(uint8_t customAlpha)
{
    const uint8_t alpha = LCD::div255(widget->getAlpha() * customAlpha);
    if (alpha == 0 || !penHasBeenDown)
    {
        return true; // Nothing. Done
    }

    // If the invalidated rect is too wide compared to the allocated buffer for CWR,
    // redrawing will not help. The CanvasWidget needs to know about this situation
    // and maybe try to divide the area vertically instead, but this has not been
    // implemented. And probably should not.
    if (!close())
    {
        return false;
    }

    // Create the rendering buffer
    uint8_t* RESTRICT framebuffer = reinterpret_cast<uint8_t*>(HAL::getInstance()->lockFrameBufferForRenderingMethod(widget->getPainter()->getRenderingMethod()));
    int stride = HAL::lcd().framebufferStride();
    uint8_t xAdjust = 0;
    switch (HAL::lcd().framebufferFormat())
    {
    case Bitmap::BW:
        framebuffer += (dirtyAreaAbsolute.x / 8) + dirtyAreaAbsolute.y * stride;
        xAdjust = dirtyAreaAbsolute.x % 8;
        break;
    case Bitmap::GRAY2:
        framebuffer += (dirtyAreaAbsolute.x / 4) + dirtyAreaAbsolute.y * stride;
        xAdjust = dirtyAreaAbsolute.x % 4;
        break;
    case Bitmap::GRAY4:
        framebuffer += (dirtyAreaAbsolute.x / 2) + dirtyAreaAbsolute.y * stride;
        xAdjust = dirtyAreaAbsolute.x % 2;
        break;
    case Bitmap::RGB565:
        framebuffer += dirtyAreaAbsolute.x * 2 + dirtyAreaAbsolute.y * stride;
        break;
    case Bitmap::RGB888:
        framebuffer += dirtyAreaAbsolute.x * 3 + dirtyAreaAbsolute.y * stride;
        break;
    case Bitmap::RGBA2222:
    case Bitmap::BGRA2222:
    case Bitmap::ARGB2222:
    case Bitmap::ABGR2222:
    case Bitmap::L8:
        framebuffer += dirtyAreaAbsolute.x + dirtyAreaAbsolute.y * stride;
        break;
    case Bitmap::ARGB8888:
        framebuffer += dirtyAreaAbsolute.x * 4 + dirtyAreaAbsolute.y * stride;
        break;
    case Bitmap::BW_RLE:
    case Bitmap::A4:
    case Bitmap::CUSTOM:
        assert(false && "Unsupported bit depth");
    }
    const bool result = rasterizer.render(widget->getPainter(), framebuffer, stride, xAdjust, alpha);
    HAL::getInstance()->unlockFrameBuffer();
    return result;
}

void Canvas::transformFrameBufferToDisplay(CWRUtil::Q5& x, CWRUtil::Q5& y) const
{
    if (HAL::DISPLAY_ROTATION == rotate90)
    {
        CWRUtil::Q5 tmpY = y;
        y = CWRUtil::toQ5<int>(widget->getWidth()) - x;
        x = tmpY;
    }
}

bool Canvas::close()
{
    if (!penUp)
    {
        if (previousOutside & penDownOutside)
        {
            // We are outside on the same side as we started. No need
            // to close the path, CWR will do this for us.
            //   lineTo(penDownX, penDownY);
        }
        else
        {
            if (previousOutside)
            {
                rasterizer.lineTo(previousX, previousY);
            }
            rasterizer.lineTo(initialX, initialY);
        }
    }
    penUp = false;
    return !rasterizer.wasOutlineTooComplex();
}
} // namespace touchgfx
