#include "FxUnits.h"

extern "C" {
#include "fx_defs.h"
#include "Effects/fx_amp.h"
#include "Effects/fx_compressor.h"
#include "Effects/fx_reverb.h"
#include "Effects/fx_delay.h"
#include "Effects/fx_distortion.h"
#include "Effects/fx_modulation.h"
#include "Effects/fx_phaser.h"
#include "Effects/fx_overdrive.h"
#include "Effects/fx_tremolo.h"
}

#include <cmath>


namespace FxUnits
{
    static Desc lin (Unit u, float lo, float hi) { return { Interp::Lin, u, lo, hi }; }
    static Desc exp (Unit u, float lo, float hi) { return { Interp::Exp, u, lo, hi }; }
    static Desc pct (float hi = 1.0f)            { return { Interp::Lin, Unit::Percent, 0.0f, hi }; }


    Desc describe (int fx, int p)
    {
        switch (fx)
        {
            // ---- Amp ---------------------------------------------------------
            case FX_AMP_M:
                if (p == FX_AMPM_P_GAIN)   return lin (Unit::Decibel, 0.0f, AMP_GAIN_MAX);
                break;

            case FX_AMP_S:
                if (p == FX_AMPS_P_GAIN)   return lin (Unit::Decibel, 0.0f, AMP_GAIN_MAX);
                if (p == FX_AMPS_P_PAN)    return lin (Unit::Pan,     0.0f, 1.0f);
                if (p == FX_AMPS_P_WIDTH)  return lin (Unit::Times,   0.0f, AMP_WIDTH_MAX);
                break;

            // ---- Compressor --------------------------------------------------
            case FX_COMPRESSOR_M:
            case FX_COMPRESSOR_S:
                if (p == FX_COMPM_P_THRESHOLD) return lin (Unit::Db, COMP_THRESH_MIN_DB,
                                                                     COMP_THRESH_MAX_DB);
                if (p == FX_COMPM_P_RATIO)     return exp (Unit::Ratio, COMP_RATIO_MIN,
                                                                        COMP_RATIO_MAX);
                /* Seconds, not milliseconds - the formatter picks the unit, and
                   an attack is the one control where 0.1 ms and 100 ms both
                   have to read clearly. */
                if (p == FX_COMPM_P_ATTACK)    return exp (Unit::Seconds,
                                                           COMP_ATTACK_MIN_MS * 0.001f,
                                                           COMP_ATTACK_MAX_MS * 0.001f);
                if (p == FX_COMPM_P_RELEASE)   return exp (Unit::Seconds,
                                                           COMP_RELEASE_MIN_MS * 0.001f,
                                                           COMP_RELEASE_MAX_MS * 0.001f);
                if (p == FX_COMPM_P_MAKEUP)    return lin (Unit::Db, 0.0f, COMP_MAKEUP_MAX_DB);

                if (fx == FX_COMPRESSOR_S && p == FX_COMPS_P_LINK) return pct();
                break;

            // ---- Reverb ------------------------------------------------------
            case FX_REVERB_M:
            case FX_REVERB_S:
                if (p == FX_REVERBM_P_DECAY)     return exp (Unit::Seconds, REV_DECAY_MIN_SEC,
                                                                            REV_DECAY_MAX_SEC);
                if (p == FX_REVERBM_P_PREDELAY)  return exp (Unit::Seconds, REV_PREDELAY_MIN_SEC,
                                                                            REV_PREDELAY_MAX_SEC);
                if (p == FX_REVERBM_P_DAMPING)   return pct();
                if (p == FX_REVERBM_P_DIFFUSION) return pct (REV_DIFFUSION_MAX);
                if (p == FX_REVERBM_P_MIX)       return pct();

                if (fx == FX_REVERB_S && p == FX_REVERBS_P_WIDTH)
                    return lin (Unit::Times, 0.0f, REV_WIDTH_MAX);
                break;

            // ---- Chorus ------------------------------------------------------
            case FX_CHORUS_M:
            case FX_CHORUS_S:
                if (p == FX_CHORUSM_P_RATE)  return exp (Unit::Hertz, MOD_RATE_MIN_HZ,
                                                                      MOD_RATE_MAX_HZ);
                if (p == FX_CHORUSM_P_DEPTH) return pct();
                if (p == FX_CHORUSM_P_DELAY) return lin (Unit::Seconds,
                                                         CHORUS_DELAY_MIN_MS * 0.001f,
                                                         CHORUS_DELAY_MAX_MS * 0.001f);
                if (p == FX_CHORUSM_P_MIX)   return pct();

                if (fx == FX_CHORUS_S && p == FX_CHORUSS_P_SPREAD) return pct();
                break;

            // ---- Flanger -----------------------------------------------------
            case FX_FLANGER_M:
            case FX_FLANGER_S:
                if (p == FX_FLANGERM_P_RATE)     return exp (Unit::Hertz, MOD_RATE_MIN_HZ,
                                                                          MOD_RATE_MAX_HZ);
                if (p == FX_FLANGERM_P_DEPTH)    return pct();
                if (p == FX_FLANGERM_P_FEEDBACK) return pct (FLANGER_FEEDBACK_MAX);
                if (p == FX_FLANGERM_P_MIX)      return pct();

                if (fx == FX_FLANGER_S && p == FX_FLANGERS_P_SPREAD) return pct();
                break;

            // ---- Vibrato -----------------------------------------------------
            case FX_VIBRATO_M:
            case FX_VIBRATO_S:
                if (p == FX_VIBRATOM_P_RATE)  return exp (Unit::Hertz, MOD_RATE_MIN_HZ,
                                                                       MOD_RATE_MAX_HZ);
                if (p == FX_VIBRATOM_P_DEPTH) return pct();

                if (fx == FX_VIBRATO_S && p == FX_VIBRATOS_P_SPREAD) return pct();
                break;

            // ---- Phaser ------------------------------------------------------
            case FX_PHASER_M:
            case FX_PHASER_S:
                if (p == FX_PHASERM_P_RATE)     return exp (Unit::Hertz, PHASER_RATE_MIN_HZ,
                                                                         PHASER_RATE_MAX_HZ);
                if (p == FX_PHASERM_P_DEPTH)    return pct();
                if (p == FX_PHASERM_P_FEEDBACK) return pct (PHASER_FEEDBACK_MAX);
                if (p == FX_PHASERM_P_STAGES)   return lin (Unit::Stages, 0.0f, 1.0f);
                if (p == FX_PHASERM_P_MIX)      return pct();

                if (fx == FX_PHASER_S && p == FX_PHASERS_P_SPREAD) return pct();
                break;

            // ---- Distortion --------------------------------------------------
            case FX_DISTORTION_M:
            case FX_DISTORTION_S:
                if (p == FX_DISTM_P_DRIVE) return exp (Unit::Times, DIST_DRIVE_MIN,
                                                                    DIST_DRIVE_MAX);
                if (p == FX_DISTM_P_TONE)  return exp (Unit::Hertz, DIST_TONE_MIN_HZ,
                                                                    DIST_TONE_MAX_HZ);
                if (p == FX_DISTM_P_LEVEL) return lin (Unit::Decibel, 0.0f, DIST_LEVEL_MAX);
                if (p == FX_DISTM_P_MIX)   return pct();

                if (fx == FX_DISTORTION_S && p == FX_DISTS_P_SPREAD) return pct();
                break;

            // ---- Delay -------------------------------------------------------
            case FX_DELAY_M:
            case FX_DELAY_S:
                if (p == FX_DELAYM_P_TIME)     return exp (Unit::Seconds,
                                                           DELAY_TIME_MIN_SEC, DELAY_TIME_MAX_SEC);
                if (p == FX_DELAYM_P_FEEDBACK) return pct (DELAY_FEEDBACK_MAX);
                if (p == FX_DELAYM_P_TONE)     return exp (Unit::Hertz,
                                                           DELAY_TONE_MIN_HZ, DELAY_TONE_MAX_HZ);
                if (p == FX_DELAYM_P_MIX)      return pct();

                /* Stereo only, and both are plain 0..1 controls the effect
                   scales itself. */
                if (fx == FX_DELAY_S && p == FX_DELAYS_P_PINGPONG) return pct();
                if (fx == FX_DELAY_S && p == FX_DELAYS_P_SPREAD)   return pct();
                break;

            // ---- Overdrive ---------------------------------------------------
            case FX_OVERDRIVE_M:
            case FX_OVERDRIVE_S:
                if (p == FX_ODM_P_DRIVE) return exp (Unit::Times,   OD_DRIVE_MIN, OD_DRIVE_MAX);
                if (p == FX_ODM_P_BIAS)  return lin (Unit::Raw,     OD_BIAS_MIN,  OD_BIAS_MAX);
                if (p == FX_ODM_P_LEVEL) return lin (Unit::Decibel, 0.0f,         OD_LEVEL_MAX);
                if (p == FX_ODM_P_MIX)   return pct();

                if (fx == FX_OVERDRIVE_S && p == FX_ODS_P_SPREAD) return pct (OD_SPREAD_MAX);
                break;

            // ---- Tremolo -----------------------------------------------------
            case FX_TREMOLO_M:
            case FX_TREMOLO_S:
                if (p == FX_TREMOLOM_P_RATE)  return exp (Unit::Hertz,
                                                          TREM_RATE_MIN_HZ, TREM_RATE_MAX_HZ);
                if (p == FX_TREMOLOM_P_DEPTH) return pct();
                if (p == FX_TREMOLOM_P_SHAPE) return pct();

                /* The effect uses half a cycle of offset, so the full knob is
                   half a period - 180 degrees. */
                if (fx == FX_TREMOLO_S && p == FX_TREMOLOS_P_PHASE)
                    return lin (Unit::Degrees, 0.0f, 180.0f);
                break;

            default:
                break;
        }

        /* Anything not named above - every parameter of the seven stubbed
           effects - has no published range yet, so it stays a percentage. */
        return pct();
    }


    bool isSyncable (int fx, int p)
    {
        if (fx < 0 || fx >= (int) FX_TYPE_QTY || p < 0 || p >= (int) g_aFxDesc[fx].nParamQty)
            return false;

        return (g_aFxDesc[fx].pParam[p].nFlags & (U8) FX_PF_SYNCABLE) != 0U;
    }


    Desc describeAt (int fx, int p, float bpm)
    {
        const Desc base = describe (fx, p);

        /* Only a parameter the tempo can drive gets a tempo-dependent range,
           and this is the same flag the firmware keys off. Delay Tone is in
           hertz too and must NOT move with the project tempo. */
        if (! isSyncable (fx, p))
            return base;

        /* Mirrors TempoSpanSec in fx_common.c: the free knob spans exactly what
           1/32 and 1/1 give at this tempo, clamped by what the effect can
           actually do. */
        const float quarter  = 60.0f / juce::jlimit (20.0f, 400.0f, bpm);
        const float shortest = g_aDivQuarters[DIV_1_32] * quarter;
        const float longest  = g_aDivQuarters[DIV_1_1]  * quarter;

        Desc d = base;

        if (base.unit == Unit::Seconds)
        {
            d.lo = juce::jlimit (base.lo, base.hi, shortest);
            d.hi = juce::jlimit (base.lo, base.hi, longest);
        }
        else if (base.unit == Unit::Hertz && shortest > 0.0f && longest > 0.0f)
        {
            /* A long division is a SLOW rate, so the longest period is the low
               end here. */
            d.lo = juce::jlimit (base.lo, base.hi, 1.0f / longest);
            d.hi = juce::jlimit (base.lo, base.hi, 1.0f / shortest);
        }

        return d;
    }


    /** What one division resolves to, in the parameter's own unit. */
    static float divisionValue (const Desc& base, int division, float bpm)
    {
        const float quarter = 60.0f / juce::jlimit (20.0f, 400.0f, bpm);
        const float period  = g_aDivQuarters[division] * quarter;

        if (base.unit == Unit::Seconds)
            return juce::jlimit (base.lo, base.hi, period);

        return juce::jlimit (base.lo, base.hi, period > 0.0f ? 1.0f / period : base.lo);
    }


    /** Nearest division, and how far off it was in log distance. */
    static int nearestDivisionImpl (int fx, int p, float norm, float bpm, float* pDist)
    {
        const Desc  base  = describe (fx, p);
        const float value = physical (describeAt (fx, p, bpm), norm);

        int   best     = 0;
        float bestDist = 1.0e30f;

        if (value > 0.0f)
        {
            /* Closest in RATIO, not in difference. These are exponential
               quantities: 100 ms is as far from 50 ms as 2 s is from 1 s, and a
               linear comparison would call almost everything 1/1. */
            for (int d = 0; d < (int) DIV_QTY; ++d)
            {
                const float v = divisionValue (base, d, bpm);

                if (v <= 0.0f)
                    continue;

                const float dist = std::abs (std::log (value / v));

                if (dist < bestDist)
                {
                    bestDist = dist;
                    best     = d;
                }
            }
        }

        if (pDist != nullptr)
            *pDist = bestDist;

        return best;
    }


    int nearestDivisionIndex (int fx, int p, float norm, float bpm)
    {
        if (! isSyncable (fx, p))
            return (int) DIV_1_4;

        return nearestDivisionImpl (fx, p, norm, bpm, nullptr);
    }


    juce::String nearestDivision (int fx, int p, float norm, float bpm)
    {
        if (! isSyncable (fx, p))
            return {};

        float dist = 0.0f;
        const int  best = nearestDivisionImpl (fx, p, norm, bpm, &dist);

        if (dist > 1.0e29f)
            return {};

        /* Within a quarter percent counts as landing on it. */
        const juce::String name (g_aDivName[best]);

        return (dist < 0.0025f) ? name : ("~ " + name);
    }


    float normForDivision (int fx, int p, int division, float bpm)
    {
        if (! isSyncable (fx, p) || division < 0 || division >= (int) DIV_QTY)
            return 0.0f;

        const Desc  d     = describeAt (fx, p, bpm);
        const float value = divisionValue (describe (fx, p), division, bpm);

        /* The inverse of the map in physical(). Because the free range now
           spans exactly the divisions, every division has an exact position on
           the knob - so switching sync off lands on the same sound. */
        if (d.interp == Interp::Exp && d.lo > 0.0f && d.hi > d.lo && value > 0.0f)
            return juce::jlimit (0.0f, 1.0f,
                                 std::log (value / d.lo) / std::log (d.hi / d.lo));

        if (d.hi > d.lo)
            return juce::jlimit (0.0f, 1.0f, (value - d.lo) / (d.hi - d.lo));

        return 0.0f;
    }


    float physical (const Desc& d, float norm)
    {
        const float v = juce::jlimit (0.0f, 1.0f, norm);

        /* Same guard as FxParam_Exp: an exponential map through zero is not a
           map, so it falls back to linear rather than producing NaN. */
        if (d.interp == Interp::Exp && d.lo > 0.0f && d.hi > d.lo)
            return d.lo * std::pow (d.hi / d.lo, v);

        return d.lo + v * (d.hi - d.lo);
    }


    /* juce::String (value, 0) does NOT mean "no decimals" - it falls back to the
       shortest faithful representation, which is how 0.282843 s came out as
       "282.843 ms". Rounding to an int is the only way to actually get none. */
    static juce::String formatSeconds (float sec)
    {
        if (sec < 0.1f)
            return juce::String (sec * 1000.0f, 1) + " ms";

        if (sec < 1.0f)
            return juce::String (juce::roundToInt (sec * 1000.0f)) + " ms";

        return juce::String (sec, 2) + " s";
    }

    static juce::String formatHertz (float hz)
    {
        if (hz >= 1000.0f)
            return juce::String (hz / 1000.0f, 2) + " kHz";

        if (hz < 10.0f)
            return juce::String (hz, 2) + " Hz";

        return juce::String (juce::roundToInt (hz)) + " Hz";
    }

    static juce::String formatValue (const Desc& d, float value)
    {
        switch (d.unit)
        {
            case Unit::Percent:
                return juce::String (juce::roundToInt (value * 100.0f)) + " %";

            case Unit::Decibel:
                /* A gain of zero is silence, and log(0) is not a number a knob
                   should ever show. */
                return value <= 0.001f ? juce::String ("-inf dB")
                                       : juce::String (20.0f * std::log10 (value), 1) + " dB";

            case Unit::Times:
                return "x" + juce::String (value, value < 10.0f ? 2 : 1);

            case Unit::Db:
                return juce::String (value, 1) + " dB";

            case Unit::Ratio:
                /* 1:1 is no compression, and saying so beats printing "1.0:1"
                   and leaving the player to work it out. */
                return (value <= 1.005f) ? juce::String ("1:1  off")
                                         : juce::String (value, 1) + ":1";

            case Unit::Seconds:  return formatSeconds (value);
            case Unit::Hertz:    return formatHertz (value);
            case Unit::Degrees:  return juce::String (juce::roundToInt (value)) + " deg";

            case Unit::Pan:
            {
                const int pos = juce::roundToInt ((value * 2.0f - 1.0f) * 100.0f);

                if (pos == 0)  return "C";
                if (pos < 0)   return "L" + juce::String (-pos);
                return "R" + juce::String (pos);
            }

            case Unit::Stages:
            {
                /* The same quantisation the effect applies - each PAIR of
                   stages adds one notch, so anything between two settings
                   would sound like one of them anyway. */
                int step = (int) (value * (float) PHASER_STAGE_STEPS + 0.0001f);

                if (step >= (int) PHASER_STAGE_STEPS)
                    step = (int) PHASER_STAGE_STEPS - 1;

                return juce::String ((step + 1) * 2) + " st";
            }

            case Unit::Raw:
            default:
                return juce::String (value, 2);
        }
    }


    juce::String format (int fx, int p, float norm, float bpm)
    {
        const auto d = describeAt (fx, p, bpm);

        return formatValue (d, physical (d, norm));
    }


    bool syncResolves (int fx, int p)
    {
        const auto d = describe (fx, p);

        return d.unit == Unit::Seconds || d.unit == Unit::Hertz;
    }


    juce::String formatSynced (int fx, int p, int division, float bpm)
    {
        if (! syncResolves (fx, p) || division < 0 || division >= (int) DIV_QTY)
            return {};

        const auto  d           = describe (fx, p);
        const float quarterSec  = 60.0f / juce::jlimit (20.0f, 400.0f, bpm);
        const float periodSec   = g_aDivQuarters[division] * quarterSec;

        /* Exactly what FxParam_TimeSec and FxParam_RateHz do, including the
           clamp - a whole note at 40 BPM is six seconds and the delay line only
           holds four, so the display has to show the truth, not the wish. */
        if (d.unit == Unit::Seconds)
            return formatSeconds (juce::jlimit (d.lo, d.hi, periodSec));

        const float hz = periodSec > 0.0f ? 1.0f / periodSec : d.lo;

        return formatHertz (juce::jlimit (d.lo, d.hi, hz));
    }
}
