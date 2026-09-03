#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "FxHost.h"


class FxBenchProcessor : public juce::AudioProcessor
{
public:
    FxBenchProcessor();
    ~FxBenchProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                         { return true; }

    const juce::String getName() const override             { return "FXcursion FX Bench"; }
    bool acceptsMidi() const override                       { return false; }
    bool producesMidi() const override                      { return false; }
    bool isMidiEffect() const override                      { return false; }
    double getTailLengthSeconds() const override            { return 5.0; }

    int getNumPrograms() override                           { return 1; }
    int getCurrentProgram() override                        { return 0; }
    void setCurrentProgram (int) override                   {}
    const juce::String getProgramName (int) override        { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    /** The host's rate. The effect code is compiled for 48 kHz and cannot be
        told otherwise, so the editor warns when these disagree. */
    double hostSampleRate() const noexcept                  { return currentSampleRate; }

    /** What the engine is actually being fed, for the editor's tempo readout. */
    float  effectiveBpm() const noexcept                    { return lastBpm; }

    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

    static juce::String fxParamId    (int i) { return "p"  + juce::String (i); }
    static juce::String syncParamId  (int i) { return "s"  + juce::String (i); }
    static juce::String divParamId   (int i) { return "d"  + juce::String (i); }

private:
    FxEngine engine;

    double currentSampleRate = 48000.0;
    float  lastBpm           = 120.0f;

    std::atomic<float>* pFx        = nullptr;
    std::atomic<float>* pFollow    = nullptr;
    std::atomic<float>* pBpm       = nullptr;
    std::atomic<float>* pSigNum    = nullptr;
    std::atomic<float>* pSigDen    = nullptr;
    std::atomic<float>* pValue[FX_PARAM_QTY] {};
    std::atomic<float>* pSync [FX_PARAM_QTY] {};
    std::atomic<float>* pDiv  [FX_PARAM_QTY] {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxBenchProcessor)
};
