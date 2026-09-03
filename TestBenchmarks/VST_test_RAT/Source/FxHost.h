/*
    FxHost - the whole bridge between JUCE and the pedal's effect code.

    There is deliberately very little here. The firmware's effects need no grid,
    no mixer and no protocol: an effect is a function that takes an FX_CTX and
    an array of plane pointers. So this file is mostly bookkeeping around

        g_aFxEntry[fx].pfProcess(&ctx, apPlane, nFrames);

    Two things it does that are worth knowing about:

    BLOCK SIZE.  The firmware processes exactly AUDIO_BLOCK_FRAMES frames at a
    time and several effects derive their smoothing coefficients from that. The
    host hands us whatever it likes, so process() chops it into 64-frame blocks.

    MONO EFFECTS RUN TWICE.  A mono-only effect is one plane wide. Rather than
    collapsing the input to mono, it is run as TWO independent instances - plane
    0 on the left, plane 1 on the right - each with its own state. That is
    exactly what the pedal does in its four-mono topology, so a mono algorithm
    auditioned here behaves the way it will on the hardware.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

extern "C" {
#include "fx_defs.h"
#include "audio_cfg.h"
#include "Effects/fx_common.h"
}


namespace FxHost
{
    /** Number of selectable effects. 11 concepts x mono and stereo. */
    int          effectCount();

    /** "Delay  -  stereo" */
    juce::String effectName (int fx);

    /** Parameters this effect actually uses. The rest of the eight are dead. */
    int          paramCount (int fx);

    juce::String paramName  (int fx, int p);

    /** TRUE when the effect will accept a note division instead of a raw value. */
    bool         paramSyncable (int fx, int p);

    /** TRUE when the effect reads this as a small integer, not a continuum. */
    bool         paramStepped  (int fx, int p);

    bool         isStereo   (int fx);

    /** FALSE for the seven concepts still stubbed out in fx_stubs.c. */
    bool         isImplemented (int fx);

    int          divisionCount();
    juce::String divisionName (int d);

    /** The rate the effect code was compiled for. Not negotiable at runtime. */
    double       engineSampleRate();
}


/**
    One instance of one effect, driven at the host's block size.
*/
class FxEngine
{
public:
    FxEngine();

    /** Clears every plane and the current effect's state. */
    void reset();

    /** Selecting a different effect resets it, so it never starts mid-tail. */
    void setEffect (int fx);

    void setParam (int p, float value, bool sync, int division);

    void setTempo (float bpm, int beatsPerBar, int beatUnit);

    /** In place, stereo, any number of frames. */
    void process (float* left, float* right, int numSamples);

private:
    void processOneBlock (float* left, float* right, int numFrames);

    int     currentFx = -1;

    FX_PARAM params[FX_PARAM_QTY] {};
    TEMPO    tempo {};

    /* The engine's planes. Only the first two are used here - the pedal has
       four because it has four channels, not because an effect needs them. */
    FLOAT32  planes[CHAIN_MAX_WIDTH][AUDIO_BLOCK_FRAMES] {};

    juce::uint32 barFrames = 1;
};
