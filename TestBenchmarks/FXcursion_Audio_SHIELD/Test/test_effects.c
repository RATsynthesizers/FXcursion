/**
 * @file      test_effects.c
 *
 * @details   Delay tap position, tempo arithmetic, looper transport.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "audio_sys.h"
#include "grid.h"
#include "params.h"
#include "looper.h"
#include "Effects/fx_common.h"
#include "Effects/fx_modulation.h"
#include "Effects/fx_phaser.h"
#include "Effects/fx_tremolo.h"


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


void Test_Tempo(void)
{
    TEST_BEGIN("bar length: 4/4 and 6/8 at 120 BPM");
    CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);
    /* 4 quarters at 0.5 s = 2.000 s = 96000 frames */
    CHECK_EQ_U32(Params_BarFrames(), 96000UL);

    CHECK(Params_SetTempo(1200U, 6U, 8U) == RESULT_OK);
    /* six eighths = three quarters = 1.500 s = 72000 frames */
    CHECK_EQ_U32(Params_BarFrames(), 72000UL);

    CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);
    TEST_END();

    TEST_BEGIN("tempo bounds are enforced");
    CHECK(Params_SetTempo(10U, 4U, 4U)   != RESULT_OK);     /* 1 BPM  */
    CHECK(Params_SetTempo(9000U, 4U, 4U) != RESULT_OK);     /* 900 BPM */
    CHECK(Params_SetTempo(1200U, 0U, 4U) != RESULT_OK);
    CHECK(Params_SetTempo(1200U, 4U, 5U) != RESULT_OK);     /* x/5 is not a thing */
    CHECK_EQ_U32(Params_BarFrames(), 96000UL);              /* unchanged */
    TEST_END();

    TEST_BEGIN("synced division resolves to the right time");
    {
        FX_PARAM tParam;

        tParam.fValue       = 0.0f;
        tParam.bSync        = (U8)TRUE;
        tParam.eDivision    = (U8)DIV_1_4;
        tParam.nReserved[0] = 0U;
        tParam.nReserved[1] = 0U;

        /* a quarter note at 120 BPM is 0.5 s */
        CHECK_NEAR(FxParam_TimeSec(&tParam, Params_Tempo(), 0.001f, 10.0f), 0.5, 1e-6);

        tParam.eDivision = (U8)DIV_1_8;
        CHECK_NEAR(FxParam_TimeSec(&tParam, Params_Tempo(), 0.001f, 10.0f), 0.25, 1e-6);

        tParam.eDivision = (U8)DIV_1_8D;                 /* dotted eighth */
        CHECK_NEAR(FxParam_TimeSec(&tParam, Params_Tempo(), 0.001f, 10.0f), 0.375, 1e-6);

        /* a rate parameter is the reciprocal */
        tParam.eDivision = (U8)DIV_1_4;
        CHECK_NEAR(FxParam_RateHz(&tParam, Params_Tempo(), 0.01f, 100.0f), 2.0, 1e-5);

        /* a division longer than the effect can store must be clamped, not
         * allowed to read outside the buffer */
        tParam.eDivision = (U8)DIV_1_1;                  /* whole note = 2 s */
        CHECK_NEAR(FxParam_TimeSec(&tParam, Params_Tempo(), 0.02f, 0.5f), 0.5, 1e-6);
    }
    TEST_END();

    TEST_BEGIN("free and synced rate meet at the ends");
    {
        /*
         * The free range is supposed to span exactly what the divisions span,
         * so the two modes agree at the ends of the travel. It did not: the
         * hard clamp was 0.05..8 Hz, and 8 Hz at 120 BPM is a 1/16 note - so
         * the top of the free knob landed a whole octave short of the 1/32 that
         * sync offered, which is exactly what was reported.
         *
         * The bounds now sit outside the entire legal tempo range (1/1 at
         * 20 BPM is 0.083 Hz, 1/32 at 400 BPM is 53.3 Hz), which is the only
         * way the clamp can be guaranteed never to bite. Checking the extremes
         * of that range is the point - 120 BPM alone would have passed with the
         * old bounds at the bottom end.
         *
         * The expected value is worked out HERE from the division table rather
         * than read back from FxParam_RateHz in sync mode. That matters: the
         * clamp is applied on the way out of BOTH paths, so a ceiling that is
         * too low squashes the free and the synced answer to the same number
         * and a free-versus-synced comparison sails straight through. The
         * mismatch was only ever visible because the UI labels the division
         * from the unclamped table. Compare against the unclamped truth and the
         * test can actually see it.
         */
        static const U16     anBpm10[3] = { 200U, 1200U, 4000U };
        static const FLOAT32 afMin[3]   = { MOD_RATE_MIN_HZ, PHASER_RATE_MIN_HZ,
                                            TREM_RATE_MIN_HZ };
        static const FLOAT32 afMax[3]   = { MOD_RATE_MAX_HZ, PHASER_RATE_MAX_HZ,
                                            TREM_RATE_MAX_HZ };
        U8 t;
        U8 e;

        for (t = 0U; t < 3U; t++)
        {
            CHECK(Params_SetTempo(anBpm10[t], 4U, 4U) == RESULT_OK);

            {
                const double fQuarter = (double)Tempo_QuarterSec(Params_Tempo());
                const double fFastHz  = 1.0 / ((double)g_aDivQuarters[DIV_1_32] * fQuarter);
                const double fSlowHz  = 1.0 / ((double)g_aDivQuarters[DIV_1_1]  * fQuarter);

                for (e = 0U; e < 3U; e++)
                {
                    FX_PARAM tFree;

                    tFree.bSync        = (U8)FALSE;
                    tFree.eDivision    = (U8)DIV_1_4;
                    tFree.fValue       = 0.0f;
                    tFree.nReserved[0] = 0U;
                    tFree.nReserved[1] = 0U;

                    /* bottom of the knob is the SLOWEST division */
                    CHECK_NEAR(FxParam_RateHz(&tFree, Params_Tempo(), afMin[e], afMax[e]),
                               fSlowHz, fSlowHz * 1e-4);

                    /* top of the knob is the FASTEST */
                    tFree.fValue = 1.0f;
                    CHECK_NEAR(FxParam_RateHz(&tFree, Params_Tempo(), afMin[e], afMax[e]),
                               fFastHz, fFastHz * 1e-4);
                }
            }
        }

        CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);
    }
    TEST_END();
}


void Test_Delay(void)
{
    PROTO_CFG tCfg;
    PROTO_ACK tAck;
    U32       nBlock;
    U16       i;
    S32       nFoundAt = -1;
    S32       nPeak    = 0;

    /* A 1/16 note at 120 BPM is exactly 0.125 s = 6000 frames, and every value
     * in that chain is exactly representable in FLOAT32 - so the expected tap
     * position is an exact integer rather than something to fuzz around. */
    TEST_BEGIN("delay tap lands on the exact sample");

    CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);

    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
    tCfg.aSlot[0][0]   = (U8)BLOCK_FX;
    tCfg.aFxSlot[0][0] = (U8)FX_DELAY_M;
    tCfg.aFxEnabled[0] = 0x01U;
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    /* time = synced 1/16, feedback = 0, tone = wide open, mix = 100% wet */
    SetParam(0U, (U8)FX_DELAY_M, (U8)FX_DELAYM_P_TIME,     0U,     (U8)TRUE,  (U8)DIV_1_16);
    SetParam(0U, (U8)FX_DELAY_M, (U8)FX_DELAYM_P_FEEDBACK, 0U,     (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, (U8)FX_DELAY_M, (U8)FX_DELAYM_P_TONE,     65535U, (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, (U8)FX_DELAY_M, (U8)FX_DELAYM_P_MIX,      65535U, (U8)FALSE, (U8)DIV_1_4);

    /* Impulse in the first sample of the first block, silence after. */
    Silence();
    aIn[0] = 4194304L;                                  /* 0.5 full scale */

    for (nBlock = 0UL; nBlock < 128UL; nBlock++)
    {
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        Silence();

        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            const S32 nVal = aOut[(U32)i * AUDIO_CH_QTY];

            if ((nVal > 1000L) && (nFoundAt < 0))
            {
                nFoundAt = (S32)((nBlock * AUDIO_BLOCK_FRAMES) + i);
                nPeak    = nVal;
            }
        }
    }

    /* 1/16 at 120 BPM = 0.125 s = 6000 frames */
    CHECK_EQ_U32((U32)nFoundAt, 6000UL);

    /* Feedback is zero and mix is fully wet, so the repeat is the input
     * unchanged - no soft clipping on the way into the line. */
    CHECK_NEAR((double)nPeak, 4194304.0, 4.0);
    TEST_END();

    TEST_BEGIN("delay stays in bounds at every setting");
    {
        U16 nStep;

        /* Sweep the time control across its whole range, including a synced
         * division far longer than the line, and check nothing explodes. */
        for (nStep = 0U; nStep <= 16U; nStep++)
        {
            SetParam(0U, (U8)FX_DELAY_M, (U8)FX_DELAYM_P_TIME,
                     (U16)(nStep * 4095U), (U8)FALSE, (U8)DIV_1_4);
            SetParam(0U, (U8)FX_DELAY_M, (U8)FX_DELAYM_P_FEEDBACK, 65535U,
                     (U8)FALSE, (U8)DIV_1_4);

            for (nBlock = 0UL; nBlock < 8UL; nBlock++)
            {
                Silence();
                aIn[0] = 8000000L;
                AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

                for (i = 0U; i < (AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY); i++)
                {
                    CHECK(aOut[i] <= 8388607L);
                    CHECK(aOut[i] >= -8388608L);
                }
            }
        }

        /* And a synced whole note, which is 2 s - inside the 4 s line - followed
         * by a very slow tempo where a whole note would be 6 s and must clamp. */
        SetParam(0U, (U8)FX_DELAY_M, (U8)FX_DELAYM_P_TIME, 0U, (U8)TRUE, (U8)DIV_1_1);
        CHECK(Params_SetTempo(400U, 4U, 4U) == RESULT_OK);      /* 40 BPM */

        for (nBlock = 0UL; nBlock < 8UL; nBlock++)
        {
            Silence();
            AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        }
        CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);
    }
    TEST_END();
}


void Test_DelayStereo(void)
{
    PROTO_CFG tCfg;
    PROTO_ACK tAck;
    U32       nBlock;
    U16       i;

    /* Energy in each plane, in 6000-frame windows around each expected repeat. */
    double aEnergyL[4] = { 0.0, 0.0, 0.0, 0.0 };
    double aEnergyR[4] = { 0.0, 0.0, 0.0, 0.0 };

    /* ---- ping-pong: the repeats alternate between the planes ---------------- */
    TEST_BEGIN("stereo delay ping-pongs across the planes");

    CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);

    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_2_STEREO);
    tCfg.aSlot[0][0]   = (U8)BLOCK_FX;
    tCfg.aFxSlot[0][0] = (U8)FX_DELAY_S;        /* the STEREO type - see fx_defs.h */
    tCfg.aFxEnabled[0] = 0x01U;
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    /* 1/16 at 120 BPM = 6000 frames. Fully wet, no spread, full ping-pong. */
    SetParam(0U, (U8)FX_DELAY_S, (U8)FX_DELAYS_P_TIME,     0U,     (U8)TRUE,  (U8)DIV_1_16);
    SetParam(0U, (U8)FX_DELAY_S, (U8)FX_DELAYS_P_FEEDBACK, 55000U, (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, (U8)FX_DELAY_S, (U8)FX_DELAYS_P_TONE,     65535U, (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, (U8)FX_DELAY_S, (U8)FX_DELAYS_P_MIX,      65535U, (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, (U8)FX_DELAY_S, (U8)FX_DELAYS_P_PINGPONG, 65535U, (U8)FALSE, (U8)DIV_1_4);
    SetParam(0U, (U8)FX_DELAY_S, (U8)FX_DELAYS_P_SPREAD,   0U,     (U8)FALSE, (U8)DIV_1_4);

    /* One impulse, LEFT plane only. */
    Silence();
    aIn[0] = 4194304L;

    for (nBlock = 0UL; nBlock < 500UL; nBlock++)
    {
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        Silence();

        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            const U32    nFrame = (nBlock * AUDIO_BLOCK_FRAMES) + i;
            const U32    nSlot  = nFrame / 6000UL;
            const double fL     = (double)aOut[(U32)i * AUDIO_CH_QTY];
            const double fR     = (double)aOut[((U32)i * AUDIO_CH_QTY) + 1U];

            if (nSlot < 4UL)
            {
                aEnergyL[nSlot] += fL * fL;
                aEnergyR[nSlot] += fR * fR;
            }
        }
    }

    /* Window 1 is the first repeat: LEFT, because that is where the impulse went.
     * Window 2 is the second repeat: RIGHT, because full ping-pong routes each
     * plane's regeneration into the OTHER plane's line. Window 3 is left again. */
    CHECK(aEnergyL[1] > (aEnergyR[1] * 50.0));      /* repeat 1 is clearly left  */
    CHECK(aEnergyR[2] > (aEnergyL[2] * 50.0));      /* repeat 2 is clearly right */
    CHECK(aEnergyL[3] > (aEnergyR[3] * 50.0));      /* repeat 3 is left again    */
    TEST_END();

    /* ---- ping-pong at zero keeps each plane to itself ----------------------- */
    TEST_BEGIN("ping-pong at zero stays in one plane");

    /* Same effect, same chain, one parameter different. A width flag on a shared
     * type could not express this: there is no mono meaning for the control. */
    SetParam(0U, (U8)FX_DELAY_S, (U8)FX_DELAYS_P_PINGPONG, 0U, (U8)FALSE, (U8)DIV_1_4);

    /* Clear the tails from the previous run by REMOVING the effect and putting it
     * back. Re-applying an identical grid deliberately does NOT reset anything -
     * only a newly added effect is reset, so that a configuration message which
     * happens to repeat itself cannot chop off somebody's delay tail. */
    {
        PROTO_CFG tEmpty;

        Test_MakeDefaultCfg(&tEmpty, (U8)TOPO_2_STEREO);
        CHECK(Grid_Apply(&tEmpty, &tAck) == RESULT_OK);
    }
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    for (i = 0U; i < 4U; i++)
    {
        aEnergyL[i] = 0.0;
        aEnergyR[i] = 0.0;
    }

    Silence();
    aIn[0] = 4194304L;

    for (nBlock = 0UL; nBlock < 500UL; nBlock++)
    {
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
        Silence();

        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            const U32    nFrame = (nBlock * AUDIO_BLOCK_FRAMES) + i;
            const U32    nSlot  = nFrame / 6000UL;
            const double fL     = (double)aOut[(U32)i * AUDIO_CH_QTY];
            const double fR     = (double)aOut[((U32)i * AUDIO_CH_QTY) + 1U];

            if (nSlot < 4UL)
            {
                aEnergyL[nSlot] += fL * fL;
                aEnergyR[nSlot] += fR * fR;
            }
        }
    }

    CHECK(aEnergyL[1] > (aEnergyR[1] * 50.0));
    CHECK(aEnergyL[2] > (aEnergyR[2] * 50.0));      /* still left, not crossed   */
    CHECK(aEnergyL[3] > (aEnergyR[3] * 50.0));
    TEST_END();
}


void Test_Looper(void)
{
    PROTO_CFG tCfg;
    PROTO_ACK tAck;
    U32       nBlock;
    U16       i;

    TEST_BEGIN("looper records a bar and plays it back");

    CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);

    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
    tCfg.aSlot[0][0]    = (U8)BLOCK_LOOPER;
    tCfg.aLoopBars[0]   = 1U;                   /* 1 bar = 96000 frames */
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    /* Recording is refused until a length exists, and accepted once it does. */
    CHECK(Looper_Transport(0U, (U8)TRANSPORT_RECORD) == RESULT_OK);

    /* Record DC for the whole bar. 96000 / 64 = 1500 blocks. */
    for (nBlock = 0UL; nBlock < 1500UL; nBlock++)
    {
        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            aIn[((U32)i * AUDIO_CH_QTY) + 0U] = 2000000L;
            aIn[((U32)i * AUDIO_CH_QTY) + 1U] = 0L;
            aIn[((U32)i * AUDIO_CH_QTY) + 2U] = 0L;
            aIn[((U32)i * AUDIO_CH_QTY) + 3U] = 0L;
        }
        AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

        /* While recording, the output must stay dry. */
        if (nBlock == 10UL)
        {
            CHECK_NEAR((double)aOut[0], 2000000.0, 4.0);
        }
    }

    /* One full pass done: the transport should have flipped to PLAY. */
    {
        PROTO_TELEMETRY tTelem;

        AudioSys_GetTelemetry(&tTelem);
        CHECK_EQ_U32(tTelem.aTransport[0], (U8)TRANSPORT_PLAY);
        CHECK_EQ_U32(tTelem.aLoopLen[0],   96000UL);
    }

    /* Now feed silence; the loop must play back what was recorded. */
    Silence();
    AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);
    CHECK_NEAR((double)aOut[0], 2000000.0, 300.0);      /* packed 24-bit round trip */

    /* The other chain of the same pair shares the length but not the transport. */
    {
        PROTO_TELEMETRY tTelem;

        AudioSys_GetTelemetry(&tTelem);
        CHECK_EQ_U32(tTelem.aTransport[1], (U8)TRANSPORT_STOP);
    }

    /* Length may not change while the pair is running. */
    tCfg.aLoopBars[0] = 4U;
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);
    {
        PROTO_TELEMETRY tTelem;

        AudioSys_GetTelemetry(&tTelem);
        CHECK_EQ_U32(tTelem.aLoopLen[0], 96000UL);      /* refused, still 1 bar */
    }

    /* Stop, then it may change. */
    CHECK(Looper_Transport(0U, (U8)TRANSPORT_STOP) == RESULT_OK);
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);
    {
        PROTO_TELEMETRY tTelem;

        AudioSys_GetTelemetry(&tTelem);
        CHECK_EQ_U32(tTelem.aLoopLen[0], 384000UL);     /* 4 bars */
    }
    TEST_END();
}


/**
 * @brief Put a width-correct FX block in every chain of the given topology.
 *
 * This is exactly what the interface has to do, and it is one line per effect
 * because both firmwares share g_aFxDesc.
 */
static void FillChainsForTopology(PROTO_CFG* const pCfg, const U8 eTopo)
{
    U8 nChain;

    for (nChain = 0U; nChain < CHAIN_MAX_QTY; nChain++)
    {
        const U8 nW = g_aTopology[eTopo].aChain[nChain].nWidth;

        if (nW != 0U)                       /* chain present in this topology */
        {
            pCfg->aSlot[nChain][0]   = (U8)BLOCK_FX;
            pCfg->aFxSlot[nChain][0] = FX_VARIANT_FOR_WIDTH(FX_DELAY_M, nW);
            pCfg->aFxSlot[nChain][1] = FX_VARIANT_FOR_WIDTH(FX_OVERDRIVE_M, nW);
            pCfg->aFxEnabled[nChain] = 0x03U;
        }
    }
}


void Test_Topology(void)
{
    PROTO_CFG tCfg;
    PROTO_ACK tAck;
    U16       i;

    /* ---- a running transport must not survive a plane remap ----------------- */
    TEST_BEGIN("topology change stops loop transports");

    CHECK(Params_SetTempo(1200U, 4U, 4U) == RESULT_OK);

    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
    tCfg.aSlot[1][0]  = (U8)BLOCK_LOOPER;       /* mono chain 1 == plane 1 */
    tCfg.aLoopBars[0] = 1U;
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);
    CHECK(Looper_Transport(1U, (U8)TRANSPORT_RECORD) == RESULT_OK);

    {
        PROTO_TELEMETRY tTelem;
        AudioSys_GetTelemetry(&tTelem);
        CHECK_EQ_U32(tTelem.aTransport[1], (U8)TRANSPORT_RECORD);
    }

    /* Merge the chains. Chain 1 now means planes 2 and 3, so a transport left
     * running would be advancing audio it never recorded. */
    Test_MakeDefaultCfg(&tCfg, (U8)TOPO_2_STEREO);
    tCfg.aSlot[1][0]  = (U8)BLOCK_LOOPER;
    tCfg.aLoopBars[0] = 1U;
    CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

    {
        PROTO_TELEMETRY tTelem;
        U8              c;

        AudioSys_GetTelemetry(&tTelem);
        for (c = 0U; c < CHAIN_MAX_QTY; c++)
        {
            CHECK_EQ_U32(tTelem.aTransport[c], (U8)TRANSPORT_STOP);
        }
        /* Loop CONTENT and length are deliberately kept - the plane buffers
         * themselves never move. */
        CHECK_EQ_U32(tTelem.aLoopLen[0], 96000UL);
    }
    TEST_END();

    /* ---- topology may change freely at runtime, in any order ---------------- */
    TEST_BEGIN("every topology transition is accepted");
    {
        U8 eFrom;
        U8 eTo;

        for (eFrom = 0U; eFrom < (U8)TOPO_QTY; eFrom++)
        {
            for (eTo = 0U; eTo < (U8)TOPO_QTY; eTo++)
            {
                U8 nChain;

                /* Start from a loaded configuration in eFrom ... */
                Test_MakeDefaultCfg(&tCfg, eFrom);
                FillChainsForTopology(&tCfg, eFrom);
                CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);

                Silence();
                aIn[0] = 4000000L;
                AudioSys_ProcessBlock(aIn, aOut, AUDIO_BLOCK_FRAMES);

                /* ... switch to eTo. The effects have to be re-chosen for the
                 * new widths - that is the whole point of the mono/stereo split,
                 * and FX_VARIANT_FOR_WIDTH is how the interface does it. */
                Test_MakeDefaultCfg(&tCfg, eTo);
                FillChainsForTopology(&tCfg, eTo);
                CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);
                CHECK_EQ_U32(Grid_Active()->eTopology, eTo);

                /* Whatever the interface sent is what is running: the audio side
                 * never invents or empties a layout. */
                for (nChain = 0U; nChain < Grid_Active()->nChainQty; nChain++)
                {
                    const U8 nW = Grid_Active()->aWidth[nChain];

                    CHECK_EQ_U32(Grid_Active()->aFxSlot[nChain][0],
                                 FX_VARIANT_FOR_WIDTH(FX_DELAY_M, nW));
                    CHECK_EQ_U32(Grid_Active()->aFxSlot[nChain][1],
                                 FX_VARIANT_FOR_WIDTH(FX_OVERDRIVE_M, nW));
                }

                /* ... and audio still comes out finite. */
                Silence();
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

    /* ---- the width contract is declared and enforced ------------------------ */
    TEST_BEGIN("every effect is mono-only or stereo-only");
    {
        U8 eFx;

        for (eFx = 0U; eFx < (U8)FX_TYPE_QTY; eFx++)
        {
            /* Exactly one width, never both. */
            CHECK((g_aFxDesc[eFx].nWidth == 1U) || (g_aFxDesc[eFx].nWidth == CHAIN_MAX_WIDTH));
            CHECK(g_aFxDesc[eFx].nParamQty <= FX_PARAM_QTY);
            CHECK(g_aFxDesc[eFx].pParam != NULL_PTR);
            CHECK(g_aFxEntry[eFx].pfProcess != NULL_PTR);
            CHECK(g_aFxEntry[eFx].pfReset != NULL_PTR);
        }

        /* The two variants of one effect are adjacent, mono at an even id. That
         * is what FX_VARIANT_FOR_WIDTH relies on. */
        for (eFx = 0U; eFx < (U8)FX_TYPE_QTY; eFx += 2U)
        {
            CHECK_EQ_U32(g_aFxDesc[eFx].nWidth,      1U);
            CHECK_EQ_U32(g_aFxDesc[eFx + 1U].nWidth, CHAIN_MAX_WIDTH);

            /* Same display name - the GUI shows one entry for the pair. */
            CHECK(strcmp(g_aFxDesc[eFx].pName, g_aFxDesc[eFx + 1U].pName) == 0);

            /* And the stereo variant genuinely differs: at least one parameter
             * more than the mono one. If it did not, it should not exist. */
            CHECK(g_aFxDesc[eFx + 1U].nParamQty > g_aFxDesc[eFx].nParamQty);

            CHECK_EQ_U32(FX_VARIANT_FOR_WIDTH(eFx, 1U),              eFx);
            CHECK_EQ_U32(FX_VARIANT_FOR_WIDTH(eFx, CHAIN_MAX_WIDTH), eFx + 1U);
            CHECK(FX_IS_STEREO(eFx) == FALSE);
            CHECK(FX_IS_STEREO(eFx + 1U) != FALSE);
        }
    }
    TEST_END();

    /* ---- widths must match exactly ------------------------------------------ */
    TEST_BEGIN("width mismatch is refused");
    {
        /* TOPO_2_STEREO chain 0 is two planes wide, so the MONO delay is simply
         * not a legal thing to put in it. Neither can be converted into the
         * other, which is why a chain that changes width must arrive cleared or
         * repopulated with the other variants. */
        Test_MakeDefaultCfg(&tCfg, (U8)TOPO_2_STEREO);
        tCfg.aSlot[0][0]   = (U8)BLOCK_FX;
        tCfg.aFxSlot[0][0] = (U8)FX_DELAY_M;
        CHECK(Grid_Apply(&tCfg, &tAck) != RESULT_OK);
        CHECK_EQ_U32(tAck.eResult, (U8)PROTO_RES_BAD_WIDTH);

        /* and the other way round */
        Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
        tCfg.aSlot[0][0]   = (U8)BLOCK_FX;
        tCfg.aFxSlot[0][0] = (U8)FX_REVERB_S;
        CHECK(Grid_Apply(&tCfg, &tAck) != RESULT_OK);
        CHECK_EQ_U32(tAck.eResult, (U8)PROTO_RES_BAD_WIDTH);

        /* the correct variants are accepted */
        Test_MakeDefaultCfg(&tCfg, (U8)TOPO_2_STEREO);
        tCfg.aSlot[0][0]   = (U8)BLOCK_FX;
        tCfg.aFxSlot[0][0] = (U8)FX_DELAY_S;
        tCfg.aFxSlot[0][1] = (U8)FX_REVERB_S;
        CHECK(Grid_Apply(&tCfg, &tAck) == RESULT_OK);
    }
    TEST_END();
}
