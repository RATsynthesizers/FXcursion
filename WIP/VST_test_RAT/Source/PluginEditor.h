#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"


/**
    Draws the knob and puts a READABLE value in the middle of it.

    Not the normalised 0..1 the engine stores - "Time 0.700" tells a player
    nothing - but what that position actually means: 470 ms, -6.0 dB, 2.40 kHz.
    See FxUnits.

    When a parameter is switched to tempo sync the knob stops being a continuum
    and becomes the division picker itself, so the centre shows the division and
    a second line shows what it works out to at the current tempo.
*/
class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    /* Set by the owning cell; the text is computed at paint time because the
       value changes continuously while the knob is being dragged. */
    int   fx    = -1;
    int   slot  = 0;
    bool  sync  = false;
    bool  inUse = false;
    float bpm   = 120.0f;
};


/**
    One parameter slot: knob, name, and a sync switch when the effect says the
    parameter can be driven by tempo.

    There are always eight of these. An effect that uses fewer leaves the rest
    as inert placeholders rather than rearranging the panel, so the layout does
    not move around as you audition different effects.
*/
class ParamCell : public juce::Component
{
public:
    ParamCell (FxBenchProcessor& p, int index);
    ~ParamCell() override;

    void resized() override;
    void paint (juce::Graphics&) override;

    /** Re-label, re-range and show or hide the sync switch for this effect. */
    void updateFor (int fx);

    void setBpm (float bpm);

private:
    /** Rebinds the knob to either the value or the division parameter. */
    void rebindKnob();

    /** Carries the current setting across when the sync switch is clicked, so
        the knob quantises instead of recalling whatever it held before. */
    void onSyncToggled();

    FxBenchProcessor& proc;
    const int         slot;

    juce::Slider       knob;
    juce::Label        nameLabel;
    juce::ToggleButton syncButton { "sync" };

    KnobLookAndFeel lnf;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAtt;

    int  currentFx = -1;
    bool inUse     = false;
    bool syncable  = false;
    bool boundToDivision = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamCell)
};


class FxBenchEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit FxBenchEditor (FxBenchProcessor&);
    ~FxBenchEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    FxBenchProcessor& proc;

    juce::ComboBox     fxBox;
    juce::Label        rateLabel;
    juce::ToggleButton followButton { "Follow host tempo" };
    juce::Slider       bpmSlider;
    juce::Label        bpmLabel   { {}, "BPM" };
    juce::Slider       sigNumSlider;
    juce::ComboBox     sigDenBox;
    juce::Label        sigLabel   { {}, "Bar" };

    juce::OwnedArray<ParamCell> cells;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> fxAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   followAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   bpmAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   sigNumAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sigDenAtt;

    int   lastFx  = -1;
    float lastBpm = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FxBenchEditor)
};
