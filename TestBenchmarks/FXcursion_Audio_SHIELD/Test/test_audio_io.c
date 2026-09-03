/**
 * @file      test_audio_io.c
 *
 * @details   The channel map and the headphone bus.
 *
 *            audio_io.c itself cannot be tested here - it is the one file in
 *            the audio path that talks to the HAL. What CAN be tested is
 *            everything it delegates to, and that turns out to be where the
 *            expensive mistakes live:
 *
 *              - 24-bit sign extension. Get it wrong and half of every waveform
 *                reads as a huge positive number. On hardware that looks like a
 *                codec fault, a clock fault or a wiring fault, and it is none of
 *                them. Here it is six boundary values.
 *
 *              - which converter feeds which plane. A swap between channels 1
 *                and 2 is audible but not obviously a software bug.
 *
 *              - that the monitor sum really is a sum, and that the default
 *                master gain really does make a two-plane sum land exactly at
 *                full scale rather than one bit over it.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "chan_map.h"
#include "hp_bus.h"


/** A frame of SAI words, one stereo converter. */
#define SAI_WORDS       (AUDIO_BLOCK_FRAMES * AIO_SLOTS_PER_SAI)

/** A frame of engine samples, all four planes. */
#define BLK_WORDS       (AUDIO_BLOCK_FRAMES * AUDIO_CH_QTY)


static S32 aSai1[SAI_WORDS];
static S32 aSai2[SAI_WORDS];
static S32 aBlk[BLK_WORDS];
static S32 aHp[AUDIO_BLOCK_FRAMES * AIO_HP_SLOTS];


/** Set every plane of frame 0 to a value and run the monitor sum for one block. */
static void HpOneBlock(const S32 nP0, const S32 nP1, const S32 nP2, const S32 nP3)
{
    U16 i;

    for (i = 0U; i < (U16)AUDIO_BLOCK_FRAMES; i++)
    {
        aBlk[(i * AUDIO_CH_QTY) + 0U] = nP0;
        aBlk[(i * AUDIO_CH_QTY) + 1U] = nP1;
        aBlk[(i * AUDIO_CH_QTY) + 2U] = nP2;
        aBlk[(i * AUDIO_CH_QTY) + 3U] = nP3;
    }

    HpBus_Process(aBlk, aHp, (U16)AUDIO_BLOCK_FRAMES);
}

//--------------------------------------------------------------------------------------------------

/** Sign-extend the way the headphone converter would read a 24-bit word back. */
static S32 Hp24(const S32 nWord)
{
    U32 nRaw = (U32)nWord & 0x00FFFFFFUL;

    if ((nRaw & 0x00800000UL) != 0UL)
    {
        nRaw |= 0xFF000000UL;
    }

    return (S32)nRaw;
}


void Test_ChanMap(void)
{
    U16 i;

    /* ---- the one that matters ------------------------------------------------ */
    TEST_BEGIN("24-bit words are sign extended, not zero extended");
    {
        /* The SAI stores received data right aligned with the top byte clear.
         * These six values are the whole contract. */
        static const S32 aHw[6]   = { 0x000000, 0x000001, 0x7FFFFF,
                                      0x800000, 0xFFFFFF, 0x400000 };
        static const S32 aWant[6] = { 0,        1,        8388607L,
                                      -8388608L, -1,      4194304L };

        for (i = 0U; i < 6U; i++)
        {
            aSai1[(i * AIO_SLOTS_PER_SAI) + 0U] = aHw[i];
            aSai1[(i * AIO_SLOTS_PER_SAI) + 1U] = aHw[i];
            aSai2[(i * AIO_SLOTS_PER_SAI) + 0U] = aHw[i];
            aSai2[(i * AIO_SLOTS_PER_SAI) + 1U] = aHw[i];
        }

        ChanMap_Gather(aSai1, aSai2, aBlk, 6U);

        for (i = 0U; i < 6U; i++)
        {
            U8 p;

            for (p = 0U; p < AUDIO_CH_QTY; p++)
            {
                CHECK(aBlk[(i * AUDIO_CH_QTY) + p] == aWant[i]);
            }
        }

        /* 0x800000 is the value that separates a correct implementation from a
         * plausible one: zero extension turns the most negative sample into the
         * largest positive one. */
        CHECK(aBlk[(3U * AUDIO_CH_QTY)] < 0);
    }
    TEST_END();

    /* ---- routing ------------------------------------------------------------- */
    TEST_BEGIN("each converter slot lands on its own plane");
    {
        for (i = 0U; i < (U16)AUDIO_BLOCK_FRAMES; i++)
        {
            aSai1[(i * AIO_SLOTS_PER_SAI) + 0U] = 0x000101;
            aSai1[(i * AIO_SLOTS_PER_SAI) + 1U] = 0x000202;
            aSai2[(i * AIO_SLOTS_PER_SAI) + 0U] = 0x000303;
            aSai2[(i * AIO_SLOTS_PER_SAI) + 1U] = 0x000404;
        }

        ChanMap_Gather(aSai1, aSai2, aBlk, (U16)AUDIO_BLOCK_FRAMES);

        for (i = 0U; i < (U16)AUDIO_BLOCK_FRAMES; i++)
        {
            CHECK_EQ_U32((U32)aBlk[(i * AUDIO_CH_QTY) + AIO_SAI1_PLANE_BASE + 0U], 0x000101UL);
            CHECK_EQ_U32((U32)aBlk[(i * AUDIO_CH_QTY) + AIO_SAI1_PLANE_BASE + 1U], 0x000202UL);
            CHECK_EQ_U32((U32)aBlk[(i * AUDIO_CH_QTY) + AIO_SAI2_PLANE_BASE + 0U], 0x000303UL);
            CHECK_EQ_U32((U32)aBlk[(i * AUDIO_CH_QTY) + AIO_SAI2_PLANE_BASE + 1U], 0x000404UL);
        }
    }
    TEST_END();

    /* ---- round trip ---------------------------------------------------------- */
    TEST_BEGIN("gather then scatter reproduces the hardware words");
    {
        static S32 aBackSai1[SAI_WORDS];
        static S32 aBackSai2[SAI_WORDS];

        /* A spread of values including both signs and both extremes. */
        for (i = 0U; i < (U16)AUDIO_BLOCK_FRAMES; i++)
        {
            aSai1[(i * AIO_SLOTS_PER_SAI) + 0U] = (S32)((U32)(i * 65537U) & 0x00FFFFFFUL);
            aSai1[(i * AIO_SLOTS_PER_SAI) + 1U] = (S32)((U32)(0xFFFFFFUL - i) & 0x00FFFFFFUL);
            aSai2[(i * AIO_SLOTS_PER_SAI) + 0U] = (S32)((U32)(0x800000UL + i) & 0x00FFFFFFUL);
            aSai2[(i * AIO_SLOTS_PER_SAI) + 1U] = (S32)((U32)(0x7FFFFFUL - i) & 0x00FFFFFFUL);
        }

        ChanMap_Gather(aSai1, aSai2, aBlk, (U16)AUDIO_BLOCK_FRAMES);
        ChanMap_Scatter(aBlk, aBackSai1, aBackSai2, (U16)AUDIO_BLOCK_FRAMES);

        for (i = 0U; i < (U16)SAI_WORDS; i++)
        {
            CHECK_EQ_U32((U32)aBackSai1[i], (U32)aSai1[i]);
            CHECK_EQ_U32((U32)aBackSai2[i], (U32)aSai2[i]);
        }
    }
    TEST_END();

    /* ---- what goes out to the converter -------------------------------------- */
    TEST_BEGIN("transmitted words carry no bits above 24");
    {
        for (i = 0U; i < (U16)AUDIO_BLOCK_FRAMES; i++)
        {
            /* Genuinely negative engine samples, which is what the clamp in
             * audio_sys.c produces. */
            aBlk[(i * AUDIO_CH_QTY) + 0U] = -1;
            aBlk[(i * AUDIO_CH_QTY) + 1U] = AUDIO_SAMPLE_MIN;
            aBlk[(i * AUDIO_CH_QTY) + 2U] = AUDIO_SAMPLE_MAX;
            aBlk[(i * AUDIO_CH_QTY) + 3U] = 0;
        }

        ChanMap_Scatter(aBlk, aSai1, aSai2, (U16)AUDIO_BLOCK_FRAMES);

        for (i = 0U; i < (U16)SAI_WORDS; i++)
        {
            CHECK_EQ_U32((U32)aSai1[i] & 0xFF000000UL, 0UL);
            CHECK_EQ_U32((U32)aSai2[i] & 0xFF000000UL, 0UL);
        }

        /* -1 must become 0xFFFFFF, which reads back as -1 on the wire. */
        CHECK_EQ_U32((U32)aSai1[0], 0x00FFFFFFUL);
        CHECK_EQ_U32((U32)aSai1[1], 0x00800000UL);
        CHECK_EQ_U32((U32)aSai2[0], 0x007FFFFFUL);
        CHECK_EQ_U32((U32)aSai2[1], 0x00000000UL);
    }
    TEST_END();

    /* ---- silence ------------------------------------------------------------- */
    TEST_BEGIN("silence is a zeroed buffer");
    {
        ChanMap_Silence(aSai1, (U32)SAI_WORDS);

        for (i = 0U; i < (U16)SAI_WORDS; i++)
        {
            CHECK_EQ_U32((U32)aSai1[i], 0UL);
        }
    }
    TEST_END();
}


void Test_HpBus(void)
{
    U16 i;
    U8  b;

    /* ---- defaults ------------------------------------------------------------ */
    TEST_BEGIN("even planes go left, odd planes go right");
    {
        CHECK(HpBus_Init() == RESULT_OK);
        CHECK_NEAR((double)HpBus_Master(), (double)HP_MASTER_DEFAULT, 1e-6);

        /* Only plane 0 has signal. It must appear on the left and nowhere else. */
        HpOneBlock(1000000L, 0L, 0L, 0L);
        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)500000L);      /* x 0.5 */
        CHECK_EQ_U32((U32)aHp[1], 0UL);

        /* Plane 1 on the right only. */
        HpOneBlock(0L, 1000000L, 0L, 0L);
        CHECK_EQ_U32((U32)aHp[0], 0UL);
        CHECK_EQ_U32((U32)Hp24(aHp[1]), (U32)500000L);

        /* Plane 2 joins the left, plane 3 the right. */
        HpOneBlock(0L, 0L, 1000000L, 400000L);
        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)500000L);
        CHECK_EQ_U32((U32)Hp24(aHp[1]), (U32)200000L);
    }
    TEST_END();

    /* ---- the reason the default is one half ---------------------------------- */
    TEST_BEGIN("two full-scale planes land exactly at full scale");
    {
        const U32 nClipsBefore = HpBus_ClipCount();

        /* Every plane at full scale: the worst case the monitor can be asked to
         * carry. Left gets planes 0 and 2, so 2 x 8388607 x 0.5 = 8388607. */
        HpOneBlock(AUDIO_SAMPLE_MAX, AUDIO_SAMPLE_MAX, AUDIO_SAMPLE_MAX, AUDIO_SAMPLE_MAX);

        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)AUDIO_SAMPLE_MAX);
        CHECK_EQ_U32((U32)Hp24(aHp[1]), (U32)AUDIO_SAMPLE_MAX);

        /* Exactly at the bound is not over it. If this counts a clip the meter
         * cries wolf on the one case the default gain exists to make safe. */
        CHECK_EQ_U32(HpBus_ClipCount(), nClipsBefore);

        /* And the negative extreme, which is one further from zero. */
        HpOneBlock(AUDIO_SAMPLE_MIN, AUDIO_SAMPLE_MIN, AUDIO_SAMPLE_MIN, AUDIO_SAMPLE_MIN);
        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)AUDIO_SAMPLE_MIN);
        CHECK_EQ_U32(HpBus_ClipCount(), nClipsBefore);
    }
    TEST_END();

    /* ---- it really is a plain sum -------------------------------------------- */
    TEST_BEGIN("no auto gain hides in the monitor sum");
    {
        (void)HpBus_Init();

        /* One plane at half scale, master at one half. A normalising bus would
         * scale by 1/sqrt(2) or 1/N and give something else. */
        HpOneBlock(4000000L, 0L, 0L, 0L);
        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)2000000L);

        /* Two planes at half scale on the same side: twice as loud, because a
         * sum is a sum. */
        HpOneBlock(4000000L, 0L, 4000000L, 0L);
        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)4000000L);
    }
    TEST_END();

    /* ---- clipping is reported, not hidden ------------------------------------ */
    TEST_BEGIN("clipping is counted");
    {
        (void)HpBus_Init();
        HpBus_SetMaster(HP_MASTER_MAX);

        /* Settle the smoother, then push past the bound. */
        for (b = 0U; b < 200U; b++)
        {
            HpOneBlock(AUDIO_SAMPLE_MAX, 0L, AUDIO_SAMPLE_MAX, 0L);
        }

        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)AUDIO_SAMPLE_MAX);
        CHECK(HpBus_ClipCount() > 0UL);
    }
    TEST_END();

    /* ---- the master is smoothed ---------------------------------------------- */
    TEST_BEGIN("the master gain ramps rather than jumping");
    {
        S32 nFirst;
        S32 nSettled;

        (void)HpBus_Init();
        HpBus_SetMaster(1.0f);

        /* The setter reports the target immediately... */
        CHECK_NEAR((double)HpBus_Master(), 1.0, 1e-6);

        /* ...but the first block after it must not already be there, or every
         * volume change is a click. */
        HpOneBlock(4000000L, 0L, 0L, 0L);
        nFirst = Hp24(aHp[0]);
        CHECK(nFirst > 2000000L);           /* moved off 0.5 */
        CHECK(nFirst < 4000000L);           /* but not all the way to 1.0 */

        for (b = 0U; b < 200U; b++)
        {
            HpOneBlock(4000000L, 0L, 0L, 0L);
        }

        nSettled = Hp24(aHp[0]);
        CHECK(nSettled > 3999000L);         /* settled at unity */
        CHECK(nSettled <= 4000000L);
    }
    TEST_END();

    /* ---- selective tap, the half that works today ---------------------------- */
    TEST_BEGIN("source masks solo and route planes");
    {
        (void)HpBus_Init();

        /* Solo plane 3 to both ears. */
        CHECK(HpBus_SetSourceMask(0x8U, 0x8U) == RESULT_OK);
        HpOneBlock(1000000L, 1000000L, 1000000L, 2000000L);
        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)1000000L);
        CHECK_EQ_U32((U32)Hp24(aHp[1]), (U32)1000000L);

        /* Mute everything. */
        CHECK(HpBus_SetSourceMask(0x0U, 0x0U) == RESULT_OK);
        HpOneBlock(AUDIO_SAMPLE_MAX, AUDIO_SAMPLE_MAX, AUDIO_SAMPLE_MAX, AUDIO_SAMPLE_MAX);
        CHECK_EQ_U32((U32)aHp[0], 0UL);
        CHECK_EQ_U32((U32)aHp[1], 0UL);

        /* A plane that does not exist is refused, and nothing changes. */
        CHECK(HpBus_SetSourceMask(0x10U, 0x0U) == RESULT_INVALID_PARAM_0);
        CHECK(HpBus_SetSourceMask(0x0U, 0xF0U) == RESULT_INVALID_PARAM_0);
    }
    TEST_END();

    /* ---- selective tap, the half that is reserved ---------------------------- */
    TEST_BEGIN("reserved tap points are stored but refused");
    {
        (void)HpBus_Init();

        for (i = 0U; i < (U16)CHAIN_MAX_QTY; i++)
        {
            CHECK_EQ_U32(HpBus_TapPoint((U8)i), (U32)HP_TAP_POST_EVERYTHING);
        }

        /* The only one implemented. */
        CHECK(HpBus_SetTapPoint(0U, (U8)HP_TAP_POST_EVERYTHING) == RESULT_OK);

        /* Reserved: refused, so no caller can believe it took effect... */
        CHECK(HpBus_SetTapPoint(0U, (U8)HP_TAP_PRE_FX) == RESULT_NOT_OK);
        CHECK(HpBus_SetTapPoint(1U, (U8)HP_TAP_POST_FX) == RESULT_NOT_OK);

        /* ...but stored, so a preset from a newer GUI round-trips unchanged. */
        CHECK_EQ_U32(HpBus_TapPoint(0U), (U32)HP_TAP_PRE_FX);
        CHECK_EQ_U32(HpBus_TapPoint(1U), (U32)HP_TAP_POST_FX);

        /* Out of range in either argument. */
        CHECK(HpBus_SetTapPoint((U8)CHAIN_MAX_QTY, 0U) == RESULT_INVALID_PARAM_0);
        CHECK(HpBus_SetTapPoint(0U, 99U) == RESULT_INVALID_PARAM_1);
    }
    TEST_END();

    /* ---- what reaches the converter ------------------------------------------ */
    TEST_BEGIN("monitor words carry no bits above 24");
    {
        (void)HpBus_Init();
        HpOneBlock(-4000000L, -4000000L, -4000000L, -4000000L);

        for (i = 0U; i < (U16)(AUDIO_BLOCK_FRAMES * AIO_HP_SLOTS); i++)
        {
            CHECK_EQ_U32((U32)aHp[i] & 0xFF000000UL, 0UL);
        }

        CHECK_EQ_U32((U32)Hp24(aHp[0]), (U32)(-4000000L));
    }
    TEST_END();

    /* Leave the bus at its defaults for anything that runs after this. */
    (void)HpBus_Init();
}
