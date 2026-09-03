/*
    A console host that loads the built .vst3 the way FL Studio will.

    "It compiled" and "it loads" are different claims. This target makes the
    second one testable without opening a DAW: it scans the bundle, instantiates
    the processor, walks every effect, and pushes noise through each one.

    It reports CHANGED SAMPLES as well as level, because an effect that quietly
    passes audio through unmodified measures as a perfectly healthy out/in of
    1.000 - which is exactly the bug this test was written to catch.

    Usage:  FxBenchLoadTest "<path to FXcursion FX Bench.vst3>"
*/

#include <juce_audio_processors/juce_audio_processors.h>

#include "FxUnits.h"

extern "C" {
#include "fx_defs.h"
#include "Effects/fx_common.h"
#include "Effects/fx_amp.h"
#include "Effects/fx_delay.h"
#include "Effects/fx_overdrive.h"
#include "Effects/fx_tremolo.h"
}

#include <cmath>


/*
    The knob readout is only worth having if it is the number the DSP is
    actually using. FxUnits restates the mapping - it does not call into the
    effect - so the two can drift the moment somebody retunes a range.

    This runs both sides for the same normalised input and compares them.
*/
static int checkDisplayMatchesDsp()
{
    int bad = 0;

    TEMPO tempo {};
    tempo.fBpm         = 120.0f;
    tempo.nBeatsPerBar = 4U;
    tempo.nBeatUnit    = 4U;

    const float probes[] = { 0.0f, 0.25f, 0.5f, 0.7f, 1.0f };

    auto compare = [&bad] (const char* what, float dsp, float ui)
    {
        const float tol = juce::jmax (1.0e-4f, std::abs (dsp) * 1.0e-4f);

        if (std::abs (dsp - ui) > tol)
        {
            std::printf ("  FAIL %-22s dsp %.6f  ui %.6f\n", what, dsp, ui);
            bad++;
        }
    };

    for (float v : probes)
    {
        FX_PARAM prm {};
        prm.fValue    = v;
        prm.bSync     = (U8) FALSE;
        prm.eDivision = (U8) DIV_1_4;

        compare ("delay time (exp)",
                 FxParam_TimeSec (&prm, &tempo, DELAY_TIME_MIN_SEC, DELAY_TIME_MAX_SEC),
                 FxUnits::physical (FxUnits::describeAt (FX_DELAY_M, FX_DELAYM_P_TIME, 120.0f), v));

        compare ("delay tone (exp)",
                 FxParam_Exp (&prm, DELAY_TONE_MIN_HZ, DELAY_TONE_MAX_HZ),
                 FxUnits::physical (FxUnits::describeAt (FX_DELAY_M, FX_DELAYM_P_TONE, 120.0f), v));

        compare ("delay feedback (lin)",
                 FxParam_Lin (&prm, 0.0f, DELAY_FEEDBACK_MAX),
                 FxUnits::physical (FxUnits::describeAt (FX_DELAY_M, FX_DELAYM_P_FEEDBACK, 120.0f), v));

        compare ("amp gain (lin)",
                 FxParam_Lin (&prm, 0.0f, AMP_GAIN_MAX),
                 FxUnits::physical (FxUnits::describeAt (FX_AMP_M, FX_AMPM_P_GAIN, 120.0f), v));

        compare ("overdrive drive (exp)",
                 FxParam_Exp (&prm, OD_DRIVE_MIN, OD_DRIVE_MAX),
                 FxUnits::physical (FxUnits::describeAt (FX_OVERDRIVE_M, FX_ODM_P_DRIVE, 120.0f), v));

        compare ("overdrive bias (lin)",
                 FxParam_Lin (&prm, OD_BIAS_MIN, OD_BIAS_MAX),
                 FxUnits::physical (FxUnits::describeAt (FX_OVERDRIVE_M, FX_ODM_P_BIAS, 120.0f), v));

        compare ("tremolo rate (exp)",
                 FxParam_RateHz (&prm, &tempo, TREM_RATE_MIN_HZ, TREM_RATE_MAX_HZ),
                 FxUnits::physical (FxUnits::describeAt (FX_TREMOLO_M, FX_TREMOLOM_P_RATE, 120.0f), v));
    }

    /* And the synced path, where fValue is ignored and the division decides.
       1/4 at 120 BPM is half a second. */
    {
        FX_PARAM prm {};
        prm.fValue    = 0.123f;             // deliberately not the answer
        prm.bSync     = (U8) TRUE;
        prm.eDivision = (U8) DIV_1_4;

        const float dsp = FxParam_TimeSec (&prm, &tempo,
                                           DELAY_TIME_MIN_SEC, DELAY_TIME_MAX_SEC);

        std::printf ("  synced 1/4 @120 = %.3f s, shown as \"%s\"\n",
                     dsp,
                     FxUnits::formatSynced (FX_DELAY_M, FX_DELAYM_P_TIME,
                                            (int) DIV_1_4, 120.0f).toRawUTF8());

        if (std::abs (dsp - 0.5f) > 1.0e-4f)
        {
            std::puts ("  FAIL synced quarter note is not 500 ms");
            bad++;
        }
    }

    std::printf ("display : %s\n\n",
                 bad == 0 ? "matches the DSP at every probe" : "*** MISMATCH ***");
    return bad;
}


static bool bufferIsFinite (const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (ch, i)))
                return false;

    return true;
}

static void fillNoise (juce::AudioBuffer<float>& b, juce::Random& rng)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            b.setSample (ch, i, rng.nextFloat() * 0.4f - 0.2f);
}

static double absSum (const juce::AudioBuffer<float>& b)
{
    double s = 0.0;

    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            s += std::abs (b.getSample (ch, i));

    return s;
}


int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    int failures = checkDisplayMatchesDsp();

    if (argc < 2)
    {
        std::puts ("usage: FxBenchLoadTest <plugin.vst3>");
        return 2;
    }

    /* Braces, not parentheses: with parentheses this is a function
       declaration, not a variable. */
    const juce::File bundle { juce::String (argv[1]) };

    if (! bundle.exists())
    {
        std::printf ("not found: %s\n", bundle.getFullPathName().toRawUTF8());
        return 2;
    }

    juce::VST3PluginFormat                    format;
    juce::OwnedArray<juce::PluginDescription> found;

    format.findAllTypesForFile (found, bundle.getFullPathName());

    if (found.isEmpty())
    {
        std::puts ("FAIL: the bundle exposes no plugin");
        return 1;
    }

    std::printf ("found   : %s  (%s)\n",
                 found[0]->name.toRawUTF8(),
                 found[0]->manufacturerName.toRawUTF8());

    juce::String error;
    std::unique_ptr<juce::AudioPluginInstance> plugin (
        format.createInstanceFromDescription (*found[0], 48000.0, 512, error));

    if (plugin == nullptr)
    {
        std::printf ("FAIL: could not instantiate - %s\n", error.toRawUTF8());
        return 1;
    }

    plugin->setPlayConfigDetails (2, 2, 48000.0, 512);
    plugin->prepareToPlay (48000.0, 512);

    std::printf ("params  : %d\n", plugin->getParameters().size());
    std::printf ("io      : %d in, %d out\n",
                 plugin->getTotalNumInputChannels(),
                 plugin->getTotalNumOutputChannels());

    auto& params = plugin->getParameters();

    if (params.isEmpty())
    {
        std::puts ("FAIL: no parameters");
        return 1;
    }

    auto* fxParam     = params[0];
    auto* bypassParam = plugin->getBypassParameter();
    const int fxCount = fxParam->getNumSteps();

    std::printf ("effects : %d\n", fxCount);
    std::printf ("bypass  : %s\n",
                 bypassParam != nullptr ? bypassParam->getName (24).toRawUTF8() : "none");

    if (bypassParam != nullptr)
        bypassParam->setValueNotifyingHost (0.0f);

    // ---- sanity: does ANY parameter change the audio at all? -----------------
    {
        juce::Random rng (99);
        juce::AudioBuffer<float> a (2, 512), b (2, 512);
        juce::MidiBuffer midi;

        fxParam->setValueNotifyingHost (0.0f);      // Amp, mono

        /* Amp maps its gain parameter linearly onto 0..2, so these two settings
           must not produce the same output. If they do, the audio is not
           reaching the effect at all. */
        params[1]->setValueNotifyingHost (0.1f);
        fillNoise (a, rng);
        for (int i = 0; i < 4; ++i) plugin->processBlock (a, midi);

        rng.setSeed (99);
        params[1]->setValueNotifyingHost (1.0f);
        fillNoise (b, rng);
        for (int i = 0; i < 4; ++i) plugin->processBlock (b, midi);

        std::printf ("gain A/B: %.4f vs %.4f  ->  %s\n\n",
                     absSum (a), absSum (b),
                     (std::abs (absSum (a) - absSum (b)) > 1.0)
                         ? "parameters reach the audio"
                         : "*** PARAMETERS DO NOT REACH THE AUDIO ***");
    }

    juce::Random rng (1234);

    for (int fx = 0; fx < fxCount; ++fx)
    {
        fxParam->setValueNotifyingHost (fxCount > 1 ? (float) fx / (float) (fxCount - 1) : 0.0f);

        /* Knobs to 0.7, sync switches OFF. Forcing sync on would make several
           effects read a note division instead of the knob, which is a
           different test.

           Only the knobs and the sync switches are touched. Sweeping every
           parameter index would also hit the bypass switch that the VST3
           wrapper adds on the plugin's behalf, and a bypassed plugin passes
           audio through untouched - which reads as a perfectly healthy
           out/in of 1.000. */
        for (int p = 1; p < params.size(); ++p)
        {
            if (params[p] == bypassParam)
                continue;

            const juce::String name = params[p]->getName (16);

            if (name.startsWithIgnoreCase ("Param"))
                params[p]->setValueNotifyingHost (0.7f);
            else if (name.startsWithIgnoreCase ("Sync"))
                params[p]->setValueNotifyingHost (0.0f);
        }

        juce::AudioBuffer<float> buf (2, 512);
        juce::AudioBuffer<float> ref (2, 512);
        juce::MidiBuffer         midi;

        /* Warm up with FRESH noise each time, never by feeding the output back
           in. Processing one buffer eight times compounds the effect: a delay
           at 0.7 mix has a dry gain of 0.3, and 0.3^8 is silence, which looks
           like a broken effect rather than a broken measurement. */
        for (int block = 0; block < 8; ++block)
        {
            fillNoise (buf, rng);
            plugin->processBlock (buf, midi);
        }

        fillNoise (buf, rng);
        ref.makeCopyOf (buf);

        const double inSum = absSum (buf);

        plugin->processBlock (buf, midi);

        int changed = 0;

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
                if (std::abs (buf.getSample (ch, i) - ref.getSample (ch, i)) > 1.0e-7f)
                    changed++;

        const double outSum = absSum (buf);
        const bool   finite = bufferIsFinite (buf);
        const bool   alive  = outSum > 1.0e-6;

        std::printf ("  %-26s %s  out/in %6.3f   changed %4d/1024\n",
                     fxParam->getCurrentValueAsText().toRawUTF8(),
                     (finite && alive) ? "ok  " : "FAIL",
                     inSum > 0.0 ? outSum / inSum : 0.0,
                     changed);

        if (! finite) { std::puts ("      -> produced NaN or Inf"); failures++; }
        if (! alive)  { std::puts ("      -> output is silent");    failures++; }
    }

    // ---- the editor ----------------------------------------------------------
    /* The panel is the one part of this plugin with no other coverage, and a
       throw or a null dereference in its constructor shows up in a DAW as
       "the plugin crashed on open". Building it here at least proves it
       constructs, lays out and tears down. */
    {
        if (auto* editor = plugin->createEditorIfNeeded())
        {
            editor->setBounds (0, 0, editor->getWidth(), editor->getHeight());

            std::printf ("\neditor  : constructed, %d x %d\n",
                         editor->getWidth(), editor->getHeight());

            if (editor->getWidth() <= 0 || editor->getHeight() <= 0)
            {
                std::puts ("FAIL: editor has no size");
                failures++;
            }

            plugin->editorBeingDeleted (editor);
            delete editor;
        }
        else
        {
            std::puts ("\nFAIL: the plugin refused to make an editor");
            failures++;
        }
    }

    plugin->releaseResources();
    plugin.reset();

    std::printf ("\n%s\n", failures == 0 ? "all effects loaded and produced audio"
                                         : "FAILURES ABOVE");
    return failures == 0 ? 0 : 1;
}
