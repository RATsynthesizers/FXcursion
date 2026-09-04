/**
 * @file      test_modprofile.c
 *
 * @details   Characterises the static comb of chorus, flanger and phaser with
 *            the LFO held still.
 *
 *            Diagnostic, not an assertion. All three are "dry plus a modulated
 *            something", so all three pass every test that asks whether they
 *            changed the audio - and yet they are supposed to sound like three
 *            different effects.
 *
 *            The first version of this test probed sixteen log-spaced tones and
 *            printed noise, because a 16 ms delay puts a notch every 62 Hz and
 *            sixteen scattered probes cannot see a comb that dense - they alias
 *            it. This one takes the impulse response and walks a FINE linear
 *            grid across it, which is the only way the structure is visible:
 *
 *              ripple      how deep the comb is at all
 *              notches     how many there are in the band
 *              spacing CV  how EVENLY they are spaced - the whole ball game.
 *                          A flanger's notches come from a fixed delay, so they
 *                          are harmonically spaced and the CV is near zero. A
 *                          phaser's come from phase and are not, so its CV is
 *                          large. That difference IS the difference in sound.
 *
 *            The correlation matrix at the end is the direct answer to "why do
 *            these sound the same": two curves that correlate at 0.9 are two
 *            settings of one effect, whatever the effect is called.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "audio_sys.h"
#include "grid.h"
#include "params.h"
#include "Effects/fx_common.h"

#include <math.h>


#define MP_IR_LEN       (8192U)     /**< 170 ms - long enough for the tails    */
#define MP_BINS         (2400U)     /**< 5 Hz apart: ~12 bins per chorus notch  */
#define MP_F_LO         (60.0)
#define MP_F_HI         (12000.0)
#define MP_FX_QTY       (3U)
#define MP_PI           (3.14159265358979)

/* Prominence a dip needs to count as a notch. Measured, not guessed: at 4 dB
   the shallow high-frequency notches of the flanger are missed and single gaps
   read as double ones, which threw its spacing CV from 0.03 to 1.00; at 1 dB
   ordinary ripple gets counted and it drifts to 0.40. At 2 dB the count is
   right and the number means what it says. */
#define MP_NOTCH_DB     (2.0)

static S32    aIn[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY];
static S32    aOut[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY];
static double aIr[MP_IR_LEN];
static double aDb[MP_FX_QTY][MP_BINS];


static void MpSetP(const U8 eFx, const U8 nIdx, const U16 nVal)
{
    PROTO_SET_PARAM t;
    t.nValue = nVal; t.nChain = 0U; t.eFxType = eFx; t.nParamIdx = nIdx;
    t.bSync = (U8)FALSE; t.eDivision = (U8)DIV_1_4; t.nReserved = 0U;
    (void)Params_Set(&t);
}

static void MpFresh(const U8 eFx)
{
    PROTO_CFG c;
    PROTO_ACK a;

    /* An empty grid first: re-applying an identical grid deliberately does NOT
       reset an effect, so without this the previous tail is still ringing. */
    Test_MakeDefaultCfg(&c, (U8)TOPO_4_MONO);
    (void)Grid_Apply(&c, &a);

    Test_MakeDefaultCfg(&c, (U8)TOPO_4_MONO);
    c.aSlot[0][0]   = (U8)BLOCK_FX;
    c.aFxSlot[0][0] = eFx;
    c.aFxEnabled[0] = 0x01U;
    (void)Grid_Apply(&c, &a);
}

/** Impulse response with everything at the middle of its range, LFO parked. */
static void MpCapture(const U8 eFx)
{
    /* Quarter scale: big enough to sit well clear of the integer floor, small
       enough that the soft clip on the feedback path stays essentially linear
       and we are measuring a filter rather than a distortion. */
    const double fAmp = 8388608.0 * 0.25;
    U32          n    = 0UL;
    U32          b;

    MpFresh(eFx);

    MpSetP(eFx, 0U, 0U);            /* rate: slowest, so the LFO barely moves  */
    MpSetP(eFx, 1U, 32768U);        /* depth                                   */
    MpSetP(eFx, 2U, 32768U);        /* delay / feedback                        */
    MpSetP(eFx, 3U, 32768U);        /* mix, or stages on the phaser            */
    MpSetP(eFx, 4U, 32768U);        /* mix on the phaser; rejected elsewhere   */

    for (b = 0UL; (b * AUDIO_BLOCK_FRAMES) < MP_IR_LEN; b++)
    {
        U16 i;

        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            aIn[(U32)i * AUDIO_CH_QTY] = 0;
        }

        if (b == 0UL)
        {
            aIn[0] = (S32)fAmp;
        }

        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

        for (i = 0U; (i < AUDIO_BLOCK_FRAMES) && (n < MP_IR_LEN); i++)
        {
            aIr[n] = (double)aOut[(U32)i * AUDIO_CH_QTY] / fAmp;
            n++;
        }
    }
}

/** Goertzel: one multiply per sample per bin, and no trig in the inner loop. */
static double MpBinMag(const double fHz)
{
    const double fW    = 2.0 * MP_PI * fHz / (double)AUDIO_SAMPLE_RATE_HZ;
    const double fCoef = 2.0 * cos(fW);
    double       fS1   = 0.0;
    double       fS2   = 0.0;
    double       fRe;
    double       fIm;
    U32          k;

    for (k = 0UL; k < MP_IR_LEN; k++)
    {
        const double fS0 = aIr[k] + (fCoef * fS1) - fS2;

        fS2 = fS1;
        fS1 = fS0;
    }

    fRe = fS1 - (fS2 * cos(fW));
    fIm = fS2 * sin(fW);

    return sqrt((fRe * fRe) + (fIm * fIm));
}

/**
 * @brief Count notches and measure how evenly they are spaced.
 *
 * A notch is a local minimum at least 4 dB below both shoulders, which keeps
 * ordinary ripple out of the count. The spacing CV is the standard deviation of
 * the gaps over their mean - dimensionless, so a flanger at any delay and a
 * phaser at any rate are directly comparable.
 */
static void MpNotchStats(const double* pDb, U16* const pQty, double* const pCv,
                         double* const pFirstHz)
{
    const double  fStep = (MP_F_HI - MP_F_LO) / (double)(MP_BINS - 1U);
    static double aSm[MP_BINS];
    double        aHz[256];
    U16           nQty = 0U;
    U16           k;

    /* Light smoothing, so single-bin wiggle cannot read as a notch. */
    for (k = 0U; k < MP_BINS; k++)
    {
        double fSum = 0.0;
        U16    nN   = 0U;
        S32    d;

        for (d = -1; d <= 1; d++)
        {
            const S32 j = (S32)k + d;

            if ((j >= 0) && (j < (S32)MP_BINS))
            {
                fSum += pDb[j];
                nN++;
            }
        }

        aSm[k] = fSum / (double)nN;
    }

    /*
     * Prominence, not a fixed shoulder distance.
     *
     * The first version compared each minimum against the bins three along,
     * which can only ever find notches narrower than about 30 Hz. It counted 66
     * in the chorus and none at all in the flanger or the phaser - whose
     * notches are hundreds of Hz wide - and so reported a spacing CV of zero
     * for both, which is the one number this test exists to produce. Walking
     * out to the neighbouring PEAKS instead is scale-free: one threshold then
     * works for a 62 Hz comb and a 2 kHz one alike.
     */
    for (k = 1U; k < (MP_BINS - 1U); k++)
    {
        if ((aSm[k] <= aSm[k - 1U]) && (aSm[k] < aSm[k + 1U]) && (nQty < 256U))
        {
            double fL = aSm[k];
            double fR = aSm[k];
            U16    j;

            for (j = k; j > 0U; j--)
            {
                if (aSm[j - 1U] < aSm[j]) { break; }
                fL = aSm[j - 1U];
            }

            for (j = k; j < (MP_BINS - 1U); j++)
            {
                if (aSm[j + 1U] < aSm[j]) { break; }
                fR = aSm[j + 1U];
            }

            if (((fL - aSm[k]) > MP_NOTCH_DB) && ((fR - aSm[k]) > MP_NOTCH_DB))
            {
                aHz[nQty] = MP_F_LO + ((double)k * fStep);
                nQty++;
            }
        }
    }

    *pQty     = nQty;
    *pFirstHz = (nQty > 0U) ? aHz[0] : 0.0;
    *pCv      = 0.0;

    if (nQty >= 3U)
    {
        double fMean = 0.0;
        double fVar  = 0.0;

        for (k = 1U; k < nQty; k++)
        {
            fMean += aHz[k] - aHz[k - 1U];
        }
        fMean /= (double)(nQty - 1U);

        for (k = 1U; k < nQty; k++)
        {
            const double fD = (aHz[k] - aHz[k - 1U]) - fMean;

            fVar += fD * fD;
        }
        fVar /= (double)(nQty - 1U);

        if (fMean > 1.0e-9)
        {
            *pCv = sqrt(fVar) / fMean;
        }
    }
}

/** Pearson correlation of two dB curves. 1.0 means "the same effect". */
static double MpCorr(const double* pA, const double* pB)
{
    double fMa = 0.0;
    double fMb = 0.0;
    double fSab = 0.0;
    double fSaa = 0.0;
    double fSbb = 0.0;
    U16    k;

    for (k = 0U; k < MP_BINS; k++)
    {
        fMa += pA[k];
        fMb += pB[k];
    }
    fMa /= (double)MP_BINS;
    fMb /= (double)MP_BINS;

    for (k = 0U; k < MP_BINS; k++)
    {
        const double fA = pA[k] - fMa;
        const double fB = pB[k] - fMb;

        fSab += fA * fB;
        fSaa += fA * fA;
        fSbb += fB * fB;
    }

    return (fSaa > 1.0e-12 && fSbb > 1.0e-12) ? (fSab / sqrt(fSaa * fSbb)) : 0.0;
}


void Test_ModProfile(void)
{
    static const U8    aFx[MP_FX_QTY]   = { (U8)FX_CHORUS_M, (U8)FX_FLANGER_M,
                                            (U8)FX_PHASER_M };
    static const char* aName[MP_FX_QTY] = { "chorus ", "flanger", "phaser " };

    const double fStep = (MP_F_HI - MP_F_LO) / (double)(MP_BINS - 1U);
    U8           e;

    printf("\n  static comb, LFO parked, %.0f Hz .. %.0f Hz\n", MP_F_LO, MP_F_HI);
    printf("            ripple  notches  spacing CV  first notch\n");

    for (e = 0U; e < MP_FX_QTY; e++)
    {
        double fLo = 1.0e9;
        double fHi = -1.0e9;
        double fCv;
        double fFirst;
        U16    nQty;
        U16    k;

        MpCapture(aFx[e]);

        for (k = 0U; k < MP_BINS; k++)
        {
            const double fMag = MpBinMag(MP_F_LO + ((double)k * fStep));

            aDb[e][k] = (fMag > 1.0e-7) ? (20.0 * log10(fMag)) : -140.0;

            if (aDb[e][k] < fLo) { fLo = aDb[e][k]; }
            if (aDb[e][k] > fHi) { fHi = aDb[e][k]; }
        }

        MpNotchStats(aDb[e], &nQty, &fCv, &fFirst);

        printf("  %s  %6.1f   %5u      %6.2f     %7.0f Hz\n",
               aName[e], fHi - fLo, (unsigned)nQty, fCv, fFirst);
    }

    printf("\n  correlation of the response curves - 1.00 means one effect\n");
    printf("            chorus  flanger   phaser\n");

    for (e = 0U; e < MP_FX_QTY; e++)
    {
        U8 f;

        printf("  %s ", aName[e]);

        for (f = 0U; f < MP_FX_QTY; f++)
        {
            printf("%8.2f", MpCorr(aDb[e], aDb[f]));
        }

        printf("\n");
    }

    printf("\n");
}

/****************************************** end of file *******************************************/
