#include "FxHost.h"

#include <cstring>


namespace FxHost
{
    int effectCount()                 { return (int) FX_TYPE_QTY; }
    int divisionCount()               { return (int) DIV_QTY; }
    double engineSampleRate()         { return (double) AUDIO_SAMPLE_RATE_HZ; }

    static bool valid (int fx)        { return fx >= 0 && fx < (int) FX_TYPE_QTY; }

    bool isStereo (int fx)
    {
        return valid (fx) && g_aFxDesc[fx].nWidth == CHAIN_MAX_WIDTH;
    }

    /*
        Every effect in the pool now has real DSP behind it - fx_stubs.c is gone.

        Kept as a function rather than deleted because the bench's whole job is
        to say plainly what it is playing you, and the next effect added will
        start life unimplemented like all the others did.
    */
    bool isImplemented (int fx)
    {
        return valid (fx);
    }

    juce::String effectName (int fx)
    {
        if (! valid (fx))
            return "-";

        /* The two variants share a name on purpose - they are the same effect
           to a player - so the width is what tells them apart here. */
        return juce::String (g_aFxDesc[fx].pName)
             + (isStereo (fx) ? "  -  stereo" : "  -  mono")
             + (isImplemented (fx) ? "" : "     [ stub - passes audio through ]");
    }

    int paramCount (int fx)
    {
        return valid (fx) ? (int) g_aFxDesc[fx].nParamQty : 0;
    }

    juce::String paramName (int fx, int p)
    {
        if (! valid (fx) || p < 0 || p >= paramCount (fx))
            return {};

        return juce::String (g_aFxDesc[fx].pParam[p].pName);
    }

    bool paramSyncable (int fx, int p)
    {
        if (! valid (fx) || p < 0 || p >= paramCount (fx))
            return false;

        return (g_aFxDesc[fx].pParam[p].nFlags & (U8) FX_PF_SYNCABLE) != 0;
    }

    bool paramStepped (int fx, int p)
    {
        if (! valid (fx) || p < 0 || p >= paramCount (fx))
            return false;

        return (g_aFxDesc[fx].pParam[p].nFlags & (U8) FX_PF_STEPPED) != 0;
    }

    juce::String divisionName (int d)
    {
        return (d >= 0 && d < (int) DIV_QTY) ? juce::String (g_aDivName[d]) : "-";
    }
}


//==============================================================================

FxEngine::FxEngine()
{
    setTempo (120.0f, 4, 4);

    for (auto& p : params)
    {
        p.fValue    = 0.5f;
        p.bSync     = (U8) FALSE;
        p.eDivision = (U8) DIV_1_4;
    }

    reset();
}

void FxEngine::reset()
{
    std::memset (planes, 0, sizeof (planes));

    if (currentFx >= 0 && currentFx < (int) FX_TYPE_QTY
        && g_aFxEntry[currentFx].pfReset != NULL_PTR)
    {
        /* Both planes, always. A mono effect uses them as two separate
           instances, so both need clearing either way. */
        g_aFxEntry[currentFx].pfReset (0U, (U8) CHAIN_MAX_WIDTH);
    }

    tempo.nSampleInBar = 0;
}

void FxEngine::setEffect (int fx)
{
    if (fx == currentFx)
        return;

    currentFx = fx;
    reset();
}

void FxEngine::setParam (int p, float value, bool sync, int division)
{
    if (p < 0 || p >= (int) FX_PARAM_QTY)
        return;

    params[p].fValue    = juce::jlimit (0.0f, 1.0f, value);
    params[p].bSync     = (U8) (sync ? TRUE : FALSE);
    params[p].eDivision = (U8) juce::jlimit (0, (int) DIV_QTY - 1, division);
}

void FxEngine::setTempo (float bpm, int beatsPerBar, int beatUnit)
{
    tempo.fBpm         = juce::jlimit (20.0f, 400.0f, bpm);
    tempo.nBeatsPerBar = (U8) juce::jlimit (1, 32, beatsPerBar);
    tempo.nBeatUnit    = (U8) beatUnit;
    tempo.eSource      = (U8) TEMPO_SRC_INTERNAL;

    /* Same arithmetic as Params_BarFrames, restated here rather than pulling in
       params.c - which manages per-chain storage this bench does not have.
       fBpm is the QUARTER note whatever the time signature says. */
    const double quartersPerBar = (double) tempo.nBeatsPerBar * (4.0 / (double) tempo.nBeatUnit);
    const double barSeconds     = quartersPerBar * (60.0 / (double) tempo.fBpm);

    barFrames = (juce::uint32) juce::jmax (1.0, barSeconds * (double) AUDIO_SAMPLE_RATE_HZ);

    if (tempo.nSampleInBar >= barFrames)
        tempo.nSampleInBar = 0;
}

void FxEngine::processOneBlock (float* left, float* right, int numFrames)
{
    if (currentFx < 0 || currentFx >= (int) FX_TYPE_QTY
        || g_aFxEntry[currentFx].pfProcess == NULL_PTR)
        return;

    const auto n = (U16) numFrames;

    std::memcpy (planes[0], left,  (size_t) numFrames * sizeof (FLOAT32));
    std::memcpy (planes[1], right, (size_t) numFrames * sizeof (FLOAT32));

    FX_CTX ctx {};
    ctx.nChain = 0U;
    ctx.pParam = params;
    ctx.pTempo = &tempo;

    if (FxHost::isStereo (currentFx))
    {
        FLOAT32* ap[CHAIN_MAX_WIDTH] = { planes[0], planes[1] };

        ctx.nPlaneBase = 0U;
        ctx.nWidth     = (U8) CHAIN_MAX_WIDTH;

        g_aFxEntry[currentFx].pfProcess (&ctx, ap, n);
    }
    else
    {
        /* Two independent mono instances, one per plane. State is indexed by
           nPlaneBase, so plane 1 keeps its own filters, LFO phase and delay
           line - it is not the same effect run twice on different audio. */
        ctx.nWidth = 1U;

        for (U8 plane = 0U; plane < (U8) CHAIN_MAX_WIDTH; ++plane)
        {
            FLOAT32* ap[1] = { planes[plane] };

            ctx.nPlaneBase = plane;
            g_aFxEntry[currentFx].pfProcess (&ctx, ap, n);
        }
    }

    std::memcpy (left,  planes[0], (size_t) numFrames * sizeof (FLOAT32));
    std::memcpy (right, planes[1], (size_t) numFrames * sizeof (FLOAT32));

    tempo.nSampleInBar = (tempo.nSampleInBar + (juce::uint32) numFrames) % barFrames;
}

void FxEngine::process (float* left, float* right, int numSamples)
{
    int done = 0;

    while (done < numSamples)
    {
        /* The firmware runs a fixed 64-frame block and several effects derive
           their smoothing from that, so the host's block size is chopped rather
           than passed through. */
        const int chunk = juce::jmin ((int) AUDIO_BLOCK_FRAMES, numSamples - done);

        processOneBlock (left + done, right + done, chunk);

        done += chunk;
    }
}
