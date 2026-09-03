/*
    Turning a normalised parameter back into something a player can read.

    A parameter is stored 0..1. That is right on the wire and useless on a
    screen: "Time 0.700" says nothing, "470 ms" says everything. Recovering the
    second from the first needs the range and the mapping the effect actually
    applies.

    Those ranges are NOT duplicated here. They come from the effect headers -
    DELAY_TIME_MIN_SEC, OD_DRIVE_MAX and the rest were moved out of the .c files
    for exactly this reason - so retuning an effect updates the display with it.
    What this file adds is only which mapping and which unit go with which
    parameter, which is the one thing the firmware does not record.

    The arithmetic mirrors fx_common.c exactly:

        Lin   lo + v * (hi - lo)
        Exp   lo * (hi / lo) ^ v

    so the number under the knob is the number the DSP is using, not an
    approximation of it.
*/

#pragma once

#include <juce_core/juce_core.h>


namespace FxUnits
{
    enum class Interp { Lin, Exp };

    enum class Unit
    {
        Percent,        // 0..100 %
        Decibel,        // a LINEAR gain, converted to dB with a -inf floor
        Db,             // already in dB - a threshold, a make-up trim
        Ratio,          // a compression ratio, "4.0:1"
        Times,          // a multiplier, "x12.3"
        Seconds,        // ms below a second, s above
        Hertz,          // Hz, kHz above 1000
        Degrees,        // a phase offset
        Pan,            // L / C / R
        Stages,         // quantised to 2, 4, 6 or 8 - see FX_PF_STEPPED
        Raw             // a bare number
    };

    struct Desc
    {
        Interp interp = Interp::Lin;
        Unit   unit   = Unit::Percent;
        float  lo     = 0.0f;
        float  hi     = 1.0f;
    };

    /** What parameter p of effect fx means. Percent 0..1 for anything unknown,
        which is what the seven stubbed effects get. */
    Desc describe (int fx, int p);

    /** The same, but with a syncable parameter's range narrowed to exactly what
        the note divisions span at this tempo - which is what the DSP now does.
        See TempoSpanSec in fx_common.c. */
    Desc describeAt (int fx, int p, float bpm);

    /** TRUE when the effect declares this parameter tempo-drivable. */
    bool isSyncable (int fx, int p);

    /** The division this free value sits closest to, e.g. "1/8" or "~ 1/8".
        Empty when the tempo cannot drive the parameter. */
    juce::String nearestDivision (int fx, int p, float norm, float bpm);

    /** The same as a NOTE_DIV index - what a free value quantises to when sync
        is switched on. */
    int nearestDivisionIndex (int fx, int p, float norm, float bpm);

    /** The knob position that reproduces a division exactly, for switching sync
        back off without the value jumping. */
    float normForDivision (int fx, int p, int division, float bpm);

    /** The physical value for a normalised knob position. */
    float physical (const Desc& d, float norm);

    /** Formatted for the middle of a knob, e.g. "470 ms" or "-6.0 dB". */
    juce::String format (int fx, int p, float norm, float bpm);

    /** TRUE when a synced division resolves to something worth displaying -
        a time or a rate. Everything else ignores the tempo. */
    bool syncResolves (int fx, int p);

    /** What a division works out to at this tempo, e.g. "500 ms" or "2.00 Hz".
        Empty when the parameter is not one the tempo can drive. */
    juce::String formatSynced (int fx, int p, int division, float bpm);
}
