#include <gui/recorder_screen/RecorderView.hpp>

RecorderView::RecorderView()
{

}

void RecorderView::setupScreen()
{
    RecorderViewBase::setupScreen();
}

void RecorderView::tearDownScreen()
{
    RecorderViewBase::tearDownScreen();
}

void RecorderView::encMenuUpdate(S8 nValue)
{

}

void RecorderView::btnYesUpdate(S8 nValue)
{

}

void RecorderView::btnNoUpdate(S8 nValue)
{
	application().gotoSystemScreenNoTransition();
}

void RecorderView::btnUpUpdate()
{

}

void RecorderView::btnDownUpdate()
{

}
