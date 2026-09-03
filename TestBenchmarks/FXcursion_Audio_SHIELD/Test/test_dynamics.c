/**
 * @file      test_dynamics.c
 *
 * @details   The compressor and the reverb.
 *
 *            Both are effects where "it changed the audio" says almost nothing.
 *            A compressor that reduces gain by some arbitrary amount is not a
 *            compressor, and a reverb that makes noise is not a room. So these
 *            tests check the properties the controls actually promise: that a
 *            ratio is a ratio in decibels, that LINK really does apply one gain
 *            to both planes, that a decay time is a decay time, and that the
 *            stereo tail is genuinely two different signals.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "audio_sys.h"
#include "grid.h"
#include "params.h"
#include "Effects/fx_common.h"
#include "Effects/fx_compressor.h"
#include "Effects/fx_reverb.h"

#include <math.h>


static S32 aIn[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY];
static S32 aOut[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY];


static void Silence(void)
{
    U16 i;

    for (i = 0U; i < (AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY); i++)
    {
        aIn[i] = 0L;
    }
}

//--------------------------------------------------------------------------------------------------

static void SetParam(const U8 nChain, const U8 eFxType, const U8 nIdx,
                     const U16 nValue, const U8 bSync, const U8 eDiv)
{
    PROTO_SET_PARAM tCmd;

    tCmd.nValue    = nValue;
    tCmd.nChain    = nChain;
    tCmd.eFxType   = eFxType;
    tCmd.nParamIdx = nIdx;
    tCmd.bSync     = bSync;
    tCmd.eDivision = eDiv;
    tCmd.nReserved = 0U;

    CHECK(Params_Set(&tCmd) == RESULT_OK);
}

//--------------------------------------------------------------------------------------------------

/** A normalised 0..1 as the protocol's 16-bit field. */
static U16 Norm(const double f)
{
    return (U16)(FxUtil_Clamp((FLOAT32)f, 0.0f, 1.0f) * 65535.0f);
}

//--------------------------------------------------------------------------------------------------

/** Where a value sits on an exponential range, as a normalised 0..1. */
static double ExpPos(const double v, const double lo, const double hi)
{
    return log(v / lo) / log(hi / lo);
}

//--------------------------------------------------------------------------------------------------

/** Put one effect alone on chain 0 and return with it enabled. */
static void UseEffect(const U8 eTopology, const U8 eFxType)
{
    PROTO_CFG tCfg;
    PROTO_ACK tAck;

    /*
     * The empty grid first, on purpose. Re-applying a configuration that
     * already contains this effect does NOT reset it - that is deliberate, so
     * that touching an unrelated knob does not cut a delay tail - which means a
     * test that just re-applies its own grid inherits whatever the previous
     * test left ringing. Taking the effect out and putting it back is what
     * actually gets a fresh instance.
     */
    Test_MakeDefaultCfg(&tCfg, eTopology);
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    Test_MakeDefaultCfg(&tCfg, eTopology);
    tCfg.aSlot[0][0]   = (U8)BLOCK_FX;
    tCfg.aFxSlot[0][0] = eFxType;
    tCfg.aFxEnabled[0] = 0x01U;

    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);
}

//--------------------------------------------------------------------------------------------------

/** Fill every frame of one channel with a constant, the rest with silence. */
static void FillDc(const U8 nChannel, const S32 nValue)
{
    U16 i;

    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        aIn[((U32)i * AUDIO_CH_QTY) + nChannel] = nValue;
    }
}

//--------------------------------------------------------------------------------------------------

/** Run blocks of the current aIn and return the last block's peak on a channel. */
static double RunDc(const U32 nBlocks, const U8 nChannel)
{
    double nPeak = 0.0;
    U32    b;
    U16    i;

    for (b = 0UL; b < nBlocks; b++)
    {
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
    }

    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        const double v = fabs((double)aOut[((U32)i * AUDIO_CH_QTY) + nChannel]);

        if (v > nPeak)
        {
            nPeak = v;
        }
    }

    return nPeak;
}


void Test_Compressor(void)
{
    /* Full scale is 2^23. -20 dBFS is a tenth of it. */
    const double fFull = 8388608.0;

    /* ---- below the knee nothing happens -------------------------------------- */
    TEST_BEGIN("below threshold the compressor is transparent");
    {
        UseEffect((U8)TOPO_4_MONO, (U8)FX_COMPRESSOR_M);

        /* Threshold -20 dB, and a signal 20 dB below THAT - well clear of the
           six dB knee. */
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_THRESHOLD,
                 Norm((-20.0 - (double)COMP_THRESH_MIN_DB) / ((double)COMP_THRESH_MAX_DB - (double)COMP_THRESH_MIN_DB)),
                 (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_RATIO,
                 Norm(ExpPos(8.0, (double)COMP_RATIO_MIN, (double)COMP_RATIO_MAX)), (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_ATTACK,   0U,     (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_RELEASE,  0U,     (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_MAKEUP,   0U,     (U8)FALSE, (U8)DIV_1_4);

        Silence();
        FillDc(0U, (S32)(fFull * 0.01));            /* -40 dBFS */

        {
            const double fOut = RunDc(200UL, 0U);

            /* Unity, to within the 24-bit rounding on the way out. */
            CHECK_NEAR(fOut, fFull * 0.01, 4.0);
        }
    }
    TEST_END();

    /* ---- a ratio is a ratio, in decibels -------------------------------------- */
    TEST_BEGIN("four to one means four dB in, one dB out");
    {
        double fOutA;
        double fOutB;

        UseEffect((U8)TOPO_4_MONO, (U8)FX_COMPRESSOR_M);

        /* Threshold -40 dB so both test levels sit well above the knee. */
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_THRESHOLD,
                 Norm((-40.0 - (double)COMP_THRESH_MIN_DB) / ((double)COMP_THRESH_MAX_DB - (double)COMP_THRESH_MIN_DB)),
                 (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_RATIO,
                 Norm(ExpPos(4.0, (double)COMP_RATIO_MIN, (double)COMP_RATIO_MAX)), (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_ATTACK,   0U,     (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_RELEASE,  0U,     (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_MAKEUP,   0U,     (U8)FALSE, (U8)DIV_1_4);

        /* -20 dBFS, then -10 dBFS: ten decibels more going in. */
        Silence();
        FillDc(0U, (S32)(fFull * 0.1));
        fOutA = RunDc(400UL, 0U);

        Silence();
        FillDc(0U, (S32)(fFull * 0.31623));
        fOutB = RunDc(400UL, 0U);

        {
            /* Measuring the DIFFERENCE, not the absolute level: this is what a
               ratio actually asserts, and it does not care about any fixed gain
               error elsewhere in the chain. */
            const double fDeltaDb = 20.0 * log10(fOutB / fOutA);

            CHECK(fOutA > 1.0);
            CHECK_NEAR(fDeltaDb, 2.5, 0.25);
        }
    }
    TEST_END();

    /* ---- release actually takes time ----------------------------------------- */
    TEST_BEGIN("a slow release holds the gain down");
    {
        double fQuietCompressed;
        double fQuietRecovered;

        UseEffect((U8)TOPO_4_MONO, (U8)FX_COMPRESSOR_M);

        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_THRESHOLD,
                 Norm((-40.0 - (double)COMP_THRESH_MIN_DB) / ((double)COMP_THRESH_MAX_DB - (double)COMP_THRESH_MIN_DB)),
                 (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_RATIO,
                 Norm(ExpPos(20.0, (double)COMP_RATIO_MIN, (double)COMP_RATIO_MAX)), (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_ATTACK,   0U,      (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_RELEASE,  65535U,  (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_M, (U8)FX_COMPM_P_MAKEUP,   0U,      (U8)FALSE, (U8)DIV_1_4);

        /* Loud for a while, so the gain is well down. */
        Silence();
        FillDc(0U, (S32)(fFull * 0.5));
        (void)RunDc(400UL, 0U);

        /* Then quiet. One block later the gain must still be held down. */
        Silence();
        FillDc(0U, (S32)(fFull * 0.005));
        fQuietCompressed = RunDc(1UL, 0U);

        /* Several time constants later it has recovered.
         *
         * Not one. A release of 1000 ms is the one-pole's TIME CONSTANT, so
         * after a single second the envelope has only come a third of the way
         * back and the compressor is still holding the signal down by twenty
         * decibels. Seven of them is what it takes to clear the knee. */
        fQuietRecovered = RunDc(6000UL, 0U);

        CHECK(fQuietCompressed < (fFull * 0.005 * 0.5));
        CHECK_NEAR(fQuietRecovered, fFull * 0.005, 200.0);
    }
    TEST_END();

    /* ---- the parameter that keeps a stereo image still ------------------------ */
    TEST_BEGIN("link applies one gain to both planes");
    {
        double fLeftGain;
        double fRightGain;

        UseEffect((U8)TOPO_2_STEREO, (U8)FX_COMPRESSOR_S);

        SetParam(0U, (U8)FX_COMPRESSOR_S, (U8)FX_COMPS_P_THRESHOLD,
                 Norm((-40.0 - (double)COMP_THRESH_MIN_DB) / ((double)COMP_THRESH_MAX_DB - (double)COMP_THRESH_MIN_DB)),
                 (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_S, (U8)FX_COMPS_P_RATIO,
                 Norm(ExpPos(8.0, (double)COMP_RATIO_MIN, (double)COMP_RATIO_MAX)), (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_S, (U8)FX_COMPS_P_ATTACK,   0U,     (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_S, (U8)FX_COMPS_P_RELEASE,  0U,     (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_S, (U8)FX_COMPS_P_MAKEUP,   0U,     (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_COMPRESSOR_S, (U8)FX_COMPS_P_LINK,     65535U, (U8)FALSE, (U8)DIV_1_4);

        /* Loud left, quiet right - the case that makes an unlinked compressor
           drag the image left every time the left side peaks. */
        Silence();
        FillDc(0U, (S32)(fFull * 0.5));
        FillDc(1U, (S32)(fFull * 0.05));

        fLeftGain  = RunDc(400UL, 0U) / (fFull * 0.5);
        fRightGain = RunDc(1UL,   1U) / (fFull * 0.05);

        /* Same gain on both sides: the image does not move. */
        CHECK(fLeftGain < 0.5);                     /* it really is compressing */
        CHECK_NEAR(fRightGain, fLeftGain, fLeftGain * 0.02);

        /* Unlinked, the quiet side is below threshold and is left alone. */
        SetParam(0U, (U8)FX_COMPRESSOR_S, (U8)FX_COMPS_P_LINK, 0U, (U8)FALSE, (U8)DIV_1_4);

        fLeftGain  = RunDc(400UL, 0U) / (fFull * 0.5);
        fRightGain = RunDc(1UL,   1U) / (fFull * 0.05);

        CHECK(fRightGain > (fLeftGain * 2.0));
    }
    TEST_END();
}


/*==================================================================================================
* Reverb
*=================================================================================================*/

/** Peak of channel nCh over the next nBlocks blocks of silence. */
static double TailPeak(const U32 nBlocks, const U8 nCh)
{
    double fPeak = 0.0;
    U32    b;
    U16    i;

    for (b = 0UL; b < nBlocks; b++)
    {
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            const double v = fabs((double)aOut[((U32)i * AUDIO_CH_QTY) + nCh]);

            if (v > fPeak)
            {
                fPeak = v;
            }
        }
    }

    return fPeak;
}

//--------------------------------------------------------------------------------------------------

static void SetReverb(const U8 eFxType, const U8 nDecayIdx, const U8 nMixIdx,
                      const double fDecaySec, const U16 nPredelay, const U8 bSync, const U8 eDiv)
{
    SetParam(0U, eFxType, nDecayIdx,
             Norm(ExpPos(fDecaySec, (double)REV_DECAY_MIN_SEC, (double)REV_DECAY_MAX_SEC)),
             (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, eFxType, (U8)FX_REVERBM_P_PREDELAY, nPredelay, bSync, eDiv);
    SetParam(0U, eFxType, (U8)FX_REVERBM_P_DAMPING,   0U,     (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, eFxType, (U8)FX_REVERBM_P_DIFFUSION, 45000U, (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, eFxType, nMixIdx,                    65535U, (U8)FALSE, (U8)DIV_1_4);
}


void Test_Reverb(void)
{
    const double fFull = 8388608.0;
    const U32    nBlocksPerSec = (U32)AUDIO_SAMPLE_RATE_HZ / AUDIO_BLOCK_FRAMES;   /* 750 */

    /* ---- fully dry is untouched ---------------------------------------------- */
    TEST_BEGIN("at zero mix the reverb is bit-exact");
    {
        U16 i;

        UseEffect((U8)TOPO_4_MONO, (U8)FX_REVERB_M);
        SetReverb((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DECAY, (U8)FX_REVERBM_P_MIX,
                  4.0, 0U, (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_REVERB_M, (U8)FX_REVERBM_P_MIX, 0U, (U8)FALSE, (U8)DIV_1_4);

        Silence();
        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            aIn[(U32)i * AUDIO_CH_QTY] = (S32)(fFull * 0.25);
        }

        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            CHECK_EQ_U32((U32)aOut[(U32)i * AUDIO_CH_QTY],
                         (U32)aIn[(U32)i * AUDIO_CH_QTY]);
        }
    }
    TEST_END();

    /* ---- the decay control is a decay time, measured ------------------------- */
    TEST_BEGIN("the decay knob really is an RT60");
    {
        /*
         * The test this replaces only checked that a long decay outlasts a
         * short one - which is true of almost anything, and was still true when
         * the tail turned out to be far too sparse to hear. Measuring the
         * actual slope is what a decay TIME claims.
         *
         * Excited with a burst, not an impulse: all of an impulse's energy is
         * one sample spread over half a million, so it says nothing about what
         * an instrument through the effect sounds like.
         */
        static const double aWant[3] = { 1.0, 4.0, 10.0 };
        U8 t;

        for (t = 0U; t < 3U; t++)
        {
            double fA;
            double fB;
            double fRt60;

            UseEffect((U8)TOPO_4_MONO, (U8)FX_REVERB_M);
            SetReverb((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DECAY, (U8)FX_REVERBM_P_MIX,
                      aWant[t], 0U, (U8)FALSE, (U8)DIV_1_4);
            SetParam(0U, (U8)FX_REVERB_M, (U8)FX_REVERBM_P_SIZE, 65535U,
                     (U8)FALSE, (U8)DIV_1_4);

            /* Half a second of DC to fill the network, then let it ring. */
            Silence();
            FillDc(0U, (S32)(fFull * 0.25));
            (void)RunDc(nBlocksPerSec / 2U, 0U);
            Silence();

            /* Two points on the decay.
             *
             * The first is taken 30% of the way into the expected tail, not
             * immediately: the input diffusers and the in-loop allpasses spread
             * energy forward in time, so the first tenth of a second is still
             * FILLING and is not on the exponential yet. Measuring there makes
             * the slope look shallower than it is.
             *
             * Elapsed is centre to centre, so the measurement windows count -
             * leaving them out of the arithmetic overstated RT60 by a fifth. */
            const double fWindow = 0.05;
            const double fGap    = aWant[t] * 0.40;

            (void)TailPeak((U32)((double)nBlocksPerSec * aWant[t] * 0.30), 0U);
            fA = TailPeak((U32)((double)nBlocksPerSec * fWindow), 0U);

            (void)TailPeak((U32)((double)nBlocksPerSec * fGap), 0U);
            fB = TailPeak((U32)((double)nBlocksPerSec * fWindow), 0U);

            CHECK(fA > 10.0);
            CHECK(fB > 0.0);
            CHECK(fB < fA);

            fRt60 = (fGap + fWindow) * 60.0 / (20.0 * log10(fA / fB));

            printf("\n      decay %4.1f s -> measured %4.1f s", aWant[t], fRt60);

            /* Half to double. Loose, because the damping filter and the in-loop
               allpasses each take a little off the top, but tight enough that a
               tail decaying at the wrong rate fails. */
            CHECK(fRt60 > (aWant[t] * 0.5));
            CHECK(fRt60 < (aWant[t] * 2.0));
        }
    }
    TEST_END();

    /* ---- density is what makes it a room ------------------------------------- */
    TEST_BEGIN("the tail is dense, not a handful of echoes");
    {
        U32 nLive = 0UL;
        U32 b;
        U16 i;

        UseEffect((U8)TOPO_4_MONO, (U8)FX_REVERB_M);
        SetReverb((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DECAY, (U8)FX_REVERBM_P_MIX,
                  6.0, 0U, (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_REVERB_M, (U8)FX_REVERBM_P_SIZE, 65535U, (U8)FALSE, (U8)DIV_1_4);

        Silence();
        aIn[0] = (S32)(fFull * 0.5);
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        Silence();

        /* Past the pre-delay - it is syncable, so its free minimum follows the
           tempo and is 62.5 ms at 120 BPM. */
        (void)TailPeak(90UL, 0U);

        for (b = 0UL; b < 36UL; b++)
        {
            AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

            for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
            {
                if (fabs((double)aOut[(U32)i * AUDIO_CH_QTY]) > (fFull * 0.0005))
                {
                    nLive++;
                }
            }
        }

        /*
         * Four lines of 40 to 74 ms gave about eighty echoes a second, which
         * reads as thin and distant however long it technically rings for.
         * Eight shorter lines, each with an allpass in its feedback path,
         * multiply that on every pass. More than half the window carrying
         * energy is the difference between a room and a bank of echoes.
         */
        CHECK(nLive > ((36UL * AUDIO_BLOCK_FRAMES) / 2UL));
    }
    TEST_END();

    /* ---- it stays finite ----------------------------------------------------- */
    TEST_BEGIN("maximum decay does not run away");
    {
        double fEarly;
        double fLate;

        UseEffect((U8)TOPO_4_MONO, (U8)FX_REVERB_M);
        SetReverb((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DECAY, (U8)FX_REVERBM_P_MIX,
                  (double)REV_DECAY_MAX_SEC, 0U, (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_REVERB_M, (U8)FX_REVERBM_P_DIFFUSION, 65535U, (U8)FALSE, (U8)DIV_1_4);

        Silence();
        aIn[0] = (S32)(fFull * 0.9);
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        Silence();

        fEarly = TailPeak(nBlocksPerSec, 0U);
        fLate  = TailPeak(nBlocksPerSec * 8U, 0U);

        /* A feedback network with an orthonormal mixing matrix and gains below
           one cannot grow. If this ever fails the matrix has stopped being
           orthonormal. */
        CHECK(fLate <= fEarly);
        CHECK(fLate < fFull);
    }
    TEST_END();

    /* ---- the pre-delay really delays ----------------------------------------- */
    TEST_BEGIN("pre-delay holds the tail back");
    {
        double fBefore;
        double fAfter;

        UseEffect((U8)TOPO_4_MONO, (U8)FX_REVERB_M);

        /* A synced quarter note at 120 BPM is half a second, which is exactly
           the longest pre-delay the buffer holds. */
        CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);
        SetReverb((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DECAY, (U8)FX_REVERBM_P_MIX,
                  4.0, 0U, (U8)TRUE, (U8)DIV_1_4);

        Silence();
        aIn[0] = (S32)(fFull * 0.5);
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        Silence();

        /* Nothing at all for the first four tenths of a second... */
        fBefore = TailPeak((nBlocksPerSec * 2U) / 5U, 0U);

        /* ...and a tail shortly after the half second is up. */
        (void)TailPeak(nBlocksPerSec / 10U, 0U);
        fAfter = TailPeak(nBlocksPerSec / 2U, 0U);

        CHECK(fBefore < 1.0);
        CHECK(fAfter > 100.0);
    }
    TEST_END();

    /* ---- one room, not two --------------------------------------------------- */
    TEST_BEGIN("the stereo tail is two different signals");
    {
        U32    nDiffer = 0UL;
        U32    b;
        U16    i;
        double fPeakL  = 0.0;

        UseEffect((U8)TOPO_2_STEREO, (U8)FX_REVERB_S);
        SetReverb((U8)FX_REVERB_S, (U8)FX_REVERBS_P_DECAY, (U8)FX_REVERBS_P_MIX,
                  4.0, 0U, (U8)FALSE, (U8)DIV_1_4);
        SetParam(0U, (U8)FX_REVERB_S, (U8)FX_REVERBS_P_WIDTH, 32768U, (U8)FALSE, (U8)DIV_1_4);

        /* The SAME signal into both planes. Two independent reverbs would give
           back two identical tails; one room gives back two different ones. */
        Silence();
        aIn[0] = (S32)(fFull * 0.5);
        aIn[1] = (S32)(fFull * 0.5);
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        Silence();

        for (b = 0UL; b < (nBlocksPerSec / 2U); b++)
        {
            AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

            for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
            {
                const S32 nL = aOut[((U32)i * AUDIO_CH_QTY) + 0U];
                const S32 nR = aOut[((U32)i * AUDIO_CH_QTY) + 1U];

                if (fabs((double)nL) > fPeakL)
                {
                    fPeakL = fabs((double)nL);
                }

                if (nL != nR)
                {
                    nDiffer++;
                }
            }
        }

        CHECK(fPeakL > 100.0);

        /* Not a handful of rounding differences - genuinely decorrelated. */
        CHECK(nDiffer > ((nBlocksPerSec / 2U) * AUDIO_BLOCK_FRAMES / 2U));
    }
    TEST_END();
}
