/**
 * @file      test_revprofile.c
 *
 * @details   Prints the reverb's actual decay curve and echo density.
 *
 *            Diagnostic, not an assertion. "A long decay outlasts a short one"
 *            is true of almost anything and told us nothing when the tail
 *            turned out to be inaudible after two seconds.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "audio_sys.h"
#include "grid.h"
#include "params.h"
#include "Effects/fx_common.h"
#include "Effects/fx_reverb.h"

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
    (void)Params_Set(&t);
}

static void Fresh(const U8 eFx)
{
    PROTO_CFG c; PROTO_ACK a;

    Test_MakeDefaultCfg(&c, (U8)TOPO_4_MONO);
    (void)Grid_Apply(&c, &a);

    Test_MakeDefaultCfg(&c, (U8)TOPO_4_MONO);
    c.aSlot[0][0] = (U8)BLOCK_FX; c.aFxSlot[0][0] = eFx; c.aFxEnabled[0] = 0x01U;
    (void)Grid_Apply(&c, &a);
}


void Test_ReverbProfile(void)
{
    const double fFull = 8388608.0;
    const U32    nBps  = (U32)AUDIO_SAMPLE_RATE_HZ / AUDIO_BLOCK_FRAMES;   /* 750 */
    const U16    aDamp[3] = { 0U, 32768U, 65535U };
    U8           d;

    printf("\n  reverb decay profile, RMS dB below the input impulse\n");
    printf("        damping   0.0s  0.5s  1.0s  1.5s  2.0s  3.0s  4.0s  6.0s  8.0s 12.0s\n");

    for (d = 0U; d < 3U; d++)
    {
        double aLevel[10];
        U8     w;

        /* seconds at which each window starts */
        const double aWhen[10] = { 0.0, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0 };

        Fresh((U8)FX_REVERB_M);

        /* 12 s decay, no pre-delay, full wet */
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DECAY,     65535U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_PREDELAY,  0U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_SIZE,      65535U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DAMPING,   aDamp[d]);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DIFFUSION, 45000U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_MIX,       65535U);

        /* TWO SECONDS OF CONTINUOUS NOISE, then silence.
         *
         * An impulse is the wrong excitation for a twelve second reverb: all of
         * its energy is one sample, spread over half a million, so the tail is
         * inherently tiny and says nothing about what a guitar through it
         * sounds like. A burst lets the network fill up first, which is the
         * state it is actually used in. */
        {
            U32 b; U16 i; U32 nSeed = 12345UL;

            for (b = 0UL; b < (nBps * 2U); b++)
            {
                for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
                {
                    nSeed = (nSeed * 1103515245UL) + 12345UL;
                    aIn[(U32)i * AUDIO_CH_QTY] =
                        (S32)((double)((S32)((nSeed >> 16) & 0x7FFFU) - 16384) * (fFull * 0.25 / 16384.0));
                }
                AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
            }
        }
        Sil();

        {
            double fElapsed = 0.0;

            for (w = 0U; w < 10U; w++)
            {
                double fSum = 0.0;
                U32    nN   = 0UL;
                U32    b;
                U16    i;

                /* run forward to this window */
                while (fElapsed < aWhen[w])
                {
                    AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
                    fElapsed += (double)AUDIO_BLOCK_FRAMES / (double)AUDIO_SAMPLE_RATE_HZ;
                }

                /* measure a tenth of a second */
                for (b = 0UL; b < (nBps / 10U); b++)
                {
                    AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

                    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
                    {
                        const double v = (double)aOut[(U32)i * AUDIO_CH_QTY];
                        fSum += v * v;
                        nN++;
                    }
                }

                fElapsed += 0.1;
                aLevel[w] = (nN > 0UL) ? sqrt(fSum / (double)nN) : 0.0;
            }
        }

        printf("        %5.2f  ", (double)aDamp[d] / 65535.0);
        for (w = 0U; w < 10U; w++)
        {
            /* Referenced to the RMS of the noise that went in, so 0 dB means
               the tail is as loud as the source was. */
            const double fDb = (aLevel[w] > 1.0e-6)
                                   ? 20.0 * log10(aLevel[w] / (fFull * 0.25 / 1.732)) : -999.0;
            printf("%5.0f ", fDb);
        }
        printf("\n");
    }

    /* Echo density: how many samples in the first 50 ms are non-trivial. */
    {
        U32 nHits = 0UL;
        U32 b;
        U16 i;

        Fresh((U8)FX_REVERB_M);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DECAY,     65535U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_PREDELAY,  0U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_SIZE,      65535U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DAMPING,   0U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_DIFFUSION, 45000U);
        SetP((U8)FX_REVERB_M, (U8)FX_REVERBM_P_MIX,       65535U);

        Sil();
        aIn[0] = (S32)(fFull * 0.5);
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        Sil();

        /* SKIP THE PRE-DELAY FIRST.
         *
         * Pre-delay is syncable, so its free minimum follows the tempo - 62.5 ms
         * at 120 BPM - and the first version of this metric measured a window
         * that sat entirely inside it. It reported zero echoes because it was
         * measuring silence, not because the network was empty. */
        for (b = 0UL; b < 90UL; b++)            /* past 120 ms */
        {
            AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        }

        for (b = 0UL; b < 36UL; b++)            /* the next ~50 ms */
        {
            AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

            for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
            {
                if (fabs((double)aOut[(U32)i * AUDIO_CH_QTY]) > (fFull * 0.0005)) { nHits++; }
            }
        }

        printf("\n  echoes above -66 dB in 50 ms after the pre-delay: %lu of %lu\n\n",
               (unsigned long)nHits, (unsigned long)(36UL * AUDIO_BLOCK_FRAMES));
    }
}
