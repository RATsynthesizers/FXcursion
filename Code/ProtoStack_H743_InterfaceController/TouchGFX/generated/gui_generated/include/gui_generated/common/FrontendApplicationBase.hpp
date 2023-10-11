/*********************************************************************************/
/********** THIS FILE IS GENERATED BY TOUCHGFX DESIGNER, DO NOT MODIFY ***********/
/*********************************************************************************/
#ifndef FRONTENDAPPLICATIONBASE_HPP
#define FRONTENDAPPLICATIONBASE_HPP

#include <mvp/MVPApplication.hpp>
#include <gui/model/Model.hpp>

class FrontendHeap;

class FrontendApplicationBase : public touchgfx::MVPApplication
{
public:
    FrontendApplicationBase(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplicationBase() { }

    virtual void changeToStartScreen()
    {
        gotoLogoStartupScreenNoTransition();
    }

    // LogoStartup
    void gotoLogoStartupScreenNoTransition();

    // ProjectSettings
    void gotoProjectSettingsScreenBlockTransition();

    // FXchain
    void gotoFXchainScreenBlockTransition();

    // EffectSettings
    void gotoEffectSettingsScreenBlockTransition();

protected:
    touchgfx::Callback<FrontendApplicationBase> transitionCallback;
    FrontendHeap& frontendHeap;
    Model& model;

    // LogoStartup
    void gotoLogoStartupScreenNoTransitionImpl();

    // ProjectSettings
    void gotoProjectSettingsScreenBlockTransitionImpl();

    // FXchain
    void gotoFXchainScreenBlockTransitionImpl();

    // EffectSettings
    void gotoEffectSettingsScreenBlockTransitionImpl();
};

#endif // FRONTENDAPPLICATIONBASE_HPP