/**
 * @file      test_loop_mem.c
 *
 * @details   loop_mem.c - the SDRAM loop buffers and the DTCM window onto them.
 *
 *            Replaced test_loop_store.c, which exercised the QSPI PSRAM staging
 *            chain. There is no PSRAM and no chain any more, so the whole class
 *            of test about whether a fetch drained in time went with it.
 *
 *            What is left is the part that was always the real risk: THE WRAP.
 *            A block that straddles the loop end is two runs in the buffer, and
 *            getting that wrong produces a click at the loop point that only
 *            appears at some loop lengths - which is exactly the kind of bug
 *            that survives casual testing and ships.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "loop_mem.h"

/* ------------------------------------------------------------------------- */
/* A byte pattern that is a function of position, so a misplaced copy shows up
   as a wrong VALUE rather than merely a wrong length. */
static U8 Test_LoopByte(const U32 nFrame, const U8 nB)
{
    return (U8)(((nFrame * 7UL) + (U32)nB + 1UL) & 0xFFUL);
}


static void Test_FillPlane(const U8 nPlane, const U32 nFrames)
{
    U8* const pBase = LoopMem_PlaneBase(nPlane, LOOP_COPY_TAKE);
    U32       f;

    for (f = 0UL; f < nFrames; f++)
    {
        U8 b;

        for (b = 0U; b < LOOP_BYTES_PER_SAMPLE; b++)
        {
            pBase[(f * LOOP_BYTES_PER_SAMPLE) + b] = Test_LoopByte(f, b);
        }
    }
}


void Test_LoopMem(void)
{
    TEST_BEGIN("loop_mem: layout");
    {
        U8 p;

        CHECK(LoopMem_Init() == RESULT_OK);

        /* Every plane has a distinct, non-overlapping buffer. Planes 0,1 are in
           one bank and 2,3 in the other, so an overlap here would mean the
           section macros or the linker regions disagree with LoopMem_PlaneBase. */
        for (p = 0U; p < AUDIO_PLANE_QTY; p++)
        {
            U8* const pTake = LoopMem_PlaneBase(p, LOOP_COPY_TAKE);
            U8* const pUndo = LoopMem_PlaneBase(p, LOOP_COPY_UNDO);
            U8        q;

            CHECK(pTake != NULL_PTR);
            CHECK(pUndo != NULL_PTR);
            CHECK(pTake != pUndo);

            for (q = 0U; q < AUDIO_PLANE_QTY; q++)
            {
                if (q != p)
                {
                    CHECK(LoopMem_PlaneBase(q, LOOP_COPY_TAKE) != pTake);
                    CHECK(LoopMem_PlaneBase(q, LOOP_COPY_UNDO) != pUndo);
                }
            }
        }

        /* Out of range must be refused, not wrapped to plane 0 - a bad looper
           index would otherwise silently record over looper 0's take. */
        CHECK(LoopMem_PlaneBase(AUDIO_PLANE_QTY, LOOP_COPY_TAKE) == NULL_PTR);
        CHECK(LoopMem_PlaneBase(0U, LOOP_COPY_QTY) == NULL_PTR);

        /* The buffer is what LOOP_MAX_SEC says it is. */
        CHECK_EQ_U32(LoopMem_PlaneBytes(),
                     (U32)LOOP_MAX_SEC * AUDIO_SAMPLE_RATE_HZ * LOOP_BYTES_PER_SAMPLE);
    }
    TEST_END();

    TEST_BEGIN("loop_mem: window reads back what was stored");
    {
        const U32 nLen = 4UL * LOOP_WINDOW_FRAMES;
        U8*       pWin;
        U32       f;

        CHECK(LoopMem_Init() == RESULT_OK);
        Test_FillPlane(0U, nLen);

        LoopMem_Arm(0U, 0UL, nLen);
        CHECK(LoopMem_Valid(0U) == FALSE);   /* not until the block opens */

        CHECK(LoopMem_BeginBlock() == TRUE);
        CHECK(LoopMem_Valid(0U) == TRUE);

        pWin = LoopMem_Window(0U);
        CHECK(pWin != NULL_PTR);

        for (f = 0UL; f < LOOP_WINDOW_FRAMES; f++)
        {
            CHECK_EQ_U32(pWin[f * LOOP_BYTES_PER_SAMPLE],
                         Test_LoopByte(f, 0U));
        }
    }
    TEST_END();

    TEST_BEGIN("loop_mem: the window wraps at the loop end");
    {
        /* A loop length that is NOT a whole number of windows, so the last
           block of every pass straddles the end. This is the case that used to
           be a special path in every caller. */
        const U32 nLen = (3UL * LOOP_WINDOW_FRAMES) + (LOOP_WINDOW_FRAMES / 2UL);
        const U32 nPos = nLen - (LOOP_WINDOW_FRAMES / 4UL);
        U8*       pWin;
        U32       i;

        CHECK(LoopMem_Init() == RESULT_OK);
        Test_FillPlane(1U, nLen);

        LoopMem_Arm(1U, nPos, nLen);
        CHECK(LoopMem_BeginBlock() == TRUE);
        CHECK(LoopMem_Valid(1U) == TRUE);

        pWin = LoopMem_Window(1U);

        for (i = 0UL; i < LOOP_WINDOW_FRAMES; i++)
        {
            /* Frame the window slot SHOULD hold, wrapping at the loop end. */
            const U32 nSrc = (nPos + i) % nLen;
            U8        b;

            for (b = 0U; b < LOOP_BYTES_PER_SAMPLE; b++)
            {
                CHECK_EQ_U32(pWin[(i * LOOP_BYTES_PER_SAMPLE) + b],
                             Test_LoopByte(nSrc, b));
            }
        }
    }
    TEST_END();

    TEST_BEGIN("loop_mem: a dirty commit writes back where it came from");
    {
        const U32 nLen = 4UL * LOOP_WINDOW_FRAMES;
        const U32 nPos = 2UL * LOOP_WINDOW_FRAMES;
        U8*       pWin;
        U8* const pBase = LoopMem_PlaneBase(2U, LOOP_COPY_TAKE);
        U32       i;

        CHECK(LoopMem_Init() == RESULT_OK);
        Test_FillPlane(2U, nLen);

        LoopMem_Arm(2U, nPos, nLen);
        CHECK(LoopMem_BeginBlock() == TRUE);

        /* Overwrite the window and commit it dirty. */
        pWin = LoopMem_Window(2U);
        for (i = 0UL; i < LOOP_WINDOW_BYTES; i++)
        {
            pWin[i] = (U8)(0xC0U + (i & 0x0FU));
        }

        LoopMem_Commit(2U, nPos + LOOP_WINDOW_FRAMES, nLen, TRUE);

        /* It must land at nPos - where the window was READ from - not at the
           position that was just asked for. Writing it to nNextPos instead
           sounds like a stutter that compounds as the loop runs. */
        for (i = 0UL; i < LOOP_WINDOW_BYTES; i++)
        {
            CHECK_EQ_U32(pBase[(nPos * LOOP_BYTES_PER_SAMPLE) + i],
                         (U8)(0xC0U + (i & 0x0FU)));
        }

        /* And the block before it is untouched. */
        CHECK_EQ_U32(pBase[((nPos - 1UL) * LOOP_BYTES_PER_SAMPLE)],
                     Test_LoopByte(nPos - 1UL, 0U));
    }
    TEST_END();

    TEST_BEGIN("loop_mem: a loop shorter than a window stays silent");
    {
        /* Not an error, and not a fault - an empty or barely-started loop must
           simply produce silence rather than whatever was in the buffer. */
        CHECK(LoopMem_Init() == RESULT_OK);

        LoopMem_Arm(3U, 0UL, LOOP_WINDOW_FRAMES - 1UL);
        CHECK(LoopMem_BeginBlock() == TRUE);
        CHECK(LoopMem_Valid(3U) == FALSE);

        {
            U8* const pWin = LoopMem_Window(3U);
            U32       i;
            U32       nSum = 0UL;

            for (i = 0UL; i < LOOP_WINDOW_BYTES; i++)
            {
                nSum += pWin[i];
            }

            CHECK_EQ_U32(nSum, 0UL);
        }
    }
    TEST_END();

    TEST_BEGIN("loop_mem: snapshot and restore");
    {
        const U32 nFrames = 2UL * LOOP_WINDOW_FRAMES;
        U8* const pTake   = LoopMem_PlaneBase(0U, LOOP_COPY_TAKE);
        U8* const pUndo   = LoopMem_PlaneBase(0U, LOOP_COPY_UNDO);
        U32       i;

        CHECK(LoopMem_Init() == RESULT_OK);
        Test_FillPlane(0U, nFrames);
        Test_FillPlane(1U, nFrames);

        CHECK(LoopMem_Snapshot(0U, nFrames, FALSE) == RESULT_OK);

        for (i = 0UL; i < (nFrames * LOOP_BYTES_PER_SAMPLE); i++)
        {
            CHECK_EQ_U32(pUndo[i], pTake[i]);
        }

        /* Overdub, then undo. */
        for (i = 0UL; i < (nFrames * LOOP_BYTES_PER_SAMPLE); i++)
        {
            pTake[i] = 0x5AU;
        }

        CHECK(LoopMem_Snapshot(0U, nFrames, TRUE) == RESULT_OK);

        for (i = 0UL; i < (nFrames * LOOP_BYTES_PER_SAMPLE); i++)
        {
            CHECK_EQ_U32(pTake[i], Test_LoopByte(i / LOOP_BYTES_PER_SAMPLE,
                                                 (U8)(i % LOOP_BYTES_PER_SAMPLE)));
        }

        /* Refusals. */
        CHECK(LoopMem_Snapshot(LOOPER_QTY, nFrames, FALSE) != RESULT_OK);
        CHECK(LoopMem_Snapshot(0U, 0UL, FALSE) != RESULT_OK);
        CHECK(LoopMem_Snapshot(0U, 0xFFFFFFFFUL, FALSE) != RESULT_OK);
    }
    TEST_END();
}
