/**
 * @file      test_modulation.c
 *
 * @details   The five effects added last: chorus, flanger, vibrato, phaser and
 *            distortion.
 *
 *            The distortion test is the one that matters. Its header claims
 *            that running the clipper at twice the rate keeps the aliases out,
 *            and that claim is invisible to every other kind of check - a
 *            badly aliased distortion still passes "it changed the audio",
 *            "it is finite" and "it is not silent". So it is measured directly,
 *            at the frequency the aliasing lands on.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "audio_sys.h"
#include "grid.h"
#include "params.h"
#include "Effects/fx_common.h"
#include "Effects/fx_modulation.h"
#include "Effects/fx_distortion.h"

#include <math.h>


static S32 aIn[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY];
static S32 aOut[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY];


static void Sil(void)
{
    U16 i;
    for (i = 0U; i < (AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY); i++) { aIn[i] = 0L; }
}

static void SetP(const U8 eFx, const U8 nIdx, const U16 nVal)
{
    PROTO_SET_PARAM t;
    t.nValue = nVal; t.nChain = 0U; t.eFxType = eFx; t.nParamIdx = nIdx;
    t.bSync = (U8)FALSE; t.eDivision = (U8)DIV_1_4; t.nReserved = 0U;
    CHECK(Params_Set(&t) == RESULT_OK);
}

static void Fresh(const U8 eTopology, const U8 eFx)
{
    PROTO_CFG c;
    PROTO_ACK a;

    /* Empty first: re-applying a grid that already holds this effect does not
       reset it, so without this a test inherits the previous one's state. */
    Test_MakeDefaultCfg(&c, eTopology);
    CHECK(Grid_Apply(&c, &a) == RESULT_OK);

    Test_MakeDefaultCfg(&c, eTopology);
    c.aSlot[0][0] = (U8)BLOCK_FX;
    c.aFxSlot[0][0] = eFx;
    c.aFxEnabled[0] = 0x01U;
    CHECK(Grid_Apply(&c, &a) == RESULT_OK);
}

/** Magnitude at one frequency, by correlating against a sine and a cosine. */
static double BinMag(const double* const pBuf, const U32 nLen, const double fHz)
{
    double fRe = 0.0;
    double fIm = 0.0;
    U32    i;

    for (i = 0UL; i < nLen; i++)
    {
        const double fW = 2.0 * 3.14159265358979 * fHz * (double)i /
                          (double)AUDIO_SAMPLE_RATE_HZ;

        fRe += pBuf[i] * cos(fW);
        fIm += pBuf[i] * sin(fW);
    }

    return sqrt((fRe * fRe) + (fIm * fIm)) * 2.0 / (double)nLen;
}


void Test_Modulation(void)
{
    const double fFull = 8388608.0;

    /* ---- vibrato has no dry path -------------------------------------------- */
    TEST_BEGIN("vibrato is entirely wet");
    {
        U16 i;
        U32 nEarly = 0UL;

        Fresh((U8)TOPO_4_MONO, (U8)FX_VIBRATO_M);
        SetP((U8)FX_VIBRATO_M, (U8)FX_VIBRATOM_P_RATE,  0U);    /* slowest      */
        SetP((U8)FX_VIBRATO_M, (U8)FX_VIBRATOM_P_DEPTH, 0U);    /* no sweep     */

        Sil();
        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            aIn[(U32)i * AUDIO_CH_QTY] = (S32)(fFull * 0.5);
        }

        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

        /* At the centre delay of 3 ms nothing has come through yet in the first
           block. A chorus would already be passing the dry signal; vibrato has
           none to pass. */
        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            if (fabs((double)aOut[(U32)i * AUDIO_CH_QTY]) > 1.0) { nEarly++; }
        }

        CHECK_EQ_U32(nEarly, 0UL);

        /* And it does arrive, a few milliseconds later. */
        {
            double fPeak = 0.0;
            U32    b;

            for (b = 0UL; b < 10UL; b++)
            {
                AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

                for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
                {
                    const double v = fabs((double)aOut[(U32)i * AUDIO_CH_QTY]);
                    if (v > fPeak) { fPeak = v; }
                }
            }

            CHECK_NEAR(fPeak, fFull * 0.5, fFull * 0.02);
        }
    }
    TEST_END();

    /* ---- spread is what makes a stereo chorus wide --------------------------- */
    TEST_BEGIN("chorus spread decorrelates the two planes");
    {
        U32 nDiffer = 0UL;
        U32 b;
        U16 i;

        Fresh((U8)TOPO_2_STEREO, (U8)FX_CHORUS_S);
        SetP((U8)FX_CHORUS_S, (U8)FX_CHORUSS_P_RATE,   30000U);
        SetP((U8)FX_CHORUS_S, (U8)FX_CHORUSS_P_DEPTH,  65535U);
        SetP((U8)FX_CHORUS_S, (U8)FX_CHORUSS_P_DELAY,  32768U);
        SetP((U8)FX_CHORUS_S, (U8)FX_CHORUSS_P_MIX,    65535U);
        SetP((U8)FX_CHORUS_S, (U8)FX_CHORUSS_P_SPREAD, 65535U);  /* antiphase   */

        /* The SAME signal into both planes. */
        for (b = 0UL; b < 200UL; b++)
        {
            for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
            {
                const double fPh = 2.0 * 3.14159265358979 * 440.0 *
                                   (double)((b * AUDIO_BLOCK_FRAMES) + i) /
                                   (double)AUDIO_SAMPLE_RATE_HZ;
                const S32 v = (S32)(fFull * 0.4 * sin(fPh));

                aIn[((U32)i * AUDIO_CH_QTY) + 0U] = v;
                aIn[((U32)i * AUDIO_CH_QTY) + 1U] = v;
            }

            AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

            if (b > 50UL)
            {
                for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
                {
                    const S32 nL = aOut[((U32)i * AUDIO_CH_QTY) + 0U];
                    const S32 nR = aOut[((U32)i * AUDIO_CH_QTY) + 1U];

                    if (labs((long)(nL - nR)) > (long)(fFull * 0.001)) { nDiffer++; }
                }
            }
        }

        /* At full spread the two LFOs are half a cycle apart, so the sides are
           reading different points of the delay line almost all the time. */
        CHECK(nDiffer > ((149UL * AUDIO_BLOCK_FRAMES) / 2UL));
    }
    TEST_END();

    /* ---- the claim the distortion header makes ------------------------------- */
    TEST_BEGIN("oversampling keeps the clipper's aliases down");
    {
        /*
         * 1100 Hz, deliberately not a round number.
         *
         * A symmetric clipper produces ONLY odd harmonics - 3300, 5500, 7700 and
         * so on - so any energy at a frequency that is not an odd multiple of
         * the fundamental did not come from the clipper. It came from folding.
         * Probing at 300, 700 and 1700 Hz therefore measures aliasing directly,
         * with nothing legitimate to hide behind.
         *
         * The first version of this test used 7 kHz, which is above any guitar
         * note and drove the aliases to -22 dB. That is not a bug in the
         * effect: at high drive the waveform is a square whatever shape the
         * clipper's corner has, and a square's harmonics fall off as 1/n, so
         * SOME of them land in band at any finite sample rate. It is a real
         * limit of digital clipping, it is documented in fx_distortion.c, and
         * it is worth knowing - but the case worth testing is the one the
         * effect will actually meet.
         */
        static double aBuf[4096];
        U32    n = 0UL;
        U32    b;
        U16    i;
        double fFund;
        double fAlias;

        Fresh((U8)TOPO_4_MONO, (U8)FX_DISTORTION_M);
        SetP((U8)FX_DISTORTION_M, (U8)FX_DISTM_P_DRIVE, 55000U);
        SetP((U8)FX_DISTORTION_M, (U8)FX_DISTM_P_TONE,  65535U);   /* wide open */
        SetP((U8)FX_DISTORTION_M, (U8)FX_DISTM_P_LEVEL, 32768U);
        SetP((U8)FX_DISTORTION_M, (U8)FX_DISTM_P_MIX,   65535U);   /* fully wet */

        for (b = 0UL; b < 128UL; b++)
        {
            for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
            {
                const double fPh = 2.0 * 3.14159265358979 * 1100.0 *
                                   (double)((b * AUDIO_BLOCK_FRAMES) + i) /
                                   (double)AUDIO_SAMPLE_RATE_HZ;

                aIn[(U32)i * AUDIO_CH_QTY] = (S32)(fFull * 0.4 * sin(fPh));
            }

            AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

            /* Skip the first blocks so the filters have settled. */
            if (b >= 64UL)
            {
                for (i = 0U; (i < AUDIO_BLOCK_FRAMES) && (n < 4096UL); i++)
                {
                    aBuf[n] = (double)aOut[(U32)i * AUDIO_CH_QTY];
                    n++;
                }
            }
        }

        fFund  = BinMag(aBuf, n, 1100.0);
        fAlias = BinMag(aBuf, n, 300.0);

        printf("\n      1100 Hz fund %.0f  non-harmonic:", fFund);
        {
            static const double aProbe[3] = { 300.0, 700.0, 1700.0 };
            U8 q;
            for (q = 0U; q < 3U; q++)
            {
                printf(" %.0fHz %.1fdB", aProbe[q],
                       20.0 * log10((BinMag(aBuf, n, aProbe[q]) + 1e-9) / fFund));
            }
        }

        CHECK(fFund > (fFull * 0.01));

        /* Forty-five decibels down, measured at -48 to -59. Nothing legitimate
           can appear at these frequencies, so whatever is there folded in.
           Without the oversampling the same harmonics land around -17 to -25 dB,
           so this threshold has plenty of room to catch a regression while
           leaving the real residue alone. */
        CHECK(fAlias < (fFund * 0.0056));
        CHECK(BinMag(aBuf, n, 700.0)  < (fFund * 0.0056));
        CHECK(BinMag(aBuf, n, 1700.0) < (fFund * 0.0056));
    }
    TEST_END();
}
