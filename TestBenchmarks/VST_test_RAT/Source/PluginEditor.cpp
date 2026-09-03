#include "PluginEditor.h"
#include "FxUnits.h"


namespace Col
{
    const juce::Colour bg      { 0xff191c21 };
    const juce::Colour panel   { 0xff21252c };
    const juce::Colour track   { 0xff2f343d };
    const juce::Colour accent  { 0xff56c8a0 };
    const juce::Colour synced  { 0xffe0a94a };
    const juce::Colour text    { 0xffe6e9ee };
    const juce::Colour dim     { 0xff5b636f };
}

static constexpr int kCellW   = 152;
static constexpr int kCellH   = 158;
static constexpr int kHeaderH = 116;
static constexpr int kMargin  = 12;


//==============================================================================

KnobLookAndFeel::KnobLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, Col::panel);
    setColour (juce::ComboBox::textColourId,       Col::text);
    setColour (juce::ComboBox::outlineColourId,    Col::track);
    setColour (juce::ComboBox::arrowColourId,      Col::dim);
}

void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle,
                                        juce::Slider& slider)
{
    const auto area   = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (5.0f);
    const auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 3.0f;
    const auto centre = area.getCentre();
    const bool on     = slider.isEnabled() && inUse;
    const auto angle  = startAngle + sliderPos * (endAngle - startAngle);
    const auto ring   = on ? (sync ? Col::synced : Col::accent) : Col::track;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (Col::track);
    g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

    if (on)
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
        g.setColour (ring);
        g.strokePath (value, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

        juce::Path pointer;
        pointer.startNewSubPath (0.0f, -radius + 4.0f);
        pointer.lineTo          (0.0f, -radius + 13.0f);
        g.strokePath (pointer, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved,
                                                           juce::PathStrokeType::rounded),
                      juce::AffineTransform::rotation (angle).translated (centre));
    }

    juce::String main, sub;

    if (! on)
    {
        main = "-";
    }
    else if (sync)
    {
        /* Synced: the knob IS the division picker, so it shows the division it
           has snapped to, and underneath what that works out to at the current
           tempo. The effect ignores fValue entirely in this mode. */
        const int div = juce::roundToInt (slider.getValue());

        main = FxHost::divisionName (div);
        sub  = FxUnits::formatSynced (fx, slot, div, bpm);
    }
    else
    {
        /* Free, but the ranges now line up with the divisions, so the nearest
           one is worth showing underneath - the mirror image of what sync mode
           does. "~" means it is between two divisions. */
        main = FxUnits::format         (fx, slot, (float) slider.getValue(), bpm);
        sub  = FxUnits::nearestDivision (fx, slot, (float) slider.getValue(), bpm);
    }

    auto textArea = area;

    if (sub.isNotEmpty())
        textArea = textArea.withTrimmedBottom (area.getHeight() * 0.22f);

    g.setColour (on ? Col::text : Col::dim);
    g.setFont (juce::Font (juce::FontOptions (15.0f)).withStyle (juce::Font::bold));
    g.drawText (main, textArea, juce::Justification::centred, false);

    if (sub.isNotEmpty())
    {
        g.setColour (Col::dim);
        g.setFont (juce::Font (juce::FontOptions (11.0f)));
        g.drawText (sub, area.withTrimmedTop (area.getHeight() * 0.56f),
                    juce::Justification::centred, false);
    }
}


//==============================================================================

ParamCell::ParamCell (FxBenchProcessor& p, int index)
    : proc (p), slot (index)
{
    lnf.slot = slot;

    /* Stable ids so UiTest can find a specific slot's controls without relying
       on the order components happen to be added in. */
    knob.setComponentID       ("knob" + juce::String (slot));
    syncButton.setComponentID ("sync" + juce::String (slot));

    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                              juce::MathConstants<float>::pi * 2.75f, true);
    knob.setLookAndFeel (&lnf);
    addAndMakeVisible (knob);

    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, Col::text);
    nameLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (nameLabel);

    syncButton.setColour (juce::ToggleButton::textColourId, Col::text);
    syncButton.setColour (juce::ToggleButton::tickColourId, Col::synced);
    syncButton.onClick = [this] { onSyncToggled(); };
    addChildComponent (syncButton);          // shown only when the effect allows it

    syncAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                  proc.apvts, FxBenchProcessor::syncParamId (slot), syncButton);

    rebindKnob();
}

ParamCell::~ParamCell()
{
    knob.setLookAndFeel (nullptr);
}

void ParamCell::setBpm (float bpm)
{
    if (! juce::approximatelyEqual (lnf.bpm, bpm))
    {
        lnf.bpm = bpm;

        /* Repaint whichever mode it is in: the free range follows the tempo
           now, so the number under an unsynced knob moves with it too. */
        if (syncable)
            knob.repaint();
    }
}

void ParamCell::onSyncToggled()
{
    /*
     * The free value and the division are two separate parameters, so simply
     * rebinding the knob leaves each of them holding whatever it last had -
     * which is why flipping sync used to recall an old division instead of
     * quantising the setting in front of you.
     *
     * The two ranges now cover the same span, so the setting can be carried
     * across exactly: on, it snaps to the nearest division; off, it lands on
     * the knob position that reproduces that division. Either way the sound
     * does not jump.
     */
    if (inUse && syncable && currentFx >= 0)
    {
        const bool syncOn = syncButton.getToggleState();

        if (syncOn)
        {
            const float norm = proc.apvts.getRawParameterValue (
                                   FxBenchProcessor::fxParamId (slot))->load();
            const int   div  = FxUnits::nearestDivisionIndex (currentFx, slot, norm, lnf.bpm);

            if (auto* prm = proc.apvts.getParameter (FxBenchProcessor::divParamId (slot)))
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) div));
        }
        else
        {
            const int   div  = (int) proc.apvts.getRawParameterValue (
                                   FxBenchProcessor::divParamId (slot))->load();
            const float norm = FxUnits::normForDivision (currentFx, slot, div, lnf.bpm);

            if (auto* prm = proc.apvts.getParameter (FxBenchProcessor::fxParamId (slot)))
                prm->setValueNotifyingHost (norm);
        }
    }

    rebindKnob();
}

void ParamCell::rebindKnob()
{
    const bool syncOn = inUse && syncable && syncButton.getToggleState();

    /* ONLY the attachment is conditional.
     *
     * Rebuilding it needlessly would fight the host over the parameter, so it
     * is rebuilt just when the knob has to drive a different one. Everything
     * below is plain state and must be applied on EVERY call - guarding it
     * behind the same condition is what disabled every knob on the panel: the
     * constructor runs before the effect is known, so inUse is false, and the
     * updateFor that sets it true found the binding already correct and
     * returned before re-enabling anything. */
    if (knobAtt == nullptr || syncOn != boundToDivision)
    {
        /* Bound to the division choice the slider becomes a stepped
           0..DIV_QTY-1 control, which is what makes it snap to 1/4, 1/8. and
           the rest without a separate list to keep in step. */
        knobAtt.reset();

        knobAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                      proc.apvts,
                      syncOn ? FxBenchProcessor::divParamId (slot)
                             : FxBenchProcessor::fxParamId  (slot),
                      knob);

        boundToDivision = syncOn;
    }

    lnf.sync  = syncOn;
    lnf.inUse = inUse;

    knob.setEnabled (inUse);
    knob.repaint();
    repaint();
}

void ParamCell::updateFor (int fx)
{
    currentFx = fx;
    lnf.fx    = fx;

    inUse    = slot < FxHost::paramCount (fx);
    syncable = FxHost::paramSyncable (fx, slot);

    if (inUse)
    {
        auto name = FxHost::paramName (fx, slot).toUpperCase();

        if (FxHost::paramStepped (fx, slot))
            name += " *";           // the effect reads this as a small integer

        nameLabel.setText (name, juce::dontSendNotification);
        nameLabel.setColour (juce::Label::textColourId, Col::text);
    }
    else
    {
        nameLabel.setText ("-", juce::dontSendNotification);
        nameLabel.setColour (juce::Label::textColourId, Col::dim);
    }

    /* Hidden, not merely greyed. A sync switch on a parameter the effect will
       never read as a note division is noise on the panel. */
    syncButton.setVisible (inUse && syncable);

    rebindKnob();
}

void ParamCell::paint (juce::Graphics& g)
{
    g.setColour (Col::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (3.0f), 6.0f);
}

void ParamCell::resized()
{
    auto r = getLocalBounds().reduced (8);

    knob.setBounds (r.removeFromTop (96));
    r.removeFromTop (2);
    nameLabel.setBounds (r.removeFromTop (20));
    r.removeFromTop (2);

    /* Centred rather than left aligned, because it is now the only thing on
       this row. */
    syncButton.setBounds (r.removeFromTop (22).withSizeKeepingCentre (66, 22));
}


//==============================================================================

FxBenchEditor::FxBenchEditor (FxBenchProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < FxHost::effectCount(); ++i)
        fxBox.addItem (FxHost::effectName (i), i + 1);

    fxBox.setColour (juce::ComboBox::backgroundColourId, Col::panel);
    fxBox.setColour (juce::ComboBox::textColourId,       Col::text);
    fxBox.setColour (juce::ComboBox::outlineColourId,    Col::track);
    addAndMakeVisible (fxBox);

    rateLabel.setJustificationType (juce::Justification::centredRight);
    rateLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (rateLabel);

    followButton.setColour (juce::ToggleButton::textColourId, Col::text);
    followButton.setColour (juce::ToggleButton::tickColourId, Col::accent);
    addAndMakeVisible (followButton);

    bpmSlider.setSliderStyle (juce::Slider::LinearBar);
    bpmSlider.setColour (juce::Slider::trackColourId,       Col::panel);
    bpmSlider.setColour (juce::Slider::textBoxTextColourId, Col::text);
    addAndMakeVisible (bpmSlider);
    bpmLabel.setColour (juce::Label::textColourId, Col::dim);
    bpmLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (bpmLabel);

    sigNumSlider.setSliderStyle (juce::Slider::LinearBar);
    sigNumSlider.setColour (juce::Slider::trackColourId,       Col::panel);
    sigNumSlider.setColour (juce::Slider::textBoxTextColourId, Col::text);
    addAndMakeVisible (sigNumSlider);

    sigDenBox.addItemList ({ "1", "2", "4", "8", "16" }, 1);
    sigDenBox.setColour (juce::ComboBox::backgroundColourId, Col::panel);
    sigDenBox.setColour (juce::ComboBox::textColourId,       Col::text);
    sigDenBox.setColour (juce::ComboBox::outlineColourId,    Col::track);
    addAndMakeVisible (sigDenBox);

    sigLabel.setColour (juce::Label::textColourId, Col::dim);
    sigLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (sigLabel);

    for (int i = 0; i < (int) FX_PARAM_QTY; ++i)
        addAndMakeVisible (cells.add (new ParamCell (proc, i)));

    auto& s = proc.apvts;
    fxAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "fx",         fxBox);
    followAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>   (s, "followHost", followButton);
    bpmAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (s, "bpm",        bpmSlider);
    sigNumAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (s, "signum",     sigNumSlider);
    sigDenAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, "sigden",     sigDenBox);

    setSize (kMargin * 2 + kCellW * 4, kHeaderH + kCellH * 2 + kMargin);

    startTimerHz (20);
    timerCallback();
}

void FxBenchEditor::timerCallback()
{
    const int fx = fxBox.getSelectedItemIndex();

    if (fx != lastFx)
    {
        lastFx = fx;

        for (auto* c : cells)
            c->updateFor (fx);
    }

    const float bpm = proc.effectiveBpm();

    if (! juce::approximatelyEqual (bpm, lastBpm))
    {
        lastBpm = bpm;

        for (auto* c : cells)
            c->setBpm (bpm);
    }

    const double sr = proc.hostSampleRate();

    if (std::abs (sr - FxHost::engineSampleRate()) < 1.0)
    {
        rateLabel.setText ("48 kHz", juce::dontSendNotification);
        rateLabel.setColour (juce::Label::textColourId, Col::dim);
    }
    else
    {
        /* The effect code has AUDIO_SAMPLE_RATE_HZ baked in - delay lengths,
           filter coefficients and LFO rates all derive from it - so at any
           other rate everything is out by the ratio. Resampling here would
           colour exactly what you are trying to judge, so the bench says so
           instead and asks you to set the host to 48 kHz. */
        rateLabel.setText (juce::String (sr / 1000.0, 1) + " kHz  -  set host to 48 kHz",
                           juce::dontSendNotification);
        rateLabel.setColour (juce::Label::textColourId, Col::synced);
    }

    const bool manual = ! followButton.getToggleState();
    bpmSlider.setEnabled (manual);
    sigNumSlider.setEnabled (manual);
    sigDenBox.setEnabled (manual);

    if (! manual)
        bpmSlider.setValue (bpm, juce::dontSendNotification);
}

void FxBenchEditor::paint (juce::Graphics& g)
{
    g.fillAll (Col::bg);

    g.setColour (Col::text);
    g.setFont (juce::Font (juce::FontOptions (17.0f)).withStyle (juce::Font::bold));
    g.drawText ("FXcursion  FX Bench", kMargin, 8, 300, 22, juce::Justification::centredLeft);

    g.setColour (Col::dim);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("firmware effect code, unmodified",
                kMargin, 28, 300, 16, juce::Justification::centredLeft);
}

void FxBenchEditor::resized()
{
    auto r = getLocalBounds().reduced (kMargin, 0);

    auto top = r.removeFromTop (kHeaderH);
    top.removeFromTop (8);

    rateLabel.setBounds (top.removeFromTop (22).removeFromRight (260));
    top.removeFromTop (14);

    fxBox.setBounds (top.removeFromTop (28));
    top.removeFromTop (8);

    auto tempoRow = top.removeFromTop (24);
    followButton.setBounds (tempoRow.removeFromLeft (150));
    tempoRow.removeFromLeft (10);
    bpmLabel.setBounds  (tempoRow.removeFromLeft (34));
    bpmSlider.setBounds (tempoRow.removeFromLeft (90));
    tempoRow.removeFromLeft (16);
    sigLabel.setBounds     (tempoRow.removeFromLeft (28));
    sigNumSlider.setBounds (tempoRow.removeFromLeft (54));
    tempoRow.removeFromLeft (6);
    sigDenBox.setBounds    (tempoRow.removeFromLeft (58));

    for (int i = 0; i < cells.size(); ++i)
    {
        const int col = i % 4;
        const int row = i / 4;

        cells[i]->setBounds (r.getX() + col * kCellW,
                             r.getY() + row * kCellH,
                             kCellW, kCellH);
    }
}
