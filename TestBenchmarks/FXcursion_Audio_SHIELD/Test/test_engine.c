/**
 * @file      test_engine.c
 *
 * @details   Identity, grid coverage and mixer behaviour.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "audio_sys.h"
#include "grid.h"
#include "params.h"


static S32 aIn[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY];
static S32 aOut[AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY];


static void FillRamp(void)
{
    U16 i;
    U8  c;

    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        for (c = 0U; c < AUDIO_CH_QTY; c++)
        {
            /* Spread over most of the 24-bit range, different per channel. */
            aIn[((U32)i * AUDIO_CH_QTY) + c] =
                (S32)(((S32)i * 4096) - 131072) * (S32)(c + 1U);
        }
    }
}


void Test_Identity(void)
{
    PROTO_CFG tCfg;
    PROTO_ACK tAck;
    U16       i;
    U8        c;

    /* ---- an empty grid must be bit-exact ------------------------------------ */
    TEST_BEGIN("empty grid is bit-exact");
    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    FillRamp();
    AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        for (c = 0U; c < AUDIO_CH_QTY; c++)
        {
            const U32 n = ((U32)i * AUDIO_CH_QTY) + c;

            CHECK_EQ_U32((U32)aOut[n], (U32)aIn[n]);
        }
    }
    TEST_END();

    /* ---- an FX block with no effects, and a recorder tap, are also transparent */
    TEST_BEGIN("empty FX block and tap are transparent");
    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_2_STEREO);
    tCfg.aSlot[0][0] = (U8)BLOCK_FX;            /* no effects in it */
    tCfg.aSlot[0][1] = (U8)BLOCK_RECORDER;      /* a tap must be inaudible */
    tCfg.aSlot[1][2] = (U8)BLOCK_RECORDER;
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    FillRamp();
    AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        for (c = 0U; c < AUDIO_CH_QTY; c++)
        {
            const U32 n = ((U32)i * AUDIO_CH_QTY) + c;

            CHECK_EQ_U32((U32)aOut[n], (U32)aIn[n]);
        }
    }
    TEST_END();

    /* ---- a bypassed effect must not change the signal ------------------------ */
    TEST_BEGIN("bypassed effect is transparent");
    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
    tCfg.aSlot[0][0]    = (U8)BLOCK_FX;
    tCfg.aFxSlot[0][0]  = (U8)FX_OVERDRIVE_M;
    tCfg.aFxEnabled[0]  = 0x00U;                /* slot 0 disabled */
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    FillRamp();
    AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        CHECK_EQ_U32((U32)aOut[(U32)i * AUDIO_CH_QTY], (U32)aIn[(U32)i * AUDIO_CH_QTY]);
    }
    TEST_END();
}


void Test_Grid(void)
{
    PROTO_CFG tCfg;
    PROTO_ACK tAck;

    /* ---- every legal arrangement produces finite audio ---------------------- */
    TEST_BEGIN("every topology x every mixer column");
    {
        U8 eTopo;
        S8 nMixCol;

        for (eTopo = 0U; eTopo < (U8)TOPO_QTY; eTopo++)
        {
            for (nMixCol = -1; nMixCol < (S8)GRID_SLOT_QTY; nMixCol++)
            {
                U8  nChain;
                U16 i;

                Test_MakeDefaultCfg(&tCfg, eTopo);
                tCfg.nMixerCol = nMixCol;

                for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
                {
                    static const U8 aFill[3] =
                        { (U8)BLOCK_FX, (U8)BLOCK_RECORDER, (U8)BLOCK_LOOPER };
                    U8 nCol;
                    U8 nNext = 0U;

                    /* Exactly one of every block type. With four slots and four
                     * block types the grid is exactly saturated, so this is the
                     * densest legal arrangement. */
                    if (nMixCol >= 0)
                    {
                        tCfg.aSlot[nChain][nMixCol] = (U8)BLOCK_MIXER;
                    }

                    for (nCol = 0U; nCol < GRID_SLOT_QTY; nCol++)
                    {
                        if ((S8)nCol == nMixCol)
                        {
                            continue;
                        }
                        if (nNext < 3U)
                        {
                            tCfg.aSlot[nChain][nCol] = aFill[nNext];
                            nNext++;
                        }
                    }

                    /* Width-correct variants. A mono effect in a stereo chain is
                     * refused, and rightly so - see Test_Topology. This is the
                     * one line the interface has to get right, and it gets it
                     * right from the same shared descriptor table. */
                    {
                        const U8 nW = g_aTopology[eTopo].aChain[nChain].nWidth;

                        if (nW != 0U)
                        {
                            tCfg.aFxSlot[nChain][0] = FX_VARIANT_FOR_WIDTH(FX_OVERDRIVE_M, nW);
                            tCfg.aFxSlot[nChain][1] = FX_VARIANT_FOR_WIDTH(FX_TREMOLO_M, nW);
                            tCfg.aFxSlot[nChain][2] = FX_VARIANT_FOR_WIDTH(FX_AMP_M, nW);
                            tCfg.aFxEnabled[nChain] = 0x07U;
                        }
                    }
                }

                CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

                FillRamp();
                AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

                for (i = 0U; i < (AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY); i++)
                {
                    CHECK(aOut[i] <= 8388607L);
                    CHECK(aOut[i] >= -8388608L);
                }
            }
        }
    }
    TEST_END();

    /* ---- the grid is exactly saturated -------------------------------------- */
    TEST_BEGIN("a chain holds exactly one of every block type");
    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
    tCfg.nMixerCol = 3;
    {
        U8 nChain;

        for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
        {
            tCfg.aSlot[nChain][0] = (U8)BLOCK_FX;
            tCfg.aSlot[nChain][1] = (U8)BLOCK_RECORDER;
            tCfg.aSlot[nChain][2] = (U8)BLOCK_LOOPER;
            tCfg.aSlot[nChain][3] = (U8)BLOCK_MIXER;
        }
    }
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);
    CHECK_EQ_U32(tAck.eResult, (U8)PROTO_RES_OK);
    TEST_END();
}


void Test_Mixer(void)
{
    PROTO_CFG tCfg;
    PROTO_ACK tAck;
    U16       i;

    /* ---- unity diagonal passes audio through -------------------------------- */
    TEST_BEGIN("mixer unity diagonal");
    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
    tCfg.nMixerCol = 0;
    {
        U8 nChain;
        for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
        {
            tCfg.aSlot[nChain][0] = (U8)BLOCK_MIXER;
        }
    }
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    FillRamp();
    /* Let the gain ramps settle. MIX_GAIN_SMOOTH_MS is 20 ms and a block is
     * 1.33 ms, so 400 blocks is about 26 time constants. Anything less and the
     * test is measuring the ramp rather than the gain - which is worth knowing,
     * because it proves the smoothing is actually there. */
    for (i = 0U; i < 400U; i++)
    {
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
    }

    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        const U32 n = (U32)i * AUDIO_CH_QTY;

        /* Within one LSB of the 24-bit range. */
        CHECK_NEAR((double)aOut[n], (double)aIn[n], 2.0);
    }
    TEST_END();

    /* ---- auto gain uses 1/sqrt(N), not 1/N ---------------------------------- */
    TEST_BEGIN("auto gain is 1/sqrt(N)");
    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
    tCfg.nMixerCol = 0;
    tCfg.bAutoGain = (U8)TRUE;
    {
        U8 nChain;
        for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
        {
            tCfg.aSlot[nChain][0] = (U8)BLOCK_MIXER;
        }
        /* chains 0 and 1 both feed chain 0 */
        tCfg.aMixGain[0][1] = 32768U;
    }
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    /* DC on channels 0 and 1, so the sum is predictable. */
    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        aIn[((U32)i * AUDIO_CH_QTY) + 0U] = 1000000L;
        aIn[((U32)i * AUDIO_CH_QTY) + 1U] = 1000000L;
        aIn[((U32)i * AUDIO_CH_QTY) + 2U] = 0L;
        aIn[((U32)i * AUDIO_CH_QTY) + 3U] = 0L;
    }
    for (i = 0U; i < 400U; i++)
    {
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
    }

    /* (1e6 + 1e6) / sqrt(2) = 1414213.6 ; with 1/N it would be 1e6 */
    CHECK_NEAR((double)aOut[0], 1414213.0, 200.0);
    TEST_END();

    /* ---- stereo to mono folds at -3 dB -------------------------------------- */
    TEST_BEGIN("stereo to mono fold is -3 dB");
    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_ST1_2MONO);  /* chain0 stereo, 1 and 2 mono */
    tCfg.nMixerCol = 0;
    {
        U8 nChain;
        for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
        {
            tCfg.aSlot[nChain][0] = (U8)BLOCK_MIXER;
        }
        /* stereo chain 0 feeds mono chain 1, and nothing else does */
        tCfg.aMixGain[1][1] = 0U;
        tCfg.aMixGain[1][0] = 32768U;
    }
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
    {
        aIn[((U32)i * AUDIO_CH_QTY) + 0U] = 1000000L;   /* chain 0 L */
        aIn[((U32)i * AUDIO_CH_QTY) + 1U] = 1000000L;   /* chain 0 R */
        aIn[((U32)i * AUDIO_CH_QTY) + 2U] = 0L;         /* chain 1   */
        aIn[((U32)i * AUDIO_CH_QTY) + 3U] = 0L;
    }
    for (i = 0U; i < 400U; i++)
    {
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
    }

    /* (L + R) * 0.7071 = 1414213 */
    CHECK_NEAR((double)aOut[2], 1414213.0, 200.0);
    TEST_END();
}
