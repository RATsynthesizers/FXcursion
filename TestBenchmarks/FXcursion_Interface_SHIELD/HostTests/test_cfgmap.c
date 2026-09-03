/*
 * Host harness for the GUI -> PROTO_CFG mapping added to Model.cpp.
 *
 * Compiles the REAL shared headers, so the enum values and the geometry
 * constants are the ones the firmware uses. The three mapping functions are
 * replicated verbatim from Model.cpp - they are pure functions of the two
 * stereo flags, which is exactly why they were worth splitting out.
 *
 * The point is the invariants. The audio side documents that chain widths
 * always sum to AUDIO_CH_QTY in every topology, and that chains are numbered
 * in plane order; there is no audio board to check that against yet, so it
 * gets checked here.
 */
#include <stdio.h>
#include <string.h>

#include "fx_protocol.h"

static int nChecks = 0;
static int nFails  = 0;

#define CHECK(cond, fmt, ...)                                                \
    do {                                                                     \
        nChecks++;                                                           \
        if (!(cond)) {                                                       \
            nFails++;                                                        \
            printf("FAIL line %d: " fmt "\n", __LINE__, __VA_ARGS__);         \
        }                                                                    \
    } while (0)

/* --- the GUI's own enums, in the order TextKeysAndLanguages.hpp generates --- */

typedef enum {
    T_EMPTYEFFECT = 100,        /* base is arbitrary; only deltas matter */
    T_CHORUSEFFECT,
    T_COMPRESSOREFFECT,
    T_DELAYEFFECT,
    T_DISTORTIONEFFECT,
    T_FLANGEREFFECT,
    T_OVERDRIVEEFFECT,
    T_PHASEREFFECT,
    T_REVERBEFFECT,
    T_TREMOLOEFFECT,
    T_VIBRATOEFFECT
} TEXTS;

typedef enum {
    CHANNEL_MONO_1 = 0, CHANNEL_MONO_2 = 1, CHANNEL_MONO_3 = 2,
    CHANNEL_MONO_4 = 3, CHANNEL_STEREO_1 = 4, CHANNEL_STEREO_2 = 5
} ChannelType;

#define CHANNELS_NUM 6

/* --- replicated from Model.cpp --- */

static U8 FxTypeBaseFromTexts(const TEXTS e)
{
    if ((e < T_CHORUSEFFECT) || (e > T_VIBRATOEFFECT)) return (U8)FX_TYPE_NONE;
    return (U8)((U8)FX_CHORUS_M + 2U * (U8)(e - T_CHORUSEFFECT));
}

static U8 CurrentTopology(int bSt1, int bSt2)
{
    if (bSt1) return bSt2 ? (U8)TOPO_2_STEREO : (U8)TOPO_ST1_2MONO;
    return bSt2 ? (U8)TOPO_2MONO_ST2 : (U8)TOPO_4_MONO;
}

static U8 ChainWidthForChannel(ChannelType e)
{
    return (e <= CHANNEL_MONO_4) ? 1U : (U8)CHAIN_MAX_WIDTH;
}

static S8 ProtoChainForChannel(ChannelType e, int bSt1, int bSt2)
{
    const U8 nGroup1Chains = bSt1 ? 1U : 2U;

    switch (e) {
    case CHANNEL_MONO_1:   return (!bSt1) ? (S8)0 : (S8)-1;
    case CHANNEL_MONO_2:   return (!bSt1) ? (S8)1 : (S8)-1;
    case CHANNEL_STEREO_1: return ( bSt1) ? (S8)0 : (S8)-1;
    case CHANNEL_MONO_3:   return (!bSt2) ? (S8)nGroup1Chains : (S8)-1;
    case CHANNEL_MONO_4:   return (!bSt2) ? (S8)(nGroup1Chains + 1U) : (S8)-1;
    case CHANNEL_STEREO_2: return ( bSt2) ? (S8)nGroup1Chains : (S8)-1;
    default: return (S8)-1;
    }
}

/* --- what the audio side's topology comments say, spelled out --- */

typedef struct { int bSt1, bSt2; U8 eTopo; int nChains; ChannelType aChain[4]; } EXPECT;

static const EXPECT aExpect[4] = {
    /* TOPO_4_MONO      m1 | m2 | m3 | m4      */
    { 0, 0, (U8)TOPO_4_MONO,    4, { CHANNEL_MONO_1,   CHANNEL_MONO_2,   CHANNEL_MONO_3, CHANNEL_MONO_4 } },
    /* TOPO_ST1_2MONO   (m1+m2) | m3 | m4      */
    { 1, 0, (U8)TOPO_ST1_2MONO, 3, { CHANNEL_STEREO_1, CHANNEL_MONO_3,   CHANNEL_MONO_4, (ChannelType)0 } },
    /* TOPO_2MONO_ST2   m1 | m2 | (m3+m4)      */
    { 0, 1, (U8)TOPO_2MONO_ST2, 3, { CHANNEL_MONO_1,   CHANNEL_MONO_2,   CHANNEL_STEREO_2, (ChannelType)0 } },
    /* TOPO_2_STEREO    (m1+m2) | (m3+m4)      */
    { 1, 1, (U8)TOPO_2_STEREO,  2, { CHANNEL_STEREO_1, CHANNEL_STEREO_2, (ChannelType)0, (ChannelType)0 } },
};

int main(void)
{
    /* --- the shared static assertion, evaluated here too --- */
    CHECK(((U8)FX_CHORUS_M + 2U * (U8)(T_VIBRATOEFFECT - T_CHORUSEFFECT))
              == (U8)FX_VIBRATO_M,
          "GUI text order no longer matches the FX pool (%u vs %u)",
          (unsigned)((U8)FX_CHORUS_M + 2U * (U8)(T_VIBRATOEFFECT - T_CHORUSEFFECT)),
          (unsigned)FX_VIBRATO_M);

    /* --- every GUI effect maps to the right mono id, and nothing else does --- */
    {
        static const struct { TEXTS e; U8 nFx; } aMap[] = {
            { T_CHORUSEFFECT,     (U8)FX_CHORUS_M     },
            { T_COMPRESSOREFFECT, (U8)FX_COMPRESSOR_M },
            { T_DELAYEFFECT,      (U8)FX_DELAY_M      },
            { T_DISTORTIONEFFECT, (U8)FX_DISTORTION_M },
            { T_FLANGEREFFECT,    (U8)FX_FLANGER_M    },
            { T_OVERDRIVEEFFECT,  (U8)FX_OVERDRIVE_M  },
            { T_PHASEREFFECT,     (U8)FX_PHASER_M     },
            { T_REVERBEFFECT,     (U8)FX_REVERB_M     },
            { T_TREMOLOEFFECT,    (U8)FX_TREMOLO_M    },
            { T_VIBRATOEFFECT,    (U8)FX_VIBRATO_M    },
        };
        for (unsigned i = 0; i < sizeof(aMap)/sizeof(aMap[0]); i++) {
            U8 got = FxTypeBaseFromTexts(aMap[i].e);
            CHECK(got == aMap[i].nFx, "effect %u mapped to %u, expected %u",
                  i, (unsigned)got, (unsigned)aMap[i].nFx);
            CHECK(!FX_IS_STEREO(got), "base id %u is a stereo id", (unsigned)got);
            CHECK(FX_VARIANT_FOR_WIDTH(got, 2) == (U8)(got + 1),
                  "stereo variant of %u wrong", (unsigned)got);
            CHECK(FX_VARIANT_FOR_WIDTH(got, 1) == got,
                  "mono variant of %u wrong", (unsigned)got);
        }
        CHECK(FxTypeBaseFromTexts(T_EMPTYEFFECT) == (U8)FX_TYPE_NONE,
              "empty effect mapped to %u", (unsigned)FxTypeBaseFromTexts(T_EMPTYEFFECT));
    }

    /* --- all four topologies: id, chain order, widths, uniqueness --- */
    for (int t = 0; t < 4; t++) {
        const EXPECT* x = &aExpect[t];

        CHECK(CurrentTopology(x->bSt1, x->bSt2) == x->eTopo,
              "topology for (%d,%d) is %u, expected %u",
              x->bSt1, x->bSt2,
              (unsigned)CurrentTopology(x->bSt1, x->bSt2), (unsigned)x->eTopo);

        /* Each expected chain index resolves to the expected channel. */
        for (int c = 0; c < x->nChains; c++) {
            S8 got = ProtoChainForChannel(x->aChain[c], x->bSt1, x->bSt2);
            CHECK(got == (S8)c, "topo %u: channel %d should be chain %d, got %d",
                  (unsigned)x->eTopo, (int)x->aChain[c], c, (int)got);
        }

        /* Widths sum to AUDIO_CH_QTY - the invariant fx_defs.h states. */
        int nPlaneSum = 0, nActive = 0;
        S8 aSeen[CHAIN_MAX_QTY];
        memset(aSeen, 0, sizeof aSeen);

        for (int ch = 0; ch < CHANNELS_NUM; ch++) {
            S8 nChain = ProtoChainForChannel((ChannelType)ch, x->bSt1, x->bSt2);
            if (nChain < 0) continue;

            nActive++;
            nPlaneSum += ChainWidthForChannel((ChannelType)ch);

            CHECK(nChain < (S8)CHAIN_MAX_QTY, "topo %u: chain index %d out of range",
                  (unsigned)x->eTopo, (int)nChain);
            CHECK(aSeen[nChain] == 0, "topo %u: chain %d claimed twice",
                  (unsigned)x->eTopo, (int)nChain);
            aSeen[nChain] = 1;
        }

        CHECK(nPlaneSum == (int)AUDIO_CH_QTY,
              "topo %u: widths sum to %d, must be %d",
              (unsigned)x->eTopo, nPlaneSum, (int)AUDIO_CH_QTY);
        CHECK(nActive == x->nChains,
              "topo %u: %d active chains, expected %d",
              (unsigned)x->eTopo, nActive, x->nChains);

        /* Active chains are 0..n-1 with no holes, so aSlot rows below
           nChains are the used ones and the rest stay BLOCK_NONE. */
        for (int c = 0; c < x->nChains; c++)
            CHECK(aSeen[c] == 1, "topo %u: chain %d unused but should exist",
                  (unsigned)x->eTopo, c);
    }

    /* --- value scaling preserves both endpoints and is monotone --- */
    {
        U16 nPrev = 0;
        for (int v = 0; v <= 255; v++) {
            U16 n = (U16)(((U32)v * 65535U) / 255U);
            if (v == 0)   CHECK(n == 0u,     "0 mapped to %u", (unsigned)n);
            if (v == 255) CHECK(n == 65535u, "255 mapped to %u", (unsigned)n);
            if (v > 0)    CHECK(n > nPrev,   "not monotone at %d", v);
            nPrev = n;
        }
    }

    /* --- the mixer column the GUI can produce is always a legal column --- */
    for (int col = -1; col < (int)GRID_SLOT_QTY; col++) {
        S8 nMixerCol = (col < 0) ? GRID_MIXER_COL_NONE : (S8)col;
        CHECK(nMixerCol == GRID_MIXER_COL_NONE || (nMixerCol >= 0 && nMixerCol < (S8)GRID_SLOT_QTY),
              "mixer column %d is neither NONE nor a valid slot", (int)nMixerCol);
    }

    /* --- the effect descriptor table the GUI now reads names and counts from --- */
    {
        /*
         * CustomGaugeBase sizes parameterNameBuffer at 10 UnicodeChars, so a
         * name has at most 9 characters plus a terminator. FX_PARAM_DESC
         * documents them as "<= 11 chars", which is two more than the buffer
         * holds - the gap is real, and this is where it gets caught. If a name
         * ever needs to be longer, widen the buffer in the Designer FIRST.
         */
        const unsigned nNameMax = 9U;

        for (unsigned f = 0; f < (unsigned)FX_TYPE_QTY; f++) {
            const FX_DESC* p = &g_aFxDesc[f];

            CHECK(p->pName != 0, "effect %u has no name", f);
            CHECK(p->nParamQty <= (U8)FX_PARAM_QTY,
                  "effect %u declares %u params, max is %u",
                  f, (unsigned)p->nParamQty, (unsigned)FX_PARAM_QTY);
            CHECK(p->nWidth == 1U || p->nWidth == 2U,
                  "effect %u has width %u", f, (unsigned)p->nWidth);
            CHECK(p->pParam != 0 || p->nParamQty == 0U,
                  "effect %u has %u params but no table", f, (unsigned)p->nParamQty);

            /* The variant layout the GUI relies on to pick mono vs stereo. */
            CHECK(FX_IS_STEREO(f) == ((f & 1U) != 0U),
                  "effect %u breaks the even=mono / odd=stereo layout", f);
            CHECK(p->nWidth == (FX_IS_STEREO(f) ? 2U : 1U),
                  "effect %u width %u disagrees with its id parity",
                  f, (unsigned)p->nWidth);

            for (unsigned i = 0; i < p->nParamQty; i++) {
                CHECK(p->pParam[i].pName != 0,
                      "effect %u param %u has no name", f, i);
                if (p->pParam[i].pName != 0) {
                    CHECK(strlen(p->pParam[i].pName) <= nNameMax,
                          "effect %u param %u name \"%s\" is %u chars, max %u",
                          f, i, p->pParam[i].pName,
                          (unsigned)strlen(p->pParam[i].pName), nNameMax);
                }
            }
        }
    }

    printf("\nPROTO_CFG size %u, PROTO_SET_PARAM size %u, version %u\n",
           (unsigned)sizeof(PROTO_CFG), (unsigned)sizeof(PROTO_SET_PARAM),
           (unsigned)PROTO_VERSION);
    printf("%d checks, %d failures\n", nChecks, nFails);
    return nFails ? 1 : 0;
}
