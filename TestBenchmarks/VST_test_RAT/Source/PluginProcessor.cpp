#include "PluginProcessor.h"
#include "PluginEditor.h"


static const int kBeatUnits[] = { 1, 2, 4, 8, 16 };


juce::AudioProcessorValueTreeState::ParameterLayout FxBenchProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::StringArray fxNames;
    for (int i = 0; i < FxHost::effectCount(); ++i)
        fxNames.add (FxHost::effectName (i));

    juce::StringArray divNames;
    for (int i = 0; i < FxHost::divisionCount(); ++i)
        divNames.add (FxHost::divisionName (i));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
                    juce::ParameterID { "fx", 1 }, "Effect", fxNames, 0));

    /* Eight of everything, always. The host's parameter list cannot change
       shape when the effect changes, so the slots are generic and the editor
       is what knows which of them mean anything right now. */
    for (int i = 0; i < (int) FX_PARAM_QTY; ++i)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
                        juce::ParameterID { fxParamId (i), 1 },
                        "Param " + juce::String (i + 1),
                        juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        layout.add (std::make_unique<juce::AudioParameterBool> (
                        juce::ParameterID { syncParamId (i), 1 },
                        "Sync " + juce::String (i + 1), false));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
                        juce::ParameterID { divParamId (i), 1 },
                        "Division " + juce::String (i + 1), divNames, (int) DIV_1_4));
    }

    layout.add (std::make_unique<juce::AudioParameterBool> (
                    juce::ParameterID { "followHost", 1 }, "Follow host tempo", true));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
                    juce::ParameterID { "bpm", 1 }, "BPM",
                    juce::NormalisableRange<float> (20.0f, 400.0f, 0.1f), 120.0f));

    layout.add (std::make_unique<juce::AudioParameterInt> (
                    juce::ParameterID { "signum", 1 }, "Beats per bar", 1, 32, 4));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
                    juce::ParameterID { "sigden", 1 }, "Beat unit",
                    juce::StringArray { "1", "2", "4", "8", "16" }, 2));

    return layout;
}


//==============================================================================

FxBenchProcessor::FxBenchProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "state", makeLayout())
{
    pFx     = apvts.getRawParameterValue ("fx");
    pFollow = apvts.getRawParameterValue ("followHost");
    pBpm    = apvts.getRawParameterValue ("bpm");
    pSigNum = apvts.getRawParameterValue ("signum");
    pSigDen = apvts.getRawParameterValue ("sigden");

    for (int i = 0; i < (int) FX_PARAM_QTY; ++i)
    {
        pValue[i] = apvts.getRawParameterValue (fxParamId (i));
        pSync [i] = apvts.getRawParameterValue (syncParamId (i));
        pDiv  [i] = apvts.getRawParameterValue (divParamId (i));
    }
}

void FxBenchProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    engine.reset();
}

bool FxBenchProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    /* Stereo in, stereo out, and nothing else. A mono effect is auditioned as
       two independent instances rather than by folding the input down - see
       FxHost.h. */
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void FxBenchProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples == 0 || buffer.getNumChannels() < 2)
        return;

    // ---- tempo ---------------------------------------------------------------
    float bpm         = pBpm->load();
    int   beatsPerBar = (int) pSigNum->load();
    int   beatUnit    = kBeatUnits[juce::jlimit (0, 4, (int) pSigDen->load())];

    if (pFollow->load() > 0.5f)
    {
        if (auto* ph = getPlayHead())
        {
            if (auto pos = ph->getPosition())
            {
                if (auto hostBpm = pos->getBpm())
                    bpm = (float) *hostBpm;

                if (auto sig = pos->getTimeSignature())
                {
                    beatsPerBar = sig->numerator;
                    beatUnit    = sig->denominator;
                }
            }
        }
    }

    lastBpm = bpm;
    engine.setTempo (bpm, beatsPerBar, beatUnit);

    // ---- effect and parameters ----------------------------------------------
    engine.setEffect ((int) pFx->load());

    for (int i = 0; i < (int) FX_PARAM_QTY; ++i)
        engine.setParam (i,
                         pValue[i]->load(),
                         pSync[i]->load() > 0.5f,
                         (int) pDiv[i]->load());

    // ---- audio ---------------------------------------------------------------
    engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
}

juce::AudioProcessorEditor* FxBenchProcessor::createEditor()
{
    return new FxBenchEditor (*this);
}

void FxBenchProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void FxBenchProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}


//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxBenchProcessor();
}
