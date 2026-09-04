/**
 * @file      test_frame.c
 *
 * @details   The sync slot, and the scan that reads it.
 *
 *            This is the check that decides whether a half of received audio is
 *            kept or replaced with silence, so getting it wrong is expensive in
 *            both directions. Too strict and it throws away good recordings;
 *            too loose and it passes a rotated stream, which is the exact
 *            failure it was added to catch.
 *
 *            The tests below therefore do two things. They confirm a clean
 *            stream passes - all of it, with the sequence number carried
 *            forward - and they build ACTUALLY ROTATED buffers, the way a
 *            slipped word really looks, and confirm the scan says where the
 *            slip was rather than merely that something was wrong.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "fx_frame.h"


#define TF_FRAMES   (16U)
#define TF_WORDS    (TF_FRAMES * FX_FRAME_SLOT_QTY)

static S32 aBuf[TF_WORDS];


/**
 * A clean block: correct mark, sequence running from nSeq, and every non-sync
 * slot carrying something that is NOT a valid mark, so a scan that looked in
 * the wrong place could not accidentally pass.
 */
static void TfFill(const U16 nSeq0, const U16 nFrames)
{
    U16 f;

    for (f = 0U; f < nFrames; f++)
    {
        S32* const pFrame = &aBuf[(U32)f * (U32)FX_FRAME_SLOT_QTY];
        U8         s;

        pFrame[FX_FRAME_SYNC_SLOT] = FX_FRAME_SYNC_WORD((U16)(nSeq0 + f));

        for (s = 1U; s < (U8)FX_FRAME_SLOT_QTY; s++)
        {
            /* Top byte 0x00 - a sign-extended positive sample, and by
               construction never the mark. */
            pFrame[s] = (S32)(0x00123400UL + s);
        }
    }
}


void Test_Frame(void)
{
    FX_FRAME_SCAN tScan;

    TEST_BEGIN("the frame layout accounts for every slot exactly");
    {
        /* If these ever disagree there are slots nothing writes, or two things
           writing one slot. Both are silent on target. */
        CHECK_EQ_U32((U32)(FX_FRAME_SYNC_SLOT_QTY + FX_FRAME_REC_SLOT_QTY +
                           FX_FRAME_LOOP_SLOT_QTY),
                     (U32)FX_FRAME_SLOT_QTY);

        CHECK_EQ_U32((U32)FX_FRAME_SYNC_SLOT, 0UL);
        CHECK_EQ_U32((U32)FX_FRAME_REC_SLOT_BASE, 1UL);
        CHECK_EQ_U32((U32)FX_FRAME_LOOP_SLOT_BASE, 5UL);
        CHECK_EQ_U32((U32)FX_FRAME_LOOP_SLOT_QTY, 27UL);
        CHECK_EQ_U32((U32)FX_FRAME_BYTES, 128UL);

        /* THE constraint. The receiver's half is 4096 words and its
           half-transfer interrupt fires at a fixed word, so a frame that does
           not divide it puts that boundary inside a frame. */
        CHECK_EQ_U32(4096UL % (U32)FX_FRAME_SLOT_QTY, 0UL);
    }
    TEST_END();

    TEST_BEGIN("the mark cannot be confused with sample data or a dead bus");
    {
        /*
         * A rotated recorder slot holds a sign-extended 24-bit sample, whose
         * top byte is 0x00 or 0xFF and nothing else. If the mark's top byte
         * were either, ordinary audio could forge it.
         */
        const U16 nTop = (U16)((FX_FRAME_SYNC_MARK >> 8) & 0xFFU);

        CHECK(nTop != 0x00U);
        CHECK(nTop != 0xFFU);

        /* And it must not be a pattern a stuck line or a clock produces. */
        CHECK(FX_FRAME_SYNC_MARK != 0x0000U);
        CHECK(FX_FRAME_SYNC_MARK != 0xFFFFU);
        CHECK(FX_FRAME_SYNC_MARK != 0x5555U);
        CHECK(FX_FRAME_SYNC_MARK != 0xAAAAU);
    }
    TEST_END();

    TEST_BEGIN("the sync word round-trips through its own accessors");
    {
        CHECK_EQ_U32((U32)FX_FRAME_MARK_OF(FX_FRAME_SYNC_WORD(0U)),
                     (U32)FX_FRAME_SYNC_MARK);
        CHECK_EQ_U32((U32)FX_FRAME_SEQ_OF(FX_FRAME_SYNC_WORD(0U)), 0UL);

        CHECK_EQ_U32((U32)FX_FRAME_SEQ_OF(FX_FRAME_SYNC_WORD(1234U)), 1234UL);
        CHECK_EQ_U32((U32)FX_FRAME_MARK_OF(FX_FRAME_SYNC_WORD(1234U)),
                     (U32)FX_FRAME_SYNC_MARK);

        /* 0xFFFF is a legal sequence number, not a sentinel - the sentinel
           lives in the 32-bit expectation, not in the wire field. */
        CHECK_EQ_U32((U32)FX_FRAME_SEQ_OF(FX_FRAME_SYNC_WORD(0xFFFFU)), 0xFFFFUL);
        CHECK_EQ_U32((U32)FX_FRAME_MARK_OF(FX_FRAME_SYNC_WORD(0xFFFFU)),
                     (U32)FX_FRAME_SYNC_MARK);
    }
    TEST_END();

    TEST_BEGIN("a clean block passes whole and predicts the next one");
    {
        TfFill(100U, TF_FRAMES);

        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES, 100UL, &tScan),
                     (U32)TF_FRAMES);
        CHECK(tScan.eFault == FX_FRAME_OK);
        CHECK_EQ_U32((U32)tScan.nGoodFrames, (U32)TF_FRAMES);

        /* The next half must continue from here, or every good half would
           report a fault at its first frame. */
        CHECK_EQ_U32((U32)tScan.nNextSeq, (U32)(100U + TF_FRAMES));
    }
    TEST_END();

    TEST_BEGIN("SEQ_ANY adopts the first frame but still checks the rest");
    {
        TfFill(7000U, TF_FRAMES);

        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES,
                                       FX_FRAME_SEQ_ANY, &tScan),
                     (U32)TF_FRAMES);
        CHECK_EQ_U32((U32)tScan.nNextSeq, (U32)(7000U + TF_FRAMES));

        /*
         * Adopting the FIRST frame must not mean abandoning the check for the
         * others - otherwise a resync would swallow a second fault in the same
         * half and report a clean pass.
         */
        TfFill(7000U, TF_FRAMES);
        aBuf[5U * FX_FRAME_SLOT_QTY] = FX_FRAME_SYNC_WORD(9999U);

        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES,
                                       FX_FRAME_SEQ_ANY, &tScan), 5UL);
        CHECK(tScan.eFault == FX_FRAME_FAULT_SEQ);
    }
    TEST_END();

    TEST_BEGIN("a wrong mark is a rotation, and the phase says by how much");
    {
        /*
         * THE CASE THIS MODULE EXISTS FOR.
         *
         * A real slipped word does not corrupt one value - it shifts the whole
         * stream. Built here the way it actually arrives: every word moved one
         * place later, so the mark that belonged at slot 0 now sits at slot 1.
         */
        U16 nShift;

        for (nShift = 1U; nShift < 4U; nShift++)
        {
            U32 i;

            TfFill(200U, TF_FRAMES);

            /* Shift the whole buffer right by nShift words. */
            for (i = TF_WORDS - 1UL; i >= (U32)nShift; i--)
            {
                aBuf[i] = aBuf[i - nShift];
            }

            for (i = 0UL; i < (U32)nShift; i++)
            {
                aBuf[i] = (S32)0x0055AA00UL;    /* not a mark */
            }

            /* Frame 0 is wrong, and nothing was salvaged. */
            CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES, 200UL, &tScan),
                         0UL);
            CHECK(tScan.eFault == FX_FRAME_FAULT_MARK);

            /* And the diagnosis is the shift itself, which is what tells a
               human a single word slipped rather than the data being junk. */
            CHECK_EQ_U32((U32)tScan.nPhase, (U32)nShift);
        }
    }
    TEST_END();

    TEST_BEGIN("a mark nowhere in the frame is reported as corruption");
    {
        U8 s;

        TfFill(300U, TF_FRAMES);

        /* Wipe every mark in the first frame's worth of words, so there is no
           phase to find. That is a different fault from a slip and must not be
           reported as one. */
        for (s = 0U; s < (U8)FX_FRAME_SLOT_QTY; s++)
        {
            aBuf[s] = (S32)0x00010203UL;
        }

        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES, 300UL, &tScan), 0UL);
        CHECK(tScan.eFault == FX_FRAME_FAULT_MARK);
        CHECK_EQ_U32((U32)tScan.nPhase, (U32)FX_FRAME_PHASE_NONE);
    }
    TEST_END();

    TEST_BEGIN("lost frames are counted, not mistaken for a rotation");
    {
        TfFill(400U, TF_FRAMES);

        /* Alignment intact, three frames missing: frame 4 carries the sequence
           number frame 7 should have. */
        aBuf[4U * FX_FRAME_SLOT_QTY] = FX_FRAME_SYNC_WORD(407U);

        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES, 400UL, &tScan), 4UL);
        CHECK(tScan.eFault == FX_FRAME_FAULT_SEQ);
        CHECK_EQ_U32((U32)tScan.nSeqGap, 3UL);

        /* Phase 0 - the frames are where they should be. Saying anything else
           would send someone looking for a clock fault. */
        CHECK_EQ_U32((U32)tScan.nPhase, 0UL);

        /* The four good frames before it are still reported as good. */
        CHECK_EQ_U32((U32)tScan.nGoodFrames, 4UL);
    }
    TEST_END();

    TEST_BEGIN("the sequence gap is right across the 16-bit wrap");
    {
        /* 65534, 65535, 0, 1 ... is a normal stream, not a fault. */
        TfFill(65534U, TF_FRAMES);

        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES, 65534UL, &tScan),
                     (U32)TF_FRAMES);
        CHECK(tScan.eFault == FX_FRAME_OK);
        CHECK_EQ_U32((U32)tScan.nNextSeq, (U32)(U16)(65534U + TF_FRAMES));

        /* And a gap that straddles the wrap measures correctly rather than
           reporting 65533 frames lost. */
        TfFill(65534U, TF_FRAMES);
        aBuf[2U * FX_FRAME_SLOT_QTY] = FX_FRAME_SYNC_WORD(3U);   /* want 65536->0, got 3 */

        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES, 65534UL, &tScan), 2UL);
        CHECK(tScan.eFault == FX_FRAME_FAULT_SEQ);
        CHECK_EQ_U32((U32)tScan.nSeqGap, 3UL);
    }
    TEST_END();

    TEST_BEGIN("a fault in the last frame still fails the half");
    {
        /*
         * The receiver discards the WHOLE half on any fault, so the last frame
         * matters exactly as much as the first - and an off-by-one in the loop
         * bound would let it through.
         */
        TfFill(500U, TF_FRAMES);
        aBuf[(TF_FRAMES - 1U) * FX_FRAME_SLOT_QTY] = (S32)0x00112233UL;

        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES, 500UL, &tScan),
                     (U32)(TF_FRAMES - 1U));
        CHECK(tScan.eFault == FX_FRAME_FAULT_MARK);

        /* Not equal to the frame count, which is what the caller tests. */
        CHECK(tScan.nGoodFrames != (U16)TF_FRAMES);
    }
    TEST_END();

    TEST_BEGIN("nonsense arguments are refused with a defined diagnosis");
    {
        /* A caller that ignores the return value must still find eFault and
           nGoodFrames set, not whatever was on the stack. */
        tScan.nGoodFrames = 0xFFFFU;
        tScan.eFault      = FX_FRAME_FAULT_MARK;

        CHECK_EQ_U32((U32)FxFrame_Scan(NULL_PTR, (U16)TF_FRAMES, 0UL, &tScan), 0UL);
        CHECK_EQ_U32((U32)tScan.nGoodFrames, 0UL);
        CHECK(tScan.eFault == FX_FRAME_OK);

        TfFill(600U, TF_FRAMES);
        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, 0U, 600UL, &tScan), 0UL);

        /* No output pointer at all must not write through it. */
        CHECK_EQ_U32((U32)FxFrame_Scan(aBuf, (U16)TF_FRAMES, 600UL, NULL_PTR), 0UL);
    }
    TEST_END();
}

/****************************************** end of file *******************************************/
