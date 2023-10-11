/*********************************************************************************/
/********** THIS FILE IS GENERATED BY TOUCHGFX DESIGNER, DO NOT MODIFY ***********/
/*********************************************************************************/
#ifndef EFFECTPICTOGRAMBASE_HPP
#define EFFECTPICTOGRAMBASE_HPP

#include <gui/common/FrontendApplication.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/canvas/Shape.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include <touchgfx/widgets/Image.hpp>

class EffectPictogramBase : public touchgfx::Container
{
public:
    EffectPictogramBase();
    virtual ~EffectPictogramBase();
    virtual void initialize();

protected:
    FrontendApplication& application() {
        return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
    }

    /*
     * Member Declarations
     */
    touchgfx::Box editingBox;
    touchgfx::Shape<8> selectingBox;
    touchgfx::PainterRGB565 selectingBoxPainter;
    touchgfx::Image pictRegular;
    touchgfx::Image pictEditing;

private:

};

#endif // EFFECTPICTOGRAMBASE_HPP