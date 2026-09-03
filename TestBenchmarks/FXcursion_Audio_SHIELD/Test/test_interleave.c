/**
 * @file      test_interleave.c
 *
 * @details   The recorder stream geometry.
 *
 *            This is arithmetic that ends up in MDMA registers on the interface
 *            controller, and the reason it is tested here rather than trusted is
 *            the failure mode: a stride wrong by four bytes does not crash or
 *            drop out. It records channel 2 into channel 1's file, with real
 *            audio in it, and no counter anywhere reports a problem.
 *
 *            The numbers below are worked out independently from the layout in
 *            fx_interleave.h rather than read off the implementation.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "fx_interleave.h"


void Test_Interleave(void)
{
    FX_IL_XFER tX;

    TEST_BEGIN("four mono chains at the default stream width");
    {
        /* Stride is 4 slots x 4 bytes = 16. Each mono chain moves 4 bytes per
           frame and then skips the other three slots, so 12. */
        U8 nSlot;

        for (nSlot = 0U; nSlot < 4U; nSlot++)
        {
            CHECK(FxInterleave_Xfer(&tX, nSlot, 1U, 4U, 64UL) == RESULT_OK);
            CHECK_EQ_U32(tX.nSrcOffsetBytes, (U32)nSlot * 4UL);
            CHECK_EQ_U32(tX.nBytesPerBeat,   4UL);
            CHECK_EQ_U32(tX.nSrcSkipBytes,  12UL);
            CHECK_EQ_U32(tX.nBeats,         64UL);
            CHECK_EQ_U32(tX.nDstBytes,     256UL);      /* 64 frames x 4 B */
        }
    }
    TEST_END();

    TEST_BEGIN("a stereo pair moves both slots in one contiguous beat");
    {
        /* Slots 0 and 1 are adjacent, so 8 bytes go out per frame and the skip
           is the remaining two slots. That contiguity is the reason a stereo
           WAV needs no separate merge - the pair is already in file order. */
        CHECK(FxInterleave_Xfer(&tX, 0U, 2U, 4U, 64UL) == RESULT_OK);
        CHECK_EQ_U32(tX.nSrcOffsetBytes, 0UL);
        CHECK_EQ_U32(tX.nBytesPerBeat,   8UL);
        CHECK_EQ_U32(tX.nSrcSkipBytes,   8UL);
        CHECK_EQ_U32(tX.nDstBytes,     512UL);          /* twice a mono chain */

        /* The second pair starts at slot 2, so 8 bytes in. */
        CHECK(FxInterleave_Xfer(&tX, 2U, 2U, 4U, 64UL) == RESULT_OK);
        CHECK_EQ_U32(tX.nSrcOffsetBytes, 8UL);
        CHECK_EQ_U32(tX.nBytesPerBeat,   8UL);
        CHECK_EQ_U32(tX.nSrcSkipBytes,   8UL);
    }
    TEST_END();

    TEST_BEGIN("a full-width chain has nothing to skip");
    {
        /* Degenerate but legal: one chain occupying the whole frame. If the
           skip were not zero here the source pointer would run away. */
        CHECK(FxInterleave_Xfer(&tX, 0U, 2U, 2U, 64UL) == RESULT_OK);
        CHECK_EQ_U32(tX.nBytesPerBeat, 8UL);
        CHECK_EQ_U32(tX.nSrcSkipBytes, 0UL);

        CHECK(FxInterleave_Xfer(&tX, 0U, 1U, 1U, 64UL) == RESULT_OK);
        CHECK_EQ_U32(tX.nBytesPerBeat, 4UL);
        CHECK_EQ_U32(tX.nSrcSkipBytes, 0UL);
    }
    TEST_END();

    TEST_BEGIN("a chain that would read past the frame is refused");
    {
        /* THE case this module exists for. A stereo pair starting in the last
           slot would take slot 3 of this frame and slot 0 of the NEXT one -
           half of one moment and half of another, in a file that looks fine. */
        CHECK(FxInterleave_Xfer(&tX, 3U, 2U, 4U, 64UL) != RESULT_OK);

        /* A mono chain at or past the width is the same fault, one slot wide. */
        CHECK(FxInterleave_Xfer(&tX, 4U, 1U, 4U, 64UL) != RESULT_OK);
        CHECK(FxInterleave_Xfer(&tX, 2U, 1U, 2U, 64UL) != RESULT_OK);

        /* ...and the last legal slot must still be accepted, or the check is
           merely off by one in the other direction. */
        CHECK(FxInterleave_Xfer(&tX, 3U, 1U, 4U, 64UL) == RESULT_OK);
        CHECK(FxInterleave_Xfer(&tX, 2U, 2U, 4U, 64UL) == RESULT_OK);
    }
    TEST_END();

    TEST_BEGIN("nonsense arguments are refused, not clamped");
    {
        CHECK(FxInterleave_Xfer(NULL_PTR, 0U, 1U, 4U, 64UL) != RESULT_OK);
        CHECK(FxInterleave_Xfer(&tX, 0U, 0U, 4U, 64UL) != RESULT_OK);   /* zero width  */
        CHECK(FxInterleave_Xfer(&tX, 0U, 1U, 0U, 64UL) != RESULT_OK);   /* zero stream */
        CHECK(FxInterleave_Xfer(&tX, 0U, 1U, 4U, 0UL) != RESULT_OK);    /* no frames   */

        /* Past the widest frame the wire can carry. 99 was refused before as
           "wider than the stream can be" against a cap of REC_SLOT_QTY; the cap
           is now REC_SLOT_QTY plus the loop run, so the number moved but the
           check did not. */
        CHECK(FxInterleave_Xfer(&tX, 0U, 1U,
                                (U8)(FX_IL_STREAM_WIDTH_MAX + 1U), 64UL) != RESULT_OK);
        CHECK(FxInterleave_Xfer(&tX, 0U, (U8)(FX_IL_SLOT_WIDTH_MAX + 1U),
                                (U8)FX_IL_STREAM_WIDTH_MAX, 64UL) != RESULT_OK);
    }
    TEST_END();

    TEST_BEGIN("a slot width past two is a loop run, not nonsense");
    {
        /*
         * Width 3 used to be refused outright: a recorder chain is mono or a
         * stereo pair and nothing else. The loop transport broke that
         * assumption - its slots are contiguous, so an arbitrary run of them is
         * lifted as ONE transfer, which is what makes it cost one MDMA route
         * instead of sixteen.
         *
         * The protection that actually mattered is untouched, and is checked
         * immediately below: a run reaching past the stream width still fails,
         * because that reads the next frame's samples as this frame's.
         */
        CHECK(FxInterleave_Xfer(&tX, 0U, 3U, 4U, 64UL) == RESULT_OK);

        /* A realistic loop route: 16 slots starting after the four recorder
           planes, in a 20-slot frame. */
        CHECK(FxInterleave_Xfer(&tX, (U8)REC_SLOT_QTY, 16U,
                                (U8)(REC_SLOT_QTY + 16U), 64UL) == RESULT_OK);

        /* One slot too many, and it would read into the next frame. */
        CHECK(FxInterleave_Xfer(&tX, (U8)REC_SLOT_QTY, 17U,
                                (U8)(REC_SLOT_QTY + 16U), 64UL) != RESULT_OK);
    }
    TEST_END();

    TEST_BEGIN("block size matches what the audio side actually transmits");
    {
        /* 64 frames x 4 slots x 4 bytes = 1024 B per block, which is the figure
           the SPI bandwidth arithmetic in rec_spi.h is built on. If these two
           ever disagree, one of them is wrong about the wire. */
        CHECK_EQ_U32(FxInterleave_BlockBytes(4U, (U32)AUDIO_BLOCK_FRAMES), 1024UL);

        CHECK_EQ_U32(FxInterleave_BlockBytes(0U, 64UL), 0UL);
        CHECK_EQ_U32(FxInterleave_BlockBytes(4U, 0UL), 0UL);
    }
    TEST_END();

    TEST_BEGIN("every topology's slot map tiles the frame exactly");
    {
        /*
         * The invariant from recorder.h: chain widths always sum to
         * AUDIO_CH_QTY, so "a recorder in every chain" needs exactly
         * REC_SLOT_QTY slots in every topology. Checked by walking each
         * topology's widths and confirming the transfers tile the frame with no
         * gap and no overlap - which is what makes the slot map conflict-free
         * rather than merely usually-fine.
         */
        static const U8 aaWidths[4][4] = {
            { 1U, 1U, 1U, 1U },     /* 4 mono            */
            { 2U, 1U, 1U, 0U },     /* stereo + 2 mono   */
            { 1U, 1U, 2U, 0U },     /* 2 mono + stereo   */
            { 2U, 2U, 0U, 0U },     /* 2 stereo          */
        };
        U8 t;

        for (t = 0U; t < 4U; t++)
        {
            U8 nSlot = 0U;
            U8 c;

            for (c = 0U; c < 4U; c++)
            {
                const U8 nWidth = aaWidths[t][c];

                if (nWidth == 0U)
                {
                    continue;
                }

                CHECK(FxInterleave_Xfer(&tX, nSlot, nWidth,
                                        (U8)REC_SLOT_QTY, 64UL) == RESULT_OK);
                CHECK_EQ_U32(tX.nSrcOffsetBytes, (U32)nSlot * 4UL);

                nSlot = (U8)(nSlot + nWidth);
            }

            /* Exactly filled - no slot left over, none claimed twice. */
            CHECK_EQ_U32((U32)nSlot, (U32)REC_SLOT_QTY);
        }
    }
    TEST_END();
}

/****************************************** end of file *******************************************/
