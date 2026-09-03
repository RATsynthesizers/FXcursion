/**
 * @file      test_rec_stream.c
 *
 * @details   The recorder staging state machine.
 *
 *            The thing under test is not "does it copy bytes" - it is the
 *            invariant that a block is NEVER written into a staging half that
 *            the DMA might be reading. Breaking that does not drop a block, it
 *            splices two, and on target the symptom is a recording that is
 *            subtly wrong in a way no counter reports.
 *
 *            So most of what is here drives the machine into the awkward
 *            orders that two interrupts at different priorities can actually
 *            produce, and then checks which buffer it handed out.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "rec_stream.h"


#define RS_WORDS    ((U16)REC_STAGE_WORDS)

static S32 aSrc[RS_WORDS];


/** A block whose every word identifies the block, so a splice is visible. */
static void RsFill(const S32 nTag)
{
    U16 i;

    for (i = 0U; i < RS_WORDS; i++)
    {
        aSrc[i] = nTag;
    }
}

/** TRUE when every word of a staged half carries the expected tag. */
static BOOLEAN RsHalfIs(const U8 nHalf, const S32 nTag)
{
    const S32* const p = RecStream_Buffer(nHalf);
    BOOLEAN          bOk = TRUE;
    U16              i;

    if (p == NULL_PTR)
    {
        return FALSE;
    }

    for (i = 0U; i < RS_WORDS; i++)
    {
        if (p[i] != nTag)
        {
            bOk = FALSE;
            break;
        }
    }

    return bOk;
}


void Test_RecStream(void)
{
    TEST_BEGIN("staging is inert until enabled");
    CHECK(RecStream_Init() == RESULT_OK);
    CHECK(RecStream_IsEnabled() == FALSE);
    RsFill(1);
    /* No transfer may be started, and nothing may be counted as dropped -
       being switched off is not an overrun. */
    CHECK(RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES) == (U8)REC_STAGE_NONE);
    CHECK_EQ_U32(RecStream_Stats()->nBlocksDropped, 0UL);
    CHECK_EQ_U32(RecStream_Stats()->nBlocksSent, 0UL);
    TEST_END();

    TEST_BEGIN("first block starts a transfer, second one queues");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    {
        U8 nFirst;
        U8 nSecond;

        RsFill(10);
        nFirst = RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);
        CHECK(nFirst != (U8)REC_STAGE_NONE);
        CHECK(RsHalfIs(nFirst, 10));
        CHECK_EQ_U32((U32)RecStream_Words(nFirst), (U32)RS_WORDS);

        /* Nothing has completed, so this one has to wait rather than start. */
        RsFill(11);
        nSecond = RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);
        CHECK(nSecond == (U8)REC_STAGE_NONE);

        /* ...but it must have gone into the OTHER half, untouched by the first. */
        CHECK(RsHalfIs(nFirst, 10));
        CHECK_EQ_U32(RecStream_Stats()->nBlocksDropped, 0UL);

        /* Completing hands over the queued half. */
        CHECK(RecStream_Complete() == (U8)(1U - nFirst));
        CHECK(RsHalfIs((U8)(1U - nFirst), 11));
        CHECK_EQ_U32(RecStream_Stats()->nBlocksSent, 1UL);
    }
    TEST_END();

    TEST_BEGIN("a third block with both halves committed is dropped, not spliced");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    {
        U8 nFirst;

        RsFill(20);
        nFirst = RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);      /* in flight */
        RsFill(21);
        (void)RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);         /* waiting   */

        /* One in flight, one waiting: there is no half left that is safe to
           write. The block must be dropped and counted. */
        RsFill(22);
        CHECK(RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES) == (U8)REC_STAGE_NONE);
        CHECK_EQ_U32(RecStream_Stats()->nBlocksDropped, 1UL);

        /* And crucially: NEITHER committed half may carry the dropped tag. */
        CHECK(RsHalfIs(nFirst, 20));
        CHECK(RsHalfIs((U8)(1U - nFirst), 21));
    }
    TEST_END();

    TEST_BEGIN("the hardware cadence runs indefinitely with nothing dropped");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    {
        /*
         * The real cadence: stage, transfer completes, stage, ... A transfer is
         * 667 us against a 1333 us block on this board, so the previous one has
         * always finished before the next block arrives.
         *
         * NOTE what is deliberately NOT asserted here: that the halves
         * alternate. With nothing in flight BOTH halves are free and handing
         * out the same one every time is entirely correct - the invariant is
         * "never a half the DMA is reading", not "take turns". Asserting
         * alternation here is what the first version of this test did, and it
         * failed against correct code.
         */
        U32 nIter;

        for (nIter = 0UL; nIter < 200UL; nIter++)
        {
            CHECK(RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES) != (U8)REC_STAGE_NONE);
            CHECK(RecStream_Complete() == (U8)REC_STAGE_NONE);
        }

        CHECK_EQ_U32(RecStream_Stats()->nBlocksSent, 200UL);
        CHECK_EQ_U32(RecStream_Stats()->nBlocksDropped, 0UL);
    }
    TEST_END();

    TEST_BEGIN("under overlap it never hands out the half in flight");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    {
        /*
         * The cadence that actually exercises the ping-pong: a block always
         * arrives while the previous transfer is still running, so there is
         * genuinely one busy half at every decision. This is the case a
         * slower SPI clock, or a longer block, would produce.
         */
        U8  nInFlight;
        U32 nIter;

        nInFlight = RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);
        CHECK(nInFlight != (U8)REC_STAGE_NONE);

        for (nIter = 0UL; nIter < 200UL; nIter++)
        {
            /* Next block arrives first - it must queue, and must not be put
               anywhere near the half being transmitted. */
            const S32 nTag = (S32)(100UL + nIter);

            RsFill(nTag);
            CHECK(RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES) == (U8)REC_STAGE_NONE);
            CHECK(RsHalfIs(nInFlight, (S32)(99UL + nIter)) || (nIter == 0UL));

            /* ...and only now does the transfer finish and hand over. */
            {
                const U8 nNext = RecStream_Complete();

                CHECK(nNext != (U8)REC_STAGE_NONE);
                CHECK(nNext != nInFlight);          /* must be the other one */
                CHECK(RsHalfIs(nNext, nTag));
                nInFlight = nNext;
            }
        }

        CHECK_EQ_U32(RecStream_Stats()->nBlocksDropped, 0UL);
    }
    TEST_END();

    TEST_BEGIN("enabling mid-transfer does not hand back the in-flight half");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    {
        U8 nFirst;
        U8 nNext;

        RsFill(30);
        nFirst = RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);
        CHECK(nFirst != (U8)REC_STAGE_NONE);

        /* The interface reconfigures: stop, then start again, while the DMA is
           still reading nFirst. The naive implementation resets the machine
           here, which makes nFirst look free - and the next block is written
           straight over the buffer being transmitted. */
        RecStream_Enable(FALSE);
        RecStream_Enable(TRUE);

        RsFill(31);
        nNext = RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);

        CHECK(nNext != nFirst);
        CHECK(RsHalfIs(nFirst, 30));            /* in flight, must be intact */

        /* The stale transfer finishes; it was a whole block, so it cannot
           rotate the interface's de-interleave. */
        CHECK(RecStream_Complete() != (U8)REC_STAGE_NONE);
    }
    TEST_END();

    TEST_BEGIN("disable drops the queued block but lets the live one finish");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    {
        U8 nFirst;

        RsFill(40);
        nFirst = RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);
        RsFill(41);
        (void)RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);        /* queued */

        RecStream_Enable(FALSE);

        /* Nothing is reading the queued half, so sending it late would only put
           stale audio on the card. It goes. */
        CHECK(RecStream_Complete() == (U8)REC_STAGE_NONE);
        CHECK(RsHalfIs(nFirst, 40));
        CHECK(RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES) == (U8)REC_STAGE_NONE);
    }
    TEST_END();

    TEST_BEGIN("a transfer error resynchronises rather than limping on");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    {
        RsFill(50);
        (void)RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);
        RsFill(51);
        (void)RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES);

        RecStream_Error();
        CHECK_EQ_U32(RecStream_Stats()->nErrors, 1UL);

        /* Both halves are free again and the next block starts a transfer
           immediately - the alternative is a machine that believes a transfer
           is running forever and never sends anything again. */
        RsFill(52);
        CHECK(RecStream_Stage(aSrc, (U16)AUDIO_BLOCK_FRAMES) != (U8)REC_STAGE_NONE);
    }
    TEST_END();

    TEST_BEGIN("a short block transmits only the words it staged");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    {
        const U16 nFrames = (U16)(AUDIO_BLOCK_FRAMES / 4U);
        U8        nHalf;

        RsFill(60);
        nHalf = RecStream_Stage(aSrc, nFrames);
        CHECK(nHalf != (U8)REC_STAGE_NONE);
        CHECK_EQ_U32((U32)RecStream_Words(nHalf), (U32)(nFrames * REC_SLOT_QTY));
    }
    TEST_END();

    TEST_BEGIN("bad arguments are refused without counting a drop");
    CHECK(RecStream_Init() == RESULT_OK);
    RecStream_Enable(TRUE);
    CHECK(RecStream_Stage(NULL_PTR, (U16)AUDIO_BLOCK_FRAMES) == (U8)REC_STAGE_NONE);
    CHECK(RecStream_Stage(aSrc, 0U) == (U8)REC_STAGE_NONE);
    CHECK_EQ_U32(RecStream_Stats()->nBlocksDropped, 0UL);
    CHECK(RecStream_Buffer((U8)REC_STAGE_QTY) == NULL_PTR);
    CHECK_EQ_U32((U32)RecStream_Words((U8)REC_STAGE_QTY), 0UL);
    /* A frame count past the block size is clamped, not trusted - it would
       otherwise read off the end of the recorder's own buffer. */
    CHECK(RecStream_Stage(aSrc, (U16)(AUDIO_BLOCK_FRAMES * 4U)) != (U8)REC_STAGE_NONE);
    TEST_END();
}

/****************************************** end of file *******************************************/
