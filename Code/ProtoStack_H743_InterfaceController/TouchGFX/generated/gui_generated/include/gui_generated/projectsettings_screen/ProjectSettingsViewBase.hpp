/*********************************************************************************/
/********** THIS FILE IS GENERATED BY TOUCHGFX DESIGNER, DO NOT MODIFY ***********/
/*********************************************************************************/
#ifndef PROJECTSETTINGSVIEWBASE_HPP
#define PROJECTSETTINGSVIEWBASE_HPP

#include <gui/common/FrontendApplication.hpp>
#include <mvp/View.hpp>
#include <gui/projectsettings_screen/ProjectSettingsPresenter.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/BoxWithBorder.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <gui/containers/WiringChangeBox.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/canvas/Line.hpp>
#include <touchgfx/widgets/canvas/PainterRGB565.hpp>
#include <touchgfx/containers/ListLayout.hpp>
#include <gui/containers/SettingsItem.hpp>
#include <gui/containers/SubSettingsContainer.hpp>

class ProjectSettingsViewBase : public touchgfx::View<ProjectSettingsPresenter>
{
public:
    ProjectSettingsViewBase();
    virtual ~ProjectSettingsViewBase();
    virtual void setupScreen();

protected:
    FrontendApplication& application() {
        return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
    }

    /*
     * Member Declarations
     */
    touchgfx::Box __background;
    touchgfx::BoxWithBorder background;
    touchgfx::BoxWithBorder projectNameBox;
    touchgfx::TextAreaWithOneWildcard projectSettings_text;
    WiringChangeBox wiringChangeBox;
    touchgfx::Container linesContainer;
    touchgfx::Line line1_1;
    touchgfx::PainterRGB565 line1_1Painter;
    touchgfx::Line line1_2;
    touchgfx::PainterRGB565 line1_2Painter;
    touchgfx::Line line1_3;
    touchgfx::PainterRGB565 line1_3Painter;
    touchgfx::Line line2_1;
    touchgfx::PainterRGB565 line2_1Painter;
    touchgfx::Line line2_2;
    touchgfx::PainterRGB565 line2_2Painter;
    touchgfx::Line line2_3;
    touchgfx::PainterRGB565 line2_3Painter;
    touchgfx::Line line3_1;
    touchgfx::PainterRGB565 line3_1Painter;
    touchgfx::Line line3_2;
    touchgfx::PainterRGB565 line3_2Painter;
    touchgfx::Line line3_3;
    touchgfx::PainterRGB565 line3_3Painter;
    touchgfx::ListLayout outputsList;
    SettingsItem outputItem0;
    SettingsItem outputItem1;
    SettingsItem outputItem2;
    touchgfx::ListLayout inputsList;
    SettingsItem inputItem0;
    SettingsItem inputItem1;
    SettingsItem inputItem2;
    SubSettingsContainer subSettingsContainer;

    /*
     * Wildcard Buffers
     */
    static const uint16_t PROJECTSETTINGS_TEXT_SIZE = 20;
    touchgfx::Unicode::UnicodeChar projectSettings_textBuffer[PROJECTSETTINGS_TEXT_SIZE];

private:

    /*
     * Canvas Buffer Size
     */
    static const uint32_t CANVAS_BUFFER_SIZE = 4800;
    uint8_t canvasBuffer[CANVAS_BUFFER_SIZE];

};

#endif // PROJECTSETTINGSVIEWBASE_HPP