#ifndef OUTPUTVIEW_HPP
#define OUTPUTVIEW_HPP

#include <gui_generated/output_screen/OutputViewBase.hpp>
#include <gui/output_screen/OutputPresenter.hpp>

class OutputView : public OutputViewBase
{
public:
    OutputView();
    virtual ~OutputView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void encMenuUpdate(S8 nValue);
    void btnYesUpdate(S8 nValue);
    void btnNoUpdate(S8 nValue);
    void btnUpUpdate();
    void btnDownUpdate();
protected:
};

#endif // OUTPUTVIEW_HPP
