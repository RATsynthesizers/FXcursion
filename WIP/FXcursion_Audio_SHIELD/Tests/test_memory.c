/**
 * @file      test_memory.c
 *
 * @details   The memory plan, checked against arithmetic rather than against
 *            the comment that describes it.
 *
 *            mem_map.h already proves the budgets fit at compile time. What
 *            these tests add is the reverse direction: that the numbers in the
 *            plan are the numbers the hardware actually implies, so a change to
 *            one constant cannot quietly invalidate the reasoning around it.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "mem_map.h"
#include "loop_mem.h"


#define MIB  (1024UL * 1024UL)
#define KIB  (1024UL)


void Test_Memory(void)
{
    /* ---- the invariants the whole design rests on --------------------------- */
    TEST_BEGIN("plane count is the channel count");
    CHECK_EQ_U32(AUDIO_PLANE_QTY, AUDIO_CH_QTY);
    CHECK_EQ_U32(AUDIO_CH_QTY, 4U);
    CHECK_EQ_U32(CHAIN_MAX_WIDTH, 2U);
    /* Two loopers over four planes: planes 0-1 and 2-3. */
    CHECK_EQ_U32(AUDIO_PLANE_QTY % LOOPER_QTY, 0U);
    /* A recorder tap consumes its chain's width, and widths sum to the channel
     * count - which is why the slot map can never conflict. */
    CHECK_EQ_U32(REC_SLOT_QTY, AUDIO_CH_QTY);
    TEST_END();

    /* ---- block timing ------------------------------------------------------- */
    TEST_BEGIN("block period and latency");
    CHECK_EQ_U32(AUDIO_SAMPLE_RATE_HZ, 48000UL);
    CHECK_EQ_U32(AUDIO_BLOCK_FRAMES, 64U);
    /* 64 / 48000 = 1333 us. Buffering latency is twice that. */
    CHECK_EQ_U32(AUDIO_BLOCK_PERIOD_US, 1333UL);
    TEST_END();

    /* ---- SDRAM bank 1: delay lines ------------------------------------------ */
    TEST_BEGIN("delay lines fit SDRAM bank 1");
    /* 4 s x 48 kHz x 4 B = 768 000 B per plane. */
    CHECK_EQ_U32(MEM_DELAY_BYTES_PER_PLANE, 768000UL);
    CHECK_EQ_U32(MEM_SDRAM_DELAY_TOTAL, 768000UL * 4UL);
    CHECK(MEM_SDRAM_DELAY_TOTAL <= MEM_SDRAM_BANK_BYTES);
    /* Comfortably under 3 MiB, leaving 13 MiB for future long-line effects. */
    CHECK(MEM_SDRAM_DELAY_TOTAL < (3UL * MIB));
    TEST_END();

    /* ---- SDRAM bank 2: reverb ----------------------------------------------- */
    TEST_BEGIN("reverb fits SDRAM bank 2");
    CHECK_EQ_U32(MEM_REVERB_BYTES_PER_PLANE, 256UL * KIB);
    CHECK_EQ_U32(MEM_SDRAM_REVERB_TOTAL, 1UL * MIB);
    CHECK(MEM_SDRAM_REVERB_TOTAL <= MEM_SDRAM_BANK_BYTES);
    /* 256 KiB per plane is 65536 frames - about 1.37 s of total line storage,
     * roughly ten times what a Dattorro tank needs. Deliberately generous. */
    CHECK(REVERB_FRAMES_PER_PLANE > (48000UL / 2UL));
    TEST_END();

    /* ---- AXI SRAM: the short modulation lines ------------------------------- */
    TEST_BEGIN("modulation lines fit AXI SRAM");
    /* 30 ms at 48 kHz = 1440 frames = 5760 B per plane, per effect. */
    CHECK_EQ_U32(MODDELAY_MAX_FRAMES, 1440UL);
    CHECK_EQ_U32(MEM_MODDELAY_BYTES_PER_PLANE, 5760UL);
    /* chorus + flanger + vibrato, four planes each. */
    CHECK_EQ_U32(MEM_SRAM_FAST_TOTAL, 5760UL * 3UL * 4UL);
    CHECK(MEM_SRAM_FAST_TOTAL <= MEM_SRAM_FAST_BUDGET_BYTES);
    TEST_END();

    /* ---- SDRAM: loop audio -------------------------------------------------- */
    TEST_BEGIN("loop audio fits the banks' looper regions");
    /* 20 s x 48 kHz x 3 B = 2 880 000 B per plane. */
    CHECK_EQ_U32(MEM_LOOP_BYTES_PER_PLANE, 2880000UL);

    /* One looper is a stereo pair held TWICE - the take and the undo snapshot. */
    CHECK_EQ_U32(MEM_LOOP_PLANES_PER_LOOPER, 2UL);
    CHECK_EQ_U32(MEM_LOOP_BYTES_PER_LOOPER, 2880000UL * 4UL);
    CHECK(MEM_LOOP_BYTES_PER_LOOPER <= MEM_SDRAM_LOOP_REGION_BYTES);

    /* The loop length actually achievable, so a wrong constant shows up as a
     * number rather than as a build that silently wraps. */
    CHECK(MEM_LOOP_SEC_THAT_FIT >= LOOP_MAX_SEC);
    TEST_END();

    /* ---- whole-bank budgets -------------------------------------------------- */
    TEST_BEGIN("each bank holds its looper plus its effect memory");
    /* Bank 1 carries looper 0 and the delay lines; bank 2 looper 1 and the
     * reverb. Overflowing either is a link error naming the region, but that
     * only fires on a firmware build - this fires on every host run. */
    CHECK((MEM_SDRAM_LOOP_REGION_BYTES + MEM_SDRAM_DELAY_TOTAL)  <= MEM_SDRAM_BANK_BYTES);
    CHECK((MEM_SDRAM_LOOP_REGION_BYTES + MEM_SDRAM_REVERB_TOTAL) <= MEM_SDRAM_BANK_BYTES);
    TEST_END();

    /* ---- the staging slot on the other board -------------------------------- */
    TEST_BEGIN("a loop this board can hold fits an interface staging slot");
    {
        /*
         * THE CROSS-BOARD INVARIANT.
         *
         * The interface stages a whole loop before writing it, in a 5.5 MiB
         * slot. If the audio board can record a loop larger than that, a save
         * fails at the far end after the player has already played it - and the
         * only symptom is a refusal with no obvious cause.
         *
         * 5.5 MiB is SDRAM_LOOP_A / SDRAM_LOOP_B in the interface controller's
         * linker script. Nothing here can check that file, so the number is
         * written down in both places and this asserts the relationship.
         */
        const U32 nStagingSlotBytes = (11UL * MIB) / 2UL;
        const U32 nActiveStereo     = MEM_LOOP_BYTES_PER_PLANE * 2UL;

        CHECK(nActiveStereo <= nStagingSlotBytes);
    }
    TEST_END();

    /* ---- the looper's demand on the bus ------------------------------------- */
    TEST_BEGIN("looper bandwidth is a rounding error");
    {
        /* Per block: 4 planes read + 4 planes written, 64 frames of 3 bytes.
         * Unchanged by the move out of PSRAM - the window is the same size, it
         * is just filled by memcpy from SDRAM now rather than by MDMA. */
        const U32 nPerBlock = 4UL * 2UL * AUDIO_BLOCK_FRAMES * LOOP_BYTES_PER_SAMPLE;
        const U32 nPerSec   = (nPerBlock * AUDIO_SAMPLE_RATE_HZ) / AUDIO_BLOCK_FRAMES;

        CHECK_EQ_U32(nPerBlock, 1536UL);
        /* ~1.15 MB/s of sequential burst against an SDRAM bus doing well over
         * 100 MB/s. The reason the window exists is SDRAM LATENCY per sample,
         * not bandwidth. */
        CHECK(nPerSec < (2UL * MIB));
    }
    TEST_END();
}
