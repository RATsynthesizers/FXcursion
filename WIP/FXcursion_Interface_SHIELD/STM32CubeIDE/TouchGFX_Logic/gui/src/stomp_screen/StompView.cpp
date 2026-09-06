#include <gui/stomp_screen/StompView.hpp>

StompView::StompView()
{

}

void StompView::setupScreen()
{
    StompViewBase::setupScreen();
}

void StompView::tearDownScreen()
{
    StompViewBase::tearDownScreen();
}

void StompView::encMenuUpdate(S8 nValue)
{

}

void StompView::btnYesUpdate(S8 nValue)
{

}

void StompView::btnNoUpdate(S8 nValue)
{
	application().gotoSystemScreenNoTransition();
}

void StompView::btnUpUpdate()
{

}

void StompView::btnDownUpdate()
{

}
