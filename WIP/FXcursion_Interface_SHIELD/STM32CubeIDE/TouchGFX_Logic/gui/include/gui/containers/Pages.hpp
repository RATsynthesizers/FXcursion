#ifndef PAGES_HPP
#define PAGES_HPP

#include <gui_generated/containers/PagesBase.hpp>

class Pages : public PagesBase
{
public:
    Pages();
    virtual ~Pages() {}

    virtual void initialize();

    void setAmount(U8 effectsAmount);
    BOOLEAN pageUp();
    BOOLEAN pageDown();
    U8 getPage();
protected:

    U8 nPagesAmount = 1;
    U8 nPageSelected = 0;

    touchgfx::colortype selectedColor;
    touchgfx::colortype normalColor;
};

#endif // PAGES_HPP
