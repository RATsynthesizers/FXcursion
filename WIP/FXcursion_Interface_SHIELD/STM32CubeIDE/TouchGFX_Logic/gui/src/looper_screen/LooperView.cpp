#include <gui/looper_screen/LooperView.hpp>

LooperView::LooperView()
{

}

void LooperView::setupScreen()
{
    LooperViewBase::setupScreen();
}

void LooperView::tearDownScreen()
{
    LooperViewBase::tearDownScreen();
}

void LooperView::encMenuUpdate(S8 nValue)
{

}

void LooperView::btnYesUpdate(S8 nValue)
{

}

void LooperView::btnNoUpdate(S8 nValue)
{
	application().gotoSystemScreenNoTransition();
}

void LooperView::btnUpUpdate()
{

}

void LooperView::btnDownUpdate()
{

}
