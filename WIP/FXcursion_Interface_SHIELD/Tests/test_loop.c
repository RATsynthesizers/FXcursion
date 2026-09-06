/*
 * Host harness for the loop transport - Shared/fx_loop.c and the CRC32 that
 * guards its payload.
 *
 * Compiles the REAL shared sources, not a copy: fx_loop.c, fx_crc.c and the
 * protocol headers the firmware uses. There is no audio board yet, so this is
 * the only place the two ends of the transport can be put in a room together
 * and made to disagree.
 *
 * It runs BOTH sides. A session on the "interface" and a session on the
 * "audio" board are stepped through the same lifecycle with the same
 * functions, and the invariants that matter are the ones that span them:
 * that the byte count they agree on is the same number, that the CRCs match
 * when the bytes match and differ when they do not, and that a stale reply
 * cannot kill a live session.
 *
 * Build:
 *   gcc -I../Shared -I../Config -I../SystemSW/Include \
 *       test_loop.c ../Shared/fx_loop.c ../Shared/fx_crc.c -o test_loop
 */
#include <stdio.h>
#include <string.h>

#include "fx_loop.h"
#include "fx_crc.h"

/* REC_FRAMES_PER_HALF and REC_LOOP_SLOT_BYTES, for the step-divides-slot test.
   Host-safe: common_cfg.h pulls in general.h and fx_frame.h and nothing else. */
#include "common_cfg.h"

static int nChecks = 0;
static int nFails  = 0;

#define CHECK(cond, ...)                                                     \
    do {                                                                     \
        nChecks++;                                                           \
        if (!(cond)) {                                                       \
            nFails++;                                                        \
            printf("  FAIL %s:%d  ", __FILE__, __LINE__);                    \
            printf(__VA_ARGS__);                                             \
            printf("\n");                                                    \
        }                                                                    \
    } while (0)


/* ============================================================== CRC32 ==== */
/*
 * The nibble table in fx_crc.c is sixteen hand-written constants. These
 * vectors are what makes them trustworthy - without them a transposed digit
 * would produce a CRC that is stable, self-consistent, and wrong, which is
 * the worst possible failure for a check whose entire job is to be believed.
 */
static void test_crc32_vectors(void)
{
    static const U8 aCheck[9] = { '1','2','3','4','5','6','7','8','9' };
    U32 nCrc;
    U32 nSplit;
    U8  aBig[4096];
    U32 i;

    printf("CRC32 standard vectors\n");

    /* The canonical CRC-32/ISO-HDLC check value. */
    nCrc = Crc32_Ieee(aCheck, 9UL, 0UL);
    CHECK(nCrc == 0xCBF43926UL, "\"123456789\" = 0x%08lX, want 0xCBF43926",
          (unsigned long)nCrc);

    /* Empty input must leave the running value alone, or chaining breaks. */
    CHECK(Crc32_Ieee(NULL, 0UL, 0UL) == 0UL, "empty input changed the CRC");
    CHECK(Crc32_Ieee(aCheck, 0UL, 0x12345678UL) == 0x12345678UL,
          "zero length changed the running value");

    /* Single byte, checked against zlib. */
    {
        static const U8 a1[1] = { 'a' };
        nCrc = Crc32_Ieee(a1, 1UL, 0UL);
        CHECK(nCrc == 0xE8B7BE43UL, "\"a\" = 0x%08lX, want 0xE8B7BE43",
              (unsigned long)nCrc);
    }

    {
        static const U8 aStr[3] = { 'a', 'b', 'c' };
        nCrc = Crc32_Ieee(aStr, 3UL, 0UL);
        CHECK(nCrc == 0x352441C2UL, "\"abc\" = 0x%08lX, want 0x352441C2",
              (unsigned long)nCrc);
    }

    /*
     * CHAINING IS THE PROPERTY THE TRANSPORT ACTUALLY RELIES ON.
     *
     * Loop payload arrives in whatever pieces the stream delivers, and both
     * boards fold different-sized pieces of the same bytes. If splitting
     * changed the answer, every transfer would report a mismatch and the
     * check would be worse than useless.
     */
    for (i = 0UL; i < sizeof(aBig); i++)
    {
        aBig[i] = (U8)((i * 31UL + 7UL) & 0xFFUL);
    }

    nCrc = Crc32_Ieee(aBig, sizeof(aBig), 0UL);

    for (i = 1UL; i < sizeof(aBig); i += 337UL)
    {
        nSplit = Crc32_Ieee(aBig, i, 0UL);
        nSplit = Crc32_Ieee(&aBig[i], (U32)sizeof(aBig) - i, nSplit);

        CHECK(nSplit == nCrc, "split at %lu gave 0x%08lX, whole gave 0x%08lX",
              (unsigned long)i, (unsigned long)nSplit, (unsigned long)nCrc);
    }

    /* A single flipped bit anywhere must change the result. */
    aBig[1234] ^= 0x01U;
    CHECK(Crc32_Ieee(aBig, sizeof(aBig), 0UL) != nCrc,
          "one flipped bit did not change the CRC");
    aBig[1234] ^= 0x01U;
}


/* =========================================================== geometry ==== */
static void test_bytes_for(void)
{
    U32 nBytes;

    printf("FxLoop_BytesFor\n");

    /* One second of 24-bit stereo. */
    CHECK(FxLoop_BytesFor(48000UL, 2U, (U8)LOOP_FMT_S24, &nBytes) == RESULT_OK,
          "1 s stereo S24 refused");
    CHECK(nBytes == 288000UL, "1 s stereo S24 = %lu, want 288000",
          (unsigned long)nBytes);

    CHECK(FxLoop_BytesFor(48000UL, 1U, (U8)LOOP_FMT_S32, &nBytes) == RESULT_OK,
          "1 s mono S32 refused");
    CHECK(nBytes == 192000UL, "1 s mono S32 = %lu, want 192000",
          (unsigned long)nBytes);

    /*
     * The 11 MiB staging region on the test board, in seconds. This is the
     * number the memory map claims; if the two ever drift apart, one of them
     * is a lie and it may as well be caught here.
     */
    {
        const U32 nRegion = 11UL * 1024UL * 1024UL;
        U32 nSamples = nRegion / 3UL / 2UL;

        CHECK(FxLoop_BytesFor(nSamples, 2U, (U8)LOOP_FMT_S24, &nBytes) == RESULT_OK,
              "staging-sized loop refused");
        CHECK(nBytes <= nRegion, "staging loop %lu > region %lu",
              (unsigned long)nBytes, (unsigned long)nRegion);
        CHECK((nSamples / 48000UL) == 40UL, "staging holds %lu s, want 40",
              (unsigned long)(nSamples / 48000UL));
    }

    /* Rejections. */
    CHECK(FxLoop_BytesFor(48000UL, 3U, (U8)LOOP_FMT_S24, &nBytes) != RESULT_OK,
          "3 planes accepted");
    CHECK(FxLoop_BytesFor(48000UL, 0U, (U8)LOOP_FMT_S24, &nBytes) != RESULT_OK,
          "0 planes accepted");
    CHECK(FxLoop_BytesFor(48000UL, 2U, 99U, &nBytes) != RESULT_OK,
          "bad format accepted");
    CHECK(FxLoop_BytesFor(48000UL, 2U, (U8)LOOP_FMT_S24, NULL) != RESULT_OK,
          "NULL result accepted");

    /* Overflow must be refused, not wrapped - both boards would agree on the
       wrapped value and write a short file. */
    CHECK(FxLoop_BytesFor(0xFFFFFFFFUL, 2U, (U8)LOOP_FMT_S32, &nBytes) != RESULT_OK,
          "overflowing size accepted");
}


/* ====================================================== happy path ======= */
/*
 * Both boards, one transfer. "if" is the interface, "au" the audio board;
 * every message between them is passed by value, exactly as the wire would.
 */
static void test_save_session(void)
{
    FX_LOOP_SESSION tIf;
    FX_LOOP_SESSION tAu;
    PROTO_LOOP_OPEN tOpen;
    PROTO_LOOP_STAT tStat;

    const U32 nLoopSamples = 48000UL * 10UL;      /* ten seconds */
    U32       nExpect;

    printf("SAVE session, both sides\n");

    FxLoop_Reset(&tIf);
    FxLoop_Reset(&tAu);

    CHECK(FxLoop_BytesFor(nLoopSamples, 2U, (U8)LOOP_FMT_S24, &nExpect) == RESULT_OK,
          "sizing failed");

    /* Interface opens with nBytes 0 - only the audio board knows the length. */
    CHECK(FxLoop_Open(&tIf, 7U, (U8)LOOP_DIR_SAVE, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL,
                      &tOpen) == RESULT_OK,
          "open refused");
    CHECK(tIf.eState == (U8)FX_LOOP_OPENING, "state %u, want OPENING", tIf.eState);

    /* Audio board answers with the real size. */
    CHECK(FxLoop_Accept(&tAu, &tOpen, nExpect, &tStat) == RESULT_OK,
          "accept refused");
    CHECK(tAu.eState == (U8)FX_LOOP_READY, "audio state %u, want READY", tAu.eState);
    CHECK(tStat.nBytes == nExpect, "stat says %lu, want %lu",
          (unsigned long)tStat.nBytes, (unsigned long)nExpect);

    CHECK(FxLoop_OpenReply(&tIf, &tStat) == RESULT_OK, "reply refused");
    CHECK(tIf.eState == (U8)FX_LOOP_READY, "if state %u, want READY", tIf.eState);

    /* THE POINT: both boards now hold the same number, from the same code. */
    CHECK(tIf.nBytesTotal == tAu.nBytesTotal,
          "sides disagree on size: if=%lu au=%lu",
          (unsigned long)tIf.nBytesTotal, (unsigned long)tAu.nBytesTotal);
    CHECK(tIf.nSlotQty == tAu.nSlotQty, "sides disagree on slots");

    /*
     * The frame NEVER widens - it is FX_FRAME_SLOT_QTY slots at all times, and
     * FxLoop_StreamWidth is gone with the negotiation that used to change it.
     * What a session still decides is whether the loop slots carry payload or
     * zeros, which is exactly what IsStreaming reports.
     */
    CHECK(FxLoop_IsStreaming(&tIf) == FALSE, "streaming before START");

    CHECK(FxLoop_Start(&tIf) == RESULT_OK, "if start refused");
    CHECK(FxLoop_Start(&tAu) == RESULT_OK, "au start refused");
    CHECK(FxLoop_IsStreaming(&tIf) == TRUE, "not streaming while RUNNING");

    /*
     * Move the payload in DIFFERENT sized pieces on each side - which is what
     * really happens, since one side is paced by the SPI frame and the other
     * by whatever the card accepts. The CRCs must still agree.
     */
    {
        static U8 aPayload[8192];
        U32 nSent = 0UL;
        U32 i;

        for (i = 0UL; i < sizeof(aPayload); i++)
        {
            aPayload[i] = (U8)((i * 17UL + 3UL) & 0xFFUL);
        }

        while (nSent < nExpect)
        {
            U32 nAu = nExpect - nSent;
            U32 nIf;

            if (nAu > 1024UL) { nAu = 1024UL; }         /* audio: frame paced */

            CHECK(FxLoop_Advance(&tAu, aPayload, nAu) == RESULT_OK,
                  "audio advance failed at %lu", (unsigned long)nSent);

            /* interface: a different, coarser granularity over the same bytes */
            nIf = nAu;
            CHECK(FxLoop_Advance(&tIf, aPayload, nIf) == RESULT_OK,
                  "if advance failed at %lu", (unsigned long)nSent);

            nSent += nAu;
        }

        CHECK(tAu.eState == (U8)FX_LOOP_COMPLETE, "audio %u, want COMPLETE",
              tAu.eState);
        CHECK(tIf.eState == (U8)FX_LOOP_COMPLETE, "if %u, want COMPLETE",
              tIf.eState);
        CHECK(tIf.nCrc == tAu.nCrc, "CRC mismatch if=0x%08lX au=0x%08lX",
              (unsigned long)tIf.nCrc, (unsigned long)tAu.nCrc);
    }

    /* Completion stops the payload, though the frame is the same size. */
    CHECK(FxLoop_IsStreaming(&tIf) == FALSE,
          "still streaming after COMPLETE");

    /* A CRC is only reported once it covers everything. */
    FxLoop_Report(&tAu, &tStat);
    CHECK(tStat.nCrc == tAu.nCrc, "completed report dropped the CRC");
    CHECK(tStat.eState == (U8)FX_LOOP_COMPLETE, "report state wrong");
}


/* ================================================ corruption detected ==== */
static void test_crc_catches_corruption(void)
{
    FX_LOOP_SESSION tIf;
    FX_LOOP_SESSION tAu;
    static U8 aSent[2048];
    static U8 aGot[2048];
    U32 i;

    printf("A corrupted byte is caught\n");

    FxLoop_Reset(&tIf);
    FxLoop_Reset(&tAu);

    for (i = 0UL; i < sizeof(aSent); i++)
    {
        aSent[i] = (U8)(i & 0xFFUL);
        aGot[i]  = aSent[i];
    }

    /* One bit, in the middle - what a marginal lead actually does. */
    aGot[1000] ^= 0x08U;

    tAu.nBytesTotal = sizeof(aSent);
    tAu.eState      = (U8)FX_LOOP_READY;
    tIf.nBytesTotal = sizeof(aGot);
    tIf.eState      = (U8)FX_LOOP_READY;

    CHECK(FxLoop_Start(&tAu) == RESULT_OK, "au start");
    CHECK(FxLoop_Start(&tIf) == RESULT_OK, "if start");

    CHECK(FxLoop_Advance(&tAu, aSent, sizeof(aSent)) == RESULT_OK, "au advance");
    CHECK(FxLoop_Advance(&tIf, aGot, sizeof(aGot)) == RESULT_OK, "if advance");

    CHECK(tIf.nCrc != tAu.nCrc,
          "one flipped bit produced matching CRCs - the check is not working");
}


/* ================================================== sequencing rules ===== */
static void test_sequencing(void)
{
    FX_LOOP_SESSION tS;
    FX_LOOP_SESSION tAu;
    PROTO_LOOP_OPEN tOpen;
    PROTO_LOOP_STAT tStat;

    printf("Sequencing and refusals\n");

    /* START before READY. */
    FxLoop_Reset(&tS);
    CHECK(FxLoop_Start(&tS) != RESULT_OK, "start accepted from IDLE");

    /* ADVANCE before START. */
    FxLoop_Reset(&tS);
    tS.nBytesTotal = 100UL;
    tS.eState      = (U8)FX_LOOP_READY;
    CHECK(FxLoop_Advance(&tS, NULL, 10UL) != RESULT_OK,
          "advance accepted before START");

    /* A second OPEN while one is live. */
    FxLoop_Reset(&tS);
    CHECK(FxLoop_Open(&tS, 1U, (U8)LOOP_DIR_SAVE, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL, &tOpen) == RESULT_OK, "open 1");
    CHECK(FxLoop_Open(&tS, 2U, (U8)LOOP_DIR_SAVE, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL, &tOpen) != RESULT_OK,
          "second open accepted over a live session");

    /* Bad geometry is refused at the negotiation, with a reason. */
    FxLoop_Reset(&tS);
    CHECK(FxLoop_Open(&tS, 1U, (U8)LOOP_DIR_SAVE, LOOPER_QTY, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL, &tOpen) != RESULT_OK,
          "looper index past the end accepted");
    CHECK(FxLoop_Open(&tS, 1U, (U8)LOOP_DIR_SAVE, 0U, 2U,
                      (U8)LOOP_FMT_S24, 0U, 0UL, &tOpen) != RESULT_OK,
          "zero slots accepted - the session could never progress");
    CHECK(FxLoop_Open(&tS, 1U, (U8)LOOP_DIR_SAVE, 0U, 2U, (U8)LOOP_FMT_S24,
                      FX_LOOP_SLOT_QTY_MAX + 1U, 0UL, &tOpen) != RESULT_OK,
          "slot count past the cap accepted");

    /* A LOAD must state a size; only a SAVE may ask. */
    FxLoop_Reset(&tS);
    CHECK(FxLoop_Open(&tS, 1U, (U8)LOOP_DIR_LOAD, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL, &tOpen) != RESULT_OK,
          "LOAD with unknown size accepted");

    /* A load that will not fit is refused with NO_SPACE, and says so. */
    FxLoop_Reset(&tAu);
    FxLoop_Reset(&tS);
    CHECK(FxLoop_Open(&tS, 3U, (U8)LOOP_DIR_LOAD, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 1000000UL, &tOpen) == RESULT_OK,
          "valid LOAD refused");
    CHECK(FxLoop_Accept(&tAu, &tOpen, 500000UL, &tStat) != RESULT_OK,
          "oversized LOAD accepted");
    CHECK(tStat.eResult == (U8)PROTO_RES_NO_SPACE,
          "result %u, want NO_SPACE", tStat.eResult);
    CHECK(tStat.nSession == 3U, "refusal lost the session id");

    /* An empty loop is not a transfer. */
    FxLoop_Reset(&tAu);
    FxLoop_Reset(&tS);
    (void)FxLoop_Open(&tS, 4U, (U8)LOOP_DIR_SAVE, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL, &tOpen);
    CHECK(FxLoop_Accept(&tAu, &tOpen, 0UL, &tStat) != RESULT_OK,
          "empty loop accepted");

    /* Busy: a second OPEN arriving at the audio board mid-session. */
    FxLoop_Reset(&tAu);
    FxLoop_Reset(&tS);
    (void)FxLoop_Open(&tS, 5U, (U8)LOOP_DIR_SAVE, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL, &tOpen);
    CHECK(FxLoop_Accept(&tAu, &tOpen, 96000UL, &tStat) == RESULT_OK, "accept");
    (void)FxLoop_Open(&tS, 6U, (U8)LOOP_DIR_SAVE, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL, &tOpen);
    CHECK(FxLoop_Accept(&tAu, &tOpen, 96000UL, &tStat) != RESULT_OK,
          "second session accepted while one was live");
    CHECK(tStat.eResult == (U8)PROTO_RES_BUSY, "result %u, want BUSY",
          tStat.eResult);
}


/* ==================================================== the stale reply ==== */
/*
 * The failure this guards against is nasty precisely because it is
 * intermittent: a reply to a session that has already been torn down arrives
 * just after its replacement has started, and kills a transfer that was doing
 * nothing wrong. Transfers would then fail only when one had been aborted
 * shortly before.
 */
static void test_stale_reply_ignored(void)
{
    FX_LOOP_SESSION tIf;
    PROTO_LOOP_OPEN tOpen;
    PROTO_LOOP_STAT tStale;

    printf("A stale reply cannot kill a live session\n");

    FxLoop_Reset(&tIf);
    CHECK(FxLoop_Open(&tIf, 42U, (U8)LOOP_DIR_SAVE, 0U, 2U,
                      (U8)LOOP_FMT_S24, FX_LOOP_SLOT_QTY_MAX, 0UL, &tOpen) == RESULT_OK, "open");

    /* A refusal belonging to session 41, arriving late. */
    memset(&tStale, 0, sizeof(tStale));
    tStale.nSession = 41U;
    tStale.eState   = (U8)FX_LOOP_FAILED;
    tStale.eResult  = (U8)PROTO_RES_BUSY;

    CHECK(FxLoop_OpenReply(&tIf, &tStale) != RESULT_OK, "stale reply accepted");
    CHECK(tIf.eState == (U8)FX_LOOP_OPENING,
          "stale reply moved the live session to %u", tIf.eState);
    CHECK(tIf.eResult == (U8)PROTO_RES_OK, "stale reply recorded a failure");
}


/* ========================================= the step must divide the slot ==== */
/*
 * THE TEST THAT EXISTS SO NOBODY REDISCOVERS THIS.
 *
 * The interface arms the loop destination to a whole number of route steps and
 * refuses a session whose rounded-up length passes the end of the staging slot.
 * So the step has to divide the slot, or there is a remainder at the top that a
 * long take rounds into - and the longest loop the machine can record becomes
 * the one loop it cannot save.
 *
 * It is not a theoretical hazard. 27 slots survived only because the longest
 * take happened to sit under a multiple; 23 slots put it 3 072 bytes past the
 * end; 22 divides the slot 512 times exactly. Change FX_FRAME_LOOP_SLOT_QTY and
 * the arithmetic silently re-rolls those dice.
 */
static void test_loop_step_divides_the_slot(void)
{
    const U32 nStep = (U32)FX_FRAME_LOOP_SLOT_QTY * (U32)REC_FRAMES_PER_HALF * 4UL;
    const U32 nSlot = (U32)REC_LOOP_SLOT_BYTES;
    const U32 nTake = 20UL * 48000UL * 2UL * 3UL;   /* longest stereo take, packed 24-bit */
    U32       nArm;

    printf("The loop route's step divides the staging slot\n");

    CHECK(nStep != 0UL, "step is zero");
    CHECK((nSlot % nStep) == 0UL,
          "step %lu does not divide slot %lu - remainder %lu. A take rounding "
          "into that remainder cannot be saved.",
          (unsigned long)nStep, (unsigned long)nSlot,
          (unsigned long)(nSlot % nStep));

    /* The worst case: the longest take, rounded up. It must still fit. */
    nArm = ((nTake + nStep - 1UL) / nStep) * nStep;

    CHECK(nArm <= nSlot,
          "longest take %lu rounds up to %lu, past the %lu slot by %lu",
          (unsigned long)nTake, (unsigned long)nArm, (unsigned long)nSlot,
          (unsigned long)(nArm - nSlot));

    /* And the slot must hold a whole take at all, not merely divide neatly. */
    CHECK(nSlot >= nTake, "slot %lu smaller than one take %lu",
          (unsigned long)nSlot, (unsigned long)nTake);
}


/* ================================================== size disagreement ==== */
static void test_overrun_is_a_failure(void)
{
    FX_LOOP_SESSION tS;

    printf("Advancing past the end fails rather than truncating\n");

    FxLoop_Reset(&tS);
    tS.nBytesTotal = 1000UL;
    tS.eState      = (U8)FX_LOOP_READY;
    CHECK(FxLoop_Start(&tS) == RESULT_OK, "start");

    CHECK(FxLoop_Advance(&tS, NULL, 900UL) == RESULT_OK, "900 refused");
    CHECK(tS.eState == (U8)FX_LOOP_RUNNING, "completed early");

    /* 200 more would be 1100 against a 1000-byte session. */
    CHECK(FxLoop_Advance(&tS, NULL, 200UL) != RESULT_OK, "overrun accepted");
    CHECK(tS.eState == (U8)FX_LOOP_FAILED, "state %u, want FAILED", tS.eState);
    CHECK(tS.nBytesMoved == 900UL, "moved count was disturbed");

    /* Exactly the remainder completes. */
    FxLoop_Reset(&tS);
    tS.nBytesTotal = 1000UL;
    tS.eState      = (U8)FX_LOOP_READY;
    (void)FxLoop_Start(&tS);
    (void)FxLoop_Advance(&tS, NULL, 900UL);
    CHECK(FxLoop_Advance(&tS, NULL, 100UL) == RESULT_OK, "exact remainder refused");
    CHECK(tS.eState == (U8)FX_LOOP_COMPLETE, "exact remainder did not complete");

    /* And nothing moves after COMPLETE. */
    CHECK(FxLoop_Advance(&tS, NULL, 1UL) != RESULT_OK, "advance after COMPLETE");
}


int main(void)
{
    printf("=== fx_loop / CRC32 host harness ===\n\n");

    test_crc32_vectors();
    test_bytes_for();
    test_save_session();
    test_crc_catches_corruption();
    test_sequencing();
    test_stale_reply_ignored();
    test_loop_step_divides_the_slot();
    test_overrun_is_a_failure();

    printf("\n%d checks, %d failures\n", nChecks, nFails);

    return (nFails == 0) ? 0 : 1;
}
