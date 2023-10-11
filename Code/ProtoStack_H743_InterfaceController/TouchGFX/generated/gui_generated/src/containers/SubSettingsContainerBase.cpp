/*********************************************************************************/
/********** THIS FILE IS GENERATED BY TOUCHGFX DESIGNER, DO NOT MODIFY ***********/
/*********************************************************************************/
#include <gui_generated/containers/SubSettingsContainerBase.hpp>
#include <touchgfx/Color.hpp>

SubSettingsContainerBase::SubSettingsContainerBase()
{
    setWidth(320);
    setHeight(240);
    darkeningBackground.setPosition(0, 0, 320, 240);
    darkeningBackground.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    darkeningBackground.setAlpha(149);
    add(darkeningBackground);

    subMenuWindow.setPosition(25, 56, 270, 152);
    subMenuWindow.setColor(touchgfx::Color::getColorFromRGB(83, 89, 201));
    subMenuWindow.setBorderColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    subMenuWindow.setBorderSize(5);
    add(subMenuWindow);

    listLayout.setXY(45, 75);
    listLayout.setDirection(touchgfx::SOUTH);

    listLayout.add(subMenuItem0);

    listLayout.add(subMenuItem1);

    listLayout.add(subMenuItem2);

    add(listLayout);
}

SubSettingsContainerBase::~SubSettingsContainerBase()
{

}

void SubSettingsContainerBase::initialize()
{
    subMenuItem0.initialize();
    subMenuItem1.initialize();
    subMenuItem2.initialize();
}
