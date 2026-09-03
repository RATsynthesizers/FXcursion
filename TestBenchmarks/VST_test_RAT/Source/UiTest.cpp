/*
    Tests the panel itself, by building the real editor and inspecting it.

    Written after a regression that the load test could not see: every knob on
    the panel was disabled, because the code that re-enables them sat behind an
    early return that was normally taken. The audio path was perfect and the
    plugin was unusable.

    The VST3 load test cannot catch that - a hosted plugin's UI lives in its own
    window and its components are opaque from outside. So this target links the
    plugin sources directly, makes a processor and an editor, and asks the
    controls what state they are in.

    It checks the three things the panel promises:

      1. a knob is enabled exactly when the selected effect uses that slot
      2. a sync switch is visible exactly when the effect declares the
         parameter syncable
      3. turning sync on makes the knob a stepped division picker, and turning
         it off gives back a continuous 0..1 control
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FxUnits.h"

extern "C" {
#include "Effects/fx_delay.h"
}

#include <cmath>


/** What the DSP itself resolves a delay-time setting to. Deliberately NOT
    FxUnits, so the quantise test cannot pass by agreeing with itself. */
static float dspTime (float norm, bool sync, int division, float bpm)
{
    TEMPO tempo {};
    tempo.fBpm         = bpm;
    tempo.nBeatsPerBar = 4U;
    tempo.nBeatUnit    = 4U;

    FX_PARAM prm {};
    prm.fValue    = norm;
    prm.bSync     = (U8) (sync ? TRUE : FALSE);
    prm.eDivision = (U8) division;

    return FxParam_TimeSec (&prm, &tempo, DELAY_TIME_MIN_SEC, DELAY_TIME_MAX_SEC);
}


static juce::Component* findById (juce::Component* root, const juce::String& id)
{
    for (int i = 0; i < root->getNumChildComponents(); ++i)
    {
        auto* child = root->getChildComponent (i);

        if (child->getComponentID() == id)
            return child;

        if (auto* found = findById (child, id))
            return found;
    }

    return nullptr;
}

/** Let the editor's 20 Hz timer run, since that is what applies an effect change. */
static void pump (int ms = 120)
{
    juce::MessageManager::getInstance()->runDispatchLoopUntil (ms);
}


int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    FxBenchProcessor proc;
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());

    if (editor == nullptr)
    {
        std::puts ("FAIL: no editor");
        return 1;
    }

    std::printf ("editor  : %d x %d\n\n", editor->getWidth(), editor->getHeight());

    auto* fxParam = proc.apvts.getParameter ("fx");
    int   failures = 0;

    for (int fx = 0; fx < FxHost::effectCount(); ++fx)
    {
        fxParam->setValueNotifyingHost (fxParam->convertTo0to1 ((float) fx));
        pump();

        const int wantKnobs = FxHost::paramCount (fx);
        int gotKnobs = 0, gotSync = 0, wantSync = 0;

        for (int slot = 0; slot < (int) FX_PARAM_QTY; ++slot)
        {
            auto* knob = dynamic_cast<juce::Slider*>       (findById (editor.get(), "knob" + juce::String (slot)));
            auto* sync = dynamic_cast<juce::ToggleButton*> (findById (editor.get(), "sync" + juce::String (slot)));

            if (knob == nullptr || sync == nullptr)
            {
                std::printf ("FAIL: slot %d has no controls\n", slot);
                failures++;
                continue;
            }

            if (knob->isEnabled())  gotKnobs++;
            if (sync->isVisible())  gotSync++;

            if (FxHost::paramSyncable (fx, slot) && slot < wantKnobs)
                wantSync++;
        }

        const bool okKnobs = (gotKnobs == wantKnobs);
        const bool okSync  = (gotSync  == wantSync);

        std::printf ("  %-46s knobs %d/%d  sync %d/%d  %s\n",
                     FxHost::effectName (fx).toRawUTF8(),
                     gotKnobs, wantKnobs, gotSync, wantSync,
                     (okKnobs && okSync) ? "ok" : "FAIL");

        if (! okKnobs) { std::puts ("      -> wrong number of usable knobs"); failures++; }
        if (! okSync)  { std::puts ("      -> wrong number of sync switches"); failures++; }
    }

    // ---- the knob becomes the division picker when synced ---------------------
    {
        /* Delay stereo, parameter 0, is Time - syncable by declaration. */
        const int fx   = FX_DELAY_S;
        const int slot = FX_DELAYS_P_TIME;

        fxParam->setValueNotifyingHost (fxParam->convertTo0to1 ((float) fx));
        pump();

        auto* knob = dynamic_cast<juce::Slider*>       (findById (editor.get(), "knob" + juce::String (slot)));
        auto* sync = dynamic_cast<juce::ToggleButton*> (findById (editor.get(), "sync" + juce::String (slot)));

        if (knob == nullptr || sync == nullptr)
        {
            std::puts ("\nFAIL: delay time slot missing");
            return 1;
        }

        std::printf ("\nsync off: range %.3f..%.3f step %.3f\n",
                     knob->getMinimum(), knob->getMaximum(), knob->getInterval());

        if (knob->getInterval() > 0.001)
        {
            std::puts ("FAIL: unsynced knob should be continuous");
            failures++;
        }

        sync->setToggleState (true, juce::sendNotificationSync);
        pump();

        std::printf ("sync on : range %.3f..%.3f step %.3f\n",
                     knob->getMinimum(), knob->getMaximum(), knob->getInterval());

        /* Stepped across every note division, so dragging snaps to 1/4, 1/8.
           and the rest instead of landing between them. */
        if (! juce::approximatelyEqual (knob->getMaximum(), (double) (FxHost::divisionCount() - 1))
            || ! juce::approximatelyEqual (knob->getInterval(), 1.0))
        {
            std::puts ("FAIL: synced knob is not a stepped division picker");
            failures++;
        }

        /* And the sync flag really reached the parameter the DSP reads. */
        if (proc.apvts.getRawParameterValue (FxBenchProcessor::syncParamId (slot))->load() < 0.5f)
        {
            std::puts ("FAIL: sync switch did not reach the parameter");
            failures++;
        }

        sync->setToggleState (false, juce::sendNotificationSync);
        pump();

        if (knob->getInterval() > 0.001)
        {
            std::puts ("FAIL: knob did not go back to continuous");
            failures++;
        }
    }

    // ---- flipping sync quantises, it does not recall -------------------------
    {
        /*
           The free value and the division are separate parameters. Before this
           was handled, switching sync on simply rebound the knob and whatever
           division was last used came back - so the setting in front of you was
           thrown away.

           Measured with the firmware's own FxParam_TimeSec rather than with
           FxUnits, so this cannot pass by agreeing with itself.
        */
        const int fx   = FX_DELAY_M;
        const int slot = FX_DELAYM_P_TIME;

        fxParam->setValueNotifyingHost (fxParam->convertTo0to1 ((float) fx));
        pump();

        auto* sync    = dynamic_cast<juce::ToggleButton*> (findById (editor.get(), "sync" + juce::String (slot)));
        auto* valPrm  = proc.apvts.getParameter (FxBenchProcessor::fxParamId  (slot));
        auto* divPrm  = proc.apvts.getParameter (FxBenchProcessor::divParamId (slot));

        const float probes[] = { 0.0f, 0.17f, 0.33f, 0.5f, 0.68f, 0.85f, 1.0f };

        /* Adjacent divisions are never further apart than a factor of 1.5, so
           quantising can never move the value by more than half of that. */
        const float halfStep = std::log (1.5f) * 0.5f + 0.01f;

        std::puts ("\nsync toggle:");

        for (float norm : probes)
        {
            /* Poison the remembered division with the furthest one there is.
               If the old behaviour ever comes back, this is what will appear. */
            divPrm->setValueNotifyingHost (divPrm->convertTo0to1 ((float) DIV_1_1));
            sync->setToggleState (false, juce::sendNotificationSync);
            pump (40);

            valPrm->setValueNotifyingHost (norm);
            pump (40);

            const float freeSec = dspTime (norm, false, (int) DIV_1_4, 120.0f);

            sync->setToggleState (true, juce::sendNotificationSync);
            pump (40);

            const int   div     = (int) proc.apvts.getRawParameterValue (
                                      FxBenchProcessor::divParamId (slot))->load();
            const float syncSec = dspTime (0.0f, true, div, 120.0f);
            const float jumpOn  = std::abs (std::log (syncSec / freeSec));

            /* And back off again - the knob must land on the same sound. */
            sync->setToggleState (false, juce::sendNotificationSync);
            pump (40);

            const float backNorm = proc.apvts.getRawParameterValue (
                                       FxBenchProcessor::fxParamId (slot))->load();
            const float backSec  = dspTime (backNorm, false, (int) DIV_1_4, 120.0f);
            const float jumpOff  = std::abs (std::log (backSec / syncSec));

            const bool ok = (jumpOn <= halfStep) && (jumpOff <= 0.01f);

            std::printf ("  %.2f -> %-6s  %7.1f ms -> %7.1f ms -> %7.1f ms  %s\n",
                         norm, g_aDivName[div],
                         freeSec * 1000.0f, syncSec * 1000.0f, backSec * 1000.0f,
                         ok ? "ok" : "FAIL");

            if (jumpOn > halfStep)
            {
                std::puts ("      -> sync on did not quantise to the nearest division");
                failures++;
            }

            if (jumpOff > 0.01f)
            {
                std::puts ("      -> sync off did not reproduce the division");
                failures++;
            }
        }
    }

    // ---- the readout is not the raw normalised value --------------------------
    {
        std::printf ("\nreadout : delay time 0.00 = %s, 0.50 = %s, 1.00 = %s\n",
                     FxUnits::format (FX_DELAY_M, FX_DELAYM_P_TIME, 0.0f, 120.0f).toRawUTF8(),
                     FxUnits::format (FX_DELAY_M, FX_DELAYM_P_TIME, 0.5f, 120.0f).toRawUTF8(),
                     FxUnits::format (FX_DELAY_M, FX_DELAYM_P_TIME, 1.0f, 120.0f).toRawUTF8());
        std::printf ("          amp gain   0.50 = %s     od drive 0.70 = %s\n",
                     FxUnits::format (FX_AMP_M, FX_AMPM_P_GAIN, 0.5f, 120.0f).toRawUTF8(),
                     FxUnits::format (FX_OVERDRIVE_M, FX_ODM_P_DRIVE, 0.7f, 120.0f).toRawUTF8());
    }

    editor.reset();

    std::printf ("\n%s\n", failures == 0 ? "panel behaves as specified" : "FAILURES ABOVE");
    return failures == 0 ? 0 : 1;
}
