/*********************************************************************************/
/********** THIS FILE IS GENERATED BY TOUCHGFX DESIGNER, DO NOT MODIFY ***********/
/*********************************************************************************/
#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <touchgfx/Color.hpp>
#include <BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

Screen1ViewBase::Screen1ViewBase()
{

    __background.setPosition(0, 0, 320, 240);
    __background.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));

    buttonWithLabel1.setXY(75, 18);
    buttonWithLabel1.setBitmaps(touchgfx::Bitmap(BITMAP_DARK_BUTTONS_ROUND_EDGE_SMALL_ID), touchgfx::Bitmap(BITMAP_DARK_BUTTONS_ROUND_EDGE_SMALL_PRESSED_ID));
    buttonWithLabel1.setLabelText(touchgfx::TypedText(T___SINGLEUSE_CFVR));
    buttonWithLabel1.setLabelColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    buttonWithLabel1.setLabelColorPressed(touchgfx::Color::getColorFromRGB(255, 255, 255));

    add(__background);
    add(buttonWithLabel1);
}

void Screen1ViewBase::setupScreen()
{

}
