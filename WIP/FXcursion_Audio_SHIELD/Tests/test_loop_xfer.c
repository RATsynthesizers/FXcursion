/**
 * @file      test_loop_xfer.c
 *
 * @details   The loop transport, both ends of it.
 *
 *            There is no interface controller here, so this file plays it: it
 *            drives a second FX_LOOP_SESSION through the same shared state
 *            machine, passes every message by value the way the wire would, and
 *            checks the two ends agree.
 *
 *            The properties that matter span the boards and cannot be checked
 *            on either one alone:
 *
 *              - both sides compute the SAME byte count for the same loop
 *              - a round trip through the wire slots returns the same bytes
 *              - the CRCs match when the bytes match, and differ when they do not
 *              - a stale message cannot kill the session that replaced it
 *
 *            The wire is 32-bit slots carrying FOUR payload bytes each - the
 *            loop payload is a byte stream, not one sample per slot - so the
 *            pack and unpack are the part most likely to be subtly wrong. A
 *            round trip is the only honest way to test that: pack from a
 *            looper, hand the words over, unpack into another looper, compare.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "loop_xfer.h"
#include "loop_mem.h"
#include "fx_loop.h"

/*
 * One block's worth of WIRE FRAMES, at the real frame geometry.
 *
 * Not just the loop slots: the frame is one sync slot, then REC_SLOT_QTY
 * recorder slots, then the loop slots, so the loop slots of consecutive frames
 * are a stride apart rather than adjacent. Modelling the whole frame here is
 * the point - a test that packed the loop slots contiguously would pass while
 * the firmware wrote every loop sample on top of a recorder one.
 *
 * This was REC_SLOT_QTY + FX_LOOP_SLOT_QTY_MAX, which is 31 and predates the
 * sync slot, and it based the loop run at index REC_SLOT_QTY instead of
 * FX_FRAME_LOOP_SLOT_BASE. The test therefore modelled a frame the firmware
 * has not sent since the sync word was added.
 */
#define TEST_XFER_STRIDE                (FX_FRAME_SLOT_QTY)

static S32 aSlots[AUDIO_BLOCK_FRAMES * TEST_XFER_STRIDE];


static U8 Test_XferByte(const U32 nOfs)
{
    /* Deliberately not a function of the low bits alone, so a pack that drops
       or duplicates a byte within a slot shows up rather than aliasing. */
    return (U8)(((nOfs * 31UL) + (nOfs >> 5U) + 11UL) & 0xFFUL);
}


void Test_LoopXfer(void)
{
    TEST_BEGIN("loop_xfer: open sizes a save from the recorded length");
    {
        PROTO_LOOP_OPEN tOpen;
        PROTO_LOOP_STAT tStat;
        FX_LOOP_SESSION tIf;
        const U32       nFrames = 48000UL;         /* one second */
        U32             nWant   = 0UL;

        CHECK(LoopMem_Init()  == RESULT_OK);
        CHECK(LoopXfer_Init() == RESULT_OK);
        FxLoop_Reset(&tIf);

        /* Interface asks without knowing the length. */
        CHECK(FxLoop_Open(&tIf, 1U, LOOP_DIR_SAVE, 0U, 2U,
                          LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL,
                          &tOpen) == RESULT_OK);

        CHECK(LoopXfer_OnOpen(&tOpen, nFrames, &tStat) == RESULT_OK);

        /* The answer is what FxLoop_BytesFor says, which is what the interface
           would have computed had it known - the point of sharing the function. */
        CHECK(FxLoop_BytesFor(nFrames, 2U, LOOP_FMT_S24, &nWant) == RESULT_OK);
        CHECK_EQ_U32(tStat.nBytes, nWant);
        CHECK_EQ_U32(tStat.eState, (U32)FX_LOOP_READY);

        CHECK(FxLoop_OpenReply(&tIf, &tStat) == RESULT_OK);
        CHECK_EQ_U32(tIf.nBytesTotal, nWant);

        /* Both boards now hold the same number. */
        CHECK_EQ_U32(tIf.nBytesTotal, LoopXfer_Session()->nBytesTotal);
    }
    TEST_END();

    TEST_BEGIN("loop_xfer: an empty looper is refused, not sent as silence");
    {
        PROTO_LOOP_OPEN tOpen;
        PROTO_LOOP_STAT tStat;
        FX_LOOP_SESSION tIf;

        CHECK(LoopXfer_Init() == RESULT_OK);
        FxLoop_Reset(&tIf);

        CHECK(FxLoop_Open(&tIf, 2U, LOOP_DIR_SAVE, 0U, 2U,
                          LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL,
                          &tOpen) == RESULT_OK);

        /* Nothing recorded. Creating a zero-length WAV and calling it a take is
           the failure this prevents. */
        CHECK(LoopXfer_OnOpen(&tOpen, 0UL, &tStat) != RESULT_OK);
        CHECK_EQ_U32(tStat.eState, (U32)FX_LOOP_FAILED);
        CHECK_EQ_U32(tStat.nSession, 2U);
    }
    TEST_END();

    TEST_BEGIN("loop_xfer: a load larger than a plane buffer is refused");
    {
        PROTO_LOOP_OPEN tOpen;
        PROTO_LOOP_STAT tStat;
        FX_LOOP_SESSION tIf;

        CHECK(LoopXfer_Init() == RESULT_OK);
        FxLoop_Reset(&tIf);

        CHECK(FxLoop_Open(&tIf, 3U, LOOP_DIR_LOAD, 0U, 2U, LOOP_FMT_S24,
                          FX_LOOP_SLOT_QTY_MAX,
                          LoopMem_PlaneBytes() * 4UL, &tOpen) == RESULT_OK);

        CHECK(LoopXfer_OnOpen(&tOpen, 0UL, &tStat) != RESULT_OK);
        CHECK_EQ_U32(tStat.eResult, (U32)PROTO_RES_NO_SPACE);
    }
    TEST_END();

    TEST_BEGIN("loop_xfer: a session runs and stops without moving the frame");
    {
        /*
         * This used to assert that the stream WIDENED while a transfer ran and
         * narrowed after. It no longer does, and that is the point: the frame
         * is FX_FRAME_SLOT_QTY slots at all times, so the only thing a session
         * changes is whether the loop slots carry payload or zeros.
         *
         * What is checked instead is the running flag, because that is now the
         * whole of the state a session contributes to the wire - and the
         * transitions still have to be exactly right, since the interface uses
         * them to decide whether to route the loop slots anywhere.
         */
        PROTO_LOOP_OPEN tOpen;
        PROTO_LOOP_STAT tStat;
        PROTO_LOOP_CTL  tCtl;
        FX_LOOP_SESSION tIf;

        CHECK(LoopMem_Init()  == RESULT_OK);
        CHECK(LoopXfer_Init() == RESULT_OK);
        FxLoop_Reset(&tIf);

        CHECK(LoopXfer_IsRunning() == FALSE);

        CHECK(FxLoop_Open(&tIf, 4U, LOOP_DIR_SAVE, 0U, 2U,
                          LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL,
                          &tOpen) == RESULT_OK);
        CHECK(LoopXfer_OnOpen(&tOpen, 48000UL, &tStat) == RESULT_OK);

        /* Negotiated, but not started - the interface has not routed yet. */
        CHECK(LoopXfer_IsRunning() == FALSE);

        tCtl.nSession = 4U;
        tCtl.eAction  = LOOP_ACT_START;
        CHECK(LoopXfer_OnCtl(&tCtl) == RESULT_OK);

        CHECK(LoopXfer_IsRunning() == TRUE);

        /* A control for a session that has gone must not touch this one. */
        tCtl.nSession = 99U;
        tCtl.eAction  = LOOP_ACT_ABORT;
        CHECK(LoopXfer_OnCtl(&tCtl) != RESULT_OK);
        CHECK(LoopXfer_IsRunning() == TRUE);

        tCtl.nSession = 4U;
        CHECK(LoopXfer_OnCtl(&tCtl) == RESULT_OK);
        CHECK(LoopXfer_IsRunning() == FALSE);
    }
    TEST_END();

    TEST_BEGIN("loop_xfer: the frame's slot count is the only one accepted");
    {
        /*
         * The last hole the fixed frame left open. A session could negotiate
         * fewer loop slots than the frame carries, and the two ends then
         * disagreed silently: the audio board packed nSlotQty per frame while
         * the interface routed all of them into staging, so the difference
         * arrived as zeros spliced through the take. Nothing between them
         * could notice - the SPI stream has no CRC.
         */
        PROTO_LOOP_OPEN tOpen;
        FX_LOOP_SESSION tIf;
        U8              nQty;

        for (nQty = 0U; nQty < (U8)(FX_LOOP_SLOT_QTY_MAX + 2U); nQty++)
        {
            FxLoop_Reset(&tIf);

            if (nQty == (U8)FX_LOOP_SLOT_QTY_MAX)
            {
                CHECK(FxLoop_Open(&tIf, 9U, LOOP_DIR_SAVE, 0U, 2U,
                                  LOOP_FMT_S24, nQty, 0UL, &tOpen) == RESULT_OK);
            }
            else
            {
                CHECK(FxLoop_Open(&tIf, 9U, LOOP_DIR_SAVE, 0U, 2U,
                                  LOOP_FMT_S24, nQty, 0UL, &tOpen) != RESULT_OK);
            }
        }
    }
    TEST_END();

    TEST_BEGIN("loop_xfer: a reported session frees the machine for the next");
    {
        /*
         * A session that RUNS TO COMPLETION leaves the machine out of IDLE,
         * and only a reset brings it back. Without one the first transfer of a
         * run was the only one: every later LOOP_OPEN was refused as BUSY. The
         * old tests hid it by calling LoopXfer_Init between cases, which the
         * firmware never does.
         *
         * A REFUSED open is not this case and must not be confused with it -
         * FxLoop_Accept writes FAILED into the reply and leaves the session
         * untouched, so a refusal costs nothing and needs no reset.
         */
        PROTO_LOOP_OPEN tOpen;
        PROTO_LOOP_STAT tStat;
        PROTO_LOOP_CTL  tCtl;
        FX_LOOP_SESSION tIf;
        const U32       nFrames = 100UL;    /* 600 B - one block finishes it */

        CHECK(LoopMem_Init()  == RESULT_OK);
        CHECK(LoopXfer_Init() == RESULT_OK);
        FxLoop_Reset(&tIf);

        CHECK(FxLoop_Open(&tIf, 21U, LOOP_DIR_SAVE, 0U, 2U,
                          LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL,
                          &tOpen) == RESULT_OK);
        CHECK(LoopXfer_OnOpen(&tOpen, nFrames, &tStat) == RESULT_OK);

        tCtl.nSession = 21U;
        tCtl.eAction  = LOOP_ACT_START;
        CHECK(LoopXfer_OnCtl(&tCtl) == RESULT_OK);

        CHECK(LoopXfer_Block(&aSlots[FX_FRAME_LOOP_SLOT_BASE],
                             AUDIO_BLOCK_FRAMES,
                             (U8)TEST_XFER_STRIDE) == RESULT_OK);
        CHECK(LoopXfer_IsRunning() == FALSE);

        /* The report is what hands the machine back. */
        CHECK(LoopXfer_TakeCompletion(&tStat) == TRUE);
        CHECK_EQ_U32(tStat.eState, (U32)FX_LOOP_COMPLETE);

        /* And now a second session is possible - WITHOUT a re-init. */
        FxLoop_Reset(&tIf);
        CHECK(FxLoop_Open(&tIf, 22U, LOOP_DIR_SAVE, 0U, 2U,
                          LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL,
                          &tOpen) == RESULT_OK);
        CHECK(LoopXfer_OnOpen(&tOpen, nFrames, &tStat) == RESULT_OK);
        CHECK_EQ_U32(tStat.eState, (U32)FX_LOOP_READY);
    }
    TEST_END();

    TEST_BEGIN("loop_xfer: a save round trips through the wire slots");
    {
        /*
         * THE TEST THAT MATTERS.
         *
         * Pack a known loop out of looper 0, carry the slot words across, and
         * unpack them into looper 1 with a second session running the other
         * direction. Every byte must come back, in order, and the two CRCs must
         * agree. A pack that drops a byte per slot, or shifts by the wrong
         * amount, or loses the plane boundary, fails here and is close to
         * undetectable anywhere else.
         */
        PROTO_LOOP_OPEN tOpen;
        PROTO_LOOP_STAT tStat;
        PROTO_LOOP_CTL  tCtl;

        const U8  nSlots  = (U8)FX_LOOP_SLOT_QTY_MAX;
        const U32 nFrames = 700UL;              /* not a slot multiple, on purpose */
        U32       nBytes  = 0UL;
        U32       i;
        U32       nGuard;

        CHECK(LoopMem_Init()  == RESULT_OK);
        CHECK(LoopXfer_Init() == RESULT_OK);

        CHECK(FxLoop_BytesFor(nFrames, 2U, LOOP_FMT_S24, &nBytes) == RESULT_OK);

        /* Seed looper 0 across BOTH its planes, so the plane boundary is
           crossed during the transfer. */
        {
            U32 nOfs;

            for (nOfs = 0UL; nOfs < nBytes; nOfs++)
            {
                const U32 nPerPlane = LoopMem_PlaneBytes();
                U8* const pBase = LoopMem_PlaneBase((U8)(nOfs / nPerPlane),
                                                    LOOP_COPY_TAKE);
                pBase[nOfs % nPerPlane] = Test_XferByte(nOfs);
            }
        }

        /* --- the audio side, sending ------------------------------------- */
        tOpen.nBytes    = 0UL;
        tOpen.nSession  = 7U;
        tOpen.eDir      = LOOP_DIR_SAVE;
        tOpen.nLooper   = 0U;
        tOpen.nPlaneQty = 2U;
        tOpen.eFormat   = LOOP_FMT_S24;
        tOpen.nSlotQty  = nSlots;

        CHECK(LoopXfer_OnOpen(&tOpen, nFrames, &tStat) == RESULT_OK);
        CHECK_EQ_U32(tStat.nBytes, nBytes);

        tCtl.nSession = 7U;
        tCtl.eAction  = LOOP_ACT_START;
        CHECK(LoopXfer_OnCtl(&tCtl) == RESULT_OK);

        /* --- the interface side, receiving ------------------------------- */
        {
            FX_LOOP_SESSION tIf;
            static U8       aGot[8192 * 3];
            U32             nGot = 0UL;

            FxLoop_Reset(&tIf);
            tIf.nBytesTotal = nBytes;
            tIf.nSlotQty    = nSlots;
            tIf.eState      = FX_LOOP_READY;
            CHECK(FxLoop_Start(&tIf) == RESULT_OK);

            CHECK(nBytes <= sizeof(aGot));

            nGuard = 0UL;

            while ((LoopXfer_IsRunning() == TRUE) && (nGuard < 10000UL))
            {
                U32 f;

                (void)memset(aSlots, 0, sizeof(aSlots));

                /* Exactly as rec_spi does it: the frame base plus
                   FX_FRAME_LOOP_SLOT_BASE, with the full frame width as the
                   stride. */
                CHECK(LoopXfer_Block(&aSlots[FX_FRAME_LOOP_SLOT_BASE],
                                     AUDIO_BLOCK_FRAMES,
                                     (U8)TEST_XFER_STRIDE) == RESULT_OK);

                /* Unpack exactly the way the interface will: three bytes out of
                   the low 24 bits of each slot word, in slot order. */
                for (f = 0UL; (f < AUDIO_BLOCK_FRAMES) && (nGot < nBytes); f++)
                {
                    U8 s;

                    for (s = 0U; (s < nSlots) && (nGot < nBytes); s++)
                    {
                        const U32 nWord =
                            (U32)aSlots[(f * (U32)TEST_XFER_STRIDE)
                                        + FX_FRAME_LOOP_SLOT_BASE + s];
                        U8        b;

                        /* ALL FOUR bytes of each slot: the loop payload is a
                           byte stream, not one sample per slot. */
                        for (b = 0U; (b < 4U) && (nGot < nBytes); b++)
                        {
                            aGot[nGot] = (U8)((nWord >> (8U * b)) & 0xFFUL);
                            nGot++;
                        }
                    }
                }

                nGuard++;
            }

            CHECK(nGuard < 10000UL);
            CHECK_EQ_U32(nGot, nBytes);

            /* Every byte, in order. */
            for (i = 0UL; i < nBytes; i++)
            {
                CHECK_EQ_U32(aGot[i], Test_XferByte(i));
            }

            /* And the CRCs agree - the audio side checksummed the looper, the
               interface side checksums what came off the wire. */
            (void)FxLoop_Advance(&tIf, aGot, nBytes);
            CHECK_EQ_U32(tIf.eState, (U32)FX_LOOP_COMPLETE);

            LoopXfer_Report(&tStat);
            CHECK_EQ_U32(tStat.eState, (U32)FX_LOOP_COMPLETE);
            CHECK_EQ_U32(tStat.nCrc, tIf.nCrc);

            /* A single corrupted byte must break that agreement, or the check
               is decoration. */
            aGot[nBytes / 2UL] ^= 0x01U;
            {
                FX_LOOP_SESSION tBad;

                FxLoop_Reset(&tBad);
                tBad.nBytesTotal = nBytes;
                tBad.eState      = FX_LOOP_READY;
                (void)FxLoop_Start(&tBad);
                (void)FxLoop_Advance(&tBad, aGot, nBytes);

                CHECK(tBad.nCrc != tStat.nCrc);
            }
        }
    }
    TEST_END();
}
