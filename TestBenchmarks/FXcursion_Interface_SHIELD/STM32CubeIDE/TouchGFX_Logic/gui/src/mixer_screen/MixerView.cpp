#include <gui/mixer_screen/MixerView.hpp>

MixerView::MixerView()
{

}

void MixerView::setupScreen()
{
    MixerViewBase::setupScreen();
}

void MixerView::tearDownScreen()
{
    MixerViewBase::tearDownScreen();
}

void MixerView::encMenuUpdate(S8 nValue)
{

}

void MixerView::btnYesUpdate(S8 nValue)
{

}

void MixerView::btnNoUpdate(S8 nValue)
{
	application().gotoSystemScreenNoTransition();
}

void MixerView::btnUpUpdate()
{

}

void MixerView::btnDownUpdate()
{

}
