/**
 * @file      test_loop_store.c
 *
 * @details   The staging layer between the looper and the PSRAM.
 *
 *            This is the one part of the engine that has no equivalent in the
 *            SDRAM design, so it gets its own tests rather than being covered
 *            incidentally by Test_Looper. What matters here is not DSP but
 *            bookkeeping: that a window written in DTCM lands at the right
 *            device address, that a window straddling the loop end is split
 *            into two transfers going to the right places, that planes cannot
 *            reach into each other's region, and that the arm handshake takes
 *            exactly the one block it claims to.
 *
 *            On the host the transfers are memcpy rather than DMA, which is why
 *            these tests can be deterministic. The addresses being exercised
 *            are the real ones.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "loop_store.h"
#include "mem_map.h"


/** Loop length in frames. Deliberately not a multiple of the window. */
#define TL              (657UL)

/** Whole windows that fit below TL without wrapping. 10 x 64 = 640. */
#define TL_WHOLE        (10U)


static void PutTag(U8* const pDst, const U32 nValue)
{
    pDst[0] = (U8)(nValue & 0xFFU);
    pDst[1] = (U8)((nValue >> 8U) & 0xFFU);
    pDst[2] = (U8)((nValue >> 16U) & 0xFFU);
}

//--------------------------------------------------------------------------------------------------

static U32 GetTag(const U8* const pSrc)
{
    return (U32)pSrc[0] | ((U32)pSrc[1] << 8U) | ((U32)pSrc[2] << 16U);
}

//--------------------------------------------------------------------------------------------------

/*
 * Salt, so a later phase can write DIFFERENT content at a frame an earlier
 * phase already filled. Without it the wrap test would pass even if the second
 * segment went nowhere, because the value it expects was already there.
 */
static U32 nTagSalt;

/** The tag a given plane stores at a given frame. Unique across all three. */
static U32 TagOf(const U8 nPlane, const U32 nFrame)
{
    return ((((U32)nPlane + 1UL) << 16U) | ((nFrame + 1UL) & 0xFFFFUL)) ^ (nTagSalt << 20U);
}

//--------------------------------------------------------------------------------------------------

/**
 * @brief Arm a plane and run blocks until its window is valid.
 *
 * @return blocks the store took to make it valid
 */
static U8 ArmAndSettle(const U8 nPlane, const U32 nPos, const U32 nLen)
{
    U8 nBlocks = 0U;

    LoopStore_Arm(nPlane, nPos, nLen);

    while ((LoopStore_Valid(nPlane) == FALSE) && (nBlocks < 4U))
    {
        (void)LoopStore_BeginBlock();
        LoopStore_Kick();
        nBlocks++;
    }

    /* One more, so the caller opens the block with a valid window in hand. */
    (void)LoopStore_BeginBlock();

    return nBlocks;
}

//--------------------------------------------------------------------------------------------------

/** Fill the open window with this plane's tags for frames [nPos, nPos+64). */
static void WriteWindow(const U8 nPlane, const U32 nPos, const U32 nLen)
{
    U8* const pWin = LoopStore_Window(nPlane);
    U16       i;

    for (i = 0U; i < (U16)LOOP_WINDOW_FRAMES; i++)
    {
        U32 nFrame = nPos + (U32)i;

        if (nFrame >= nLen)
        {
            nFrame -= nLen;
        }

        PutTag(&pWin[i * LOOP_BYTES_PER_SAMPLE], TagOf(nPlane, nFrame));
    }
}


void Test_LoopStore(void)
{
    U32 nPos;
    U16 i;
    U8  b;

    /* ---- the arm handshake costs exactly one block --------------------------- */
    TEST_BEGIN("arming a plane takes one block");
    {
        LoopStore_Invalidate();

        CHECK(LoopStore_Valid(0U) == FALSE);

        LoopStore_Arm(0U, 0UL, TL);
        CHECK(LoopStore_Valid(0U) == FALSE);        /* still nothing fetched */

        /* The block that queues the fetch must NOT report the window usable -
         * that is what stops the looper writing into a buffer DMA owns. */
        CHECK(LoopStore_BeginBlock() != FALSE);
        CHECK(LoopStore_Valid(0U) == FALSE);
        LoopStore_Kick();

        CHECK(LoopStore_BeginBlock() != FALSE);
        CHECK(LoopStore_Valid(0U) != FALSE);        /* landed */
    }
    TEST_END();

    /* ---- a window written in DTCM comes back from the device ----------------- */
    TEST_BEGIN("windows round-trip through the device");
    {
        nTagSalt = 0UL;

        (void)ArmAndSettle(0U, 0UL, TL);

        /* Write frames 0..639 as ten whole windows. */
        nPos = 0UL;
        for (b = 0U; b < TL_WHOLE; b++)
        {
            WriteWindow(0U, nPos, TL);
            LoopStore_Commit(0U, nPos + (U32)LOOP_WINDOW_FRAMES, TL, TRUE);
            LoopStore_Kick();

            nPos += (U32)LOOP_WINDOW_FRAMES;
            (void)LoopStore_BeginBlock();
        }

        /* Read them back. Re-arming proves the data went to the device and not
         * merely stayed in the staging buffer. */
        nPos = 0UL;
        for (b = 0U; b < TL_WHOLE; b++)
        {
            const U8* pWin;

            (void)ArmAndSettle(0U, nPos, TL);
            pWin = LoopStore_Window(0U);

            for (i = 0U; i < (U16)LOOP_WINDOW_FRAMES; i++)
            {
                CHECK_EQ_U32(GetTag(&pWin[i * LOOP_BYTES_PER_SAMPLE]),
                             TagOf(0U, nPos + (U32)i));
            }

            /* Not dirty: reading must not write anything back. */
            LoopStore_Commit(0U, nPos, TL, FALSE);
            LoopStore_Kick();

            nPos += (U32)LOOP_WINDOW_FRAMES;
        }
    }
    TEST_END();

    /* ---- the wrap is split into two transfers, to the right addresses -------- */
    TEST_BEGIN("a window straddling the loop end splits correctly");
    {
        const U32 nWrapPos = TL - 32UL;             /* 32 frames before the end */
        const U8* pWin;

        nTagSalt = 1UL;                             /* different content this time */

        (void)ArmAndSettle(0U, nWrapPos, TL);

        /* Tags here run 625..656 then 0..31 - the wrap is inside the window. */
        WriteWindow(0U, nWrapPos, TL);
        LoopStore_Commit(0U, 32UL, TL, TRUE);
        LoopStore_Kick();

        /* Read the same straddling window back. */
        (void)ArmAndSettle(0U, nWrapPos, TL);
        pWin = LoopStore_Window(0U);

        for (i = 0U; i < (U16)LOOP_WINDOW_FRAMES; i++)
        {
            U32 nFrame = nWrapPos + (U32)i;

            if (nFrame >= TL)
            {
                nFrame -= TL;
            }

            CHECK_EQ_U32(GetTag(&pWin[i * LOOP_BYTES_PER_SAMPLE]), TagOf(0U, nFrame));
        }

        LoopStore_Commit(0U, nWrapPos, TL, FALSE);
        LoopStore_Kick();

        /* And check from the other side: the second segment must have gone to
         * the START of the region, not off the end of it. Frames 0..31 now hold
         * what the wrapped part of that window wrote. */
        (void)ArmAndSettle(0U, 0UL, TL);
        pWin = LoopStore_Window(0U);

        for (i = 0U; i < 32U; i++)
        {
            CHECK_EQ_U32(GetTag(&pWin[i * LOOP_BYTES_PER_SAMPLE]), TagOf(0U, (U32)i));
        }

        LoopStore_Commit(0U, 0UL, TL, FALSE);
        LoopStore_Kick();
    }
    TEST_END();

    /* ---- planes cannot reach into each other's region ------------------------ */
    TEST_BEGIN("planes are independent regions");
    {
        U8 p;

        nTagSalt = 2UL;

        /* Same position, every plane, different tags. */
        for (p = 0U; p < AUDIO_PLANE_QTY; p++)
        {
            (void)ArmAndSettle(p, 128UL, TL);
            WriteWindow(p, 128UL, TL);
            LoopStore_Commit(p, 128UL, TL, TRUE);
            LoopStore_Kick();
        }

        for (p = 0U; p < AUDIO_PLANE_QTY; p++)
        {
            const U8* pWin;

            (void)ArmAndSettle(p, 128UL, TL);
            pWin = LoopStore_Window(p);

            for (i = 0U; i < (U16)LOOP_WINDOW_FRAMES; i++)
            {
                CHECK_EQ_U32(GetTag(&pWin[i * LOOP_BYTES_PER_SAMPLE]),
                             TagOf(p, 128UL + (U32)i));
            }

            LoopStore_Commit(p, 128UL, TL, FALSE);
            LoopStore_Kick();
        }
    }
    TEST_END();

    /* ---- a loop shorter than one window is refused, not mishandled ----------- */
    TEST_BEGIN("a sub-window loop stays silent and invalid");
    {
        const U8* pWin;

        LoopStore_Arm(1U, 0UL, 32UL);               /* half a window */

        for (b = 0U; b < 3U; b++)
        {
            (void)LoopStore_BeginBlock();
            LoopStore_Kick();
        }

        /* Never becomes valid, so the looper never plays it... */
        CHECK(LoopStore_Valid(1U) == FALSE);

        /* ...and what it holds is silence, not whatever the device had. */
        pWin = LoopStore_Window(1U);
        for (i = 0U; i < (U16)LOOP_WINDOW_BYTES; i++)
        {
            CHECK_EQ_U32((U32)pWin[i], 0UL);
        }

        /* A commit on such a plane must be a no-op rather than a stray write. */
        LoopStore_Commit(1U, 0UL, 32UL, TRUE);
        LoopStore_Kick();
    }
    TEST_END();

    /* ---- a record start must not cost a block -------------------------------- */
    TEST_BEGIN("a blank arm is usable immediately");
    {
        /* This is the asymmetry that keeps take timing honest. A recording
         * block overwrites its whole window, so there is nothing to wait for;
         * a playing block needs the existing audio and therefore does wait.
         *
         * Without it a one-bar take would come up 64 frames short, which is
         * exactly how this was found. */
        LoopStore_Invalidate();

        LoopStore_Arm(2U, 0UL, TL);
        LoopStore_ArmBlank(3U, 0UL, TL);

        CHECK(LoopStore_Valid(2U) == FALSE);        /* fetch pending */
        CHECK(LoopStore_Valid(3U) != FALSE);        /* nothing to fetch */

        /* And a blank window is silence, not the previous take. */
        {
            const U8* const pWin = LoopStore_Window(3U);

            for (i = 0U; i < (U16)LOOP_WINDOW_BYTES; i++)
            {
                CHECK_EQ_U32((U32)pWin[i], 0UL);
            }
        }

        /* One block later the fetched one has caught up. */
        (void)LoopStore_BeginBlock();
        LoopStore_Kick();
        (void)LoopStore_BeginBlock();
        CHECK(LoopStore_Valid(2U) != FALSE);

        /* A blank arm on an empty loop must still refuse to become valid. */
        LoopStore_ArmBlank(3U, 0UL, 0UL);
        CHECK(LoopStore_Valid(3U) == FALSE);
    }
    TEST_END();

    /* ---- nothing above should have glitched ---------------------------------- */
    TEST_BEGIN("no underruns and no transfer errors");
    /* The queue is sized for the exact worst case - 4 planes x (write + read) x
     * 2 segments - so an error count here would mean it overflowed. */
    CHECK_EQ_U32(LoopStore_Errors(), 0UL);
    CHECK_EQ_U32(LoopStore_Underruns(), 0UL);
    TEST_END();

    /* Leave the store clean for the looper tests that follow. */
    LoopStore_Invalidate();
}
