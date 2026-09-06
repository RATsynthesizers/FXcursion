/**
 * @file      test_protocol.c
 *
 * @details   The contract shared with the interface controller.
 *
 *            These files are duplicated into the interface project, so the
 *            thing worth testing is that the layouts are exactly what both
 *            sides will compile - a padding difference between the two
 *            firmwares would corrupt configurations silently.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "fx_defs.h"
#include "fx_protocol.h"
#include "fx_crc.h"

#include <string.h>


void Test_Protocol(void)
{
    /* ---- wire struct layouts ------------------------------------------------ */
    TEST_BEGIN("wire struct sizes");
    CHECK_EQ_U32(sizeof(PROTO_CFG),       96U);
    CHECK_EQ_U32(sizeof(PROTO_SET_PARAM),  8U);
    CHECK_EQ_U32(sizeof(PROTO_SET_TEMPO),  4U);
    CHECK_EQ_U32(sizeof(PROTO_TRANSPORT),  4U);
    CHECK_EQ_U32(sizeof(PROTO_ACK),        8U);
    CHECK_EQ_U32(sizeof(PROTO_TELEMETRY), 48U);
    CHECK_EQ_U32(sizeof(PROTO_DIAG),      48U);
    CHECK_EQ_U32(sizeof(PROTO_STREAM),     4U);
    CHECK_EQ_U32(sizeof(FX_PARAM),         8U);
    TEST_END();

    /* ---- CRC ---------------------------------------------------------------- */
    TEST_BEGIN("CRC-16/CCITT-FALSE check vector");
    {
        const U8 aCheck[9] = { '1','2','3','4','5','6','7','8','9' };

        CHECK_EQ_U32(Crc16_Ccitt(aCheck, 9U, 0xFFFFU), 0x29B1U);
    }
    TEST_END();

    /* ---- the effect pool ---------------------------------------------------- */
    TEST_BEGIN("every effect is mono-only or stereo-only");
    {
        U8 eFx;

        for (eFx = 0U; eFx < (U8)FX_TYPE_QTY; eFx++)
        {
            CHECK((g_aFxDesc[eFx].nWidth == 1U) ||
                  (g_aFxDesc[eFx].nWidth == CHAIN_MAX_WIDTH));
            CHECK(g_aFxDesc[eFx].nParamQty <= FX_PARAM_QTY);
            CHECK(g_aFxDesc[eFx].pParam != NULL_PTR);
        }

        /* Variants adjacent, mono at an even id - what FX_VARIANT_FOR_WIDTH
         * relies on. */
        for (eFx = 0U; eFx < (U8)FX_TYPE_QTY; eFx += 2U)
        {
            CHECK_EQ_U32(g_aFxDesc[eFx].nWidth,      1U);
            CHECK_EQ_U32(g_aFxDesc[eFx + 1U].nWidth, CHAIN_MAX_WIDTH);
            CHECK(strcmp(g_aFxDesc[eFx].pName, g_aFxDesc[eFx + 1U].pName) == 0);
            /* The stereo variant must genuinely differ, not just run twice. */
            CHECK(g_aFxDesc[eFx + 1U].nParamQty > g_aFxDesc[eFx].nParamQty);
            CHECK_EQ_U32(FX_VARIANT_FOR_WIDTH(eFx, 1U),              eFx);
            CHECK_EQ_U32(FX_VARIANT_FOR_WIDTH(eFx, CHAIN_MAX_WIDTH), eFx + 1U);
        }
    }
    TEST_END();

    /* ---- tempo -------------------------------------------------------------- */
    TEST_BEGIN("note divisions are in quarter notes");
    /* BPM defines the quarter note, so a quarter weighs exactly 1. */
    CHECK_NEAR(g_aDivQuarters[DIV_1_4],  1.0,  1e-6);
    CHECK_NEAR(g_aDivQuarters[DIV_1_8],  0.5,  1e-6);
    CHECK_NEAR(g_aDivQuarters[DIV_1_1],  4.0,  1e-6);
    CHECK_NEAR(g_aDivQuarters[DIV_1_8D], 0.75, 1e-6);
    TEST_END();

    /* ---- loop length is a shared fact --------------------------------------- */
    TEST_BEGIN("loop length is visible to both firmwares");
    /* The GUI needs LOOP_MAX_SEC to bound its bar picker, so it lives in the
     * shared header rather than in audio_cfg.h.
     *
     * 20, not the 60 it was: loop audio moved out of the 64 MiB QSPI PSRAM into
     * 11 MiB of each SDRAM bank, and that 11 MiB holds a stereo loop TWICE so
     * that undo has a snapshot to restore. See fx_defs.h. */
    CHECK_EQ_U32(LOOP_MAX_SEC, 20U);
    /* At 120 BPM in 4/4 a bar is 2 s, so 20 s is 10 bars. */
    CHECK_EQ_U32(LOOP_MAX_SEC / 2U, 10U);
    TEST_END();
}
