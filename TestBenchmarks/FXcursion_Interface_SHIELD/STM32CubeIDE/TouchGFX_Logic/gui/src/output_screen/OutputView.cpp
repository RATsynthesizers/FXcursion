#include <gui/output_screen/OutputView.hpp>

OutputView::OutputView()
{

}

void OutputView::setupScreen()
{
    OutputViewBase::setupScreen();
}

void OutputView::tearDownScreen()
{
    OutputViewBase::tearDownScreen();
}

void OutputView::encMenuUpdate(S8 nValue)
{

}

void OutputView::btnYesUpdate(S8 nValue)
{

}

void OutputView::btnNoUpdate(S8 nValue)
{
	application().gotoSystemScreenNoTransition();
}

void OutputView::btnUpUpdate()
{

}

void OutputView::btnDownUpdate()
{

}
