/**
 * @file      test_ctrl_link.c
 *
 * @details   The framed protocol, driven byte by byte the way a UART delivers it.
 *
 *            ctrl_link.c is transport-agnostic on purpose - it takes bytes in
 *            and calls a function pointer to get bytes out - so all of it runs
 *            here. ctrl_uart.c, the DMA and ring plumbing, is the part that
 *            cannot be tested on the host, which is exactly why the protocol
 *            was kept out of it.
 *
 *            What these tests are really for is the receiver's behaviour on
 *            BAD input. A link between two boards sees corruption, half frames
 *            and resets, and a parser that resynchronises wrongly can wedge
 *            permanently on a stream that is otherwise fine.
 *
 * @copyright RAT Synthesizers
 */

#include "test_util.h"

#include "ctrl_link.h"
#include "fx_crc.h"
#include "grid.h"
#include "rec_stream.h"

#include <string.h>


/** Captured transmit bytes, so outgoing frames can be inspected. */
static U8  aCap[512];
static U16 nCapLen;
static U16 nCapFrames;


static STD_RESULT CaptureTx(const U8* const pData, const U16 nLength)
{
    STD_RESULT eResult = RESULT_NOT_OK;

    if (((U32)nCapLen + nLength) <= sizeof(aCap))
    {
        (void)memcpy(&aCap[nCapLen], pData, (size_t)nLength);
        nCapLen = (U16)(nCapLen + nLength);
        nCapFrames++;
        eResult = RESULT_OK;
    }

    return eResult;
}

//--------------------------------------------------------------------------------------------------

static void CaptureReset(void)
{
    nCapLen    = 0U;
    nCapFrames = 0U;
}

//--------------------------------------------------------------------------------------------------

/** Frame a command the way the interface controller would. */
static U16 BuildFrame(U8* const pDst, const U8 eCmd, const U8* const pPayload, const U8 nLen)
{
    U16 nCrc;
    U16 n = 0U;

    pDst[n] = (U8)PROTO_SYNC0; n++;
    pDst[n] = (U8)PROTO_SYNC1; n++;
    pDst[n] = nLen;            n++;
    pDst[n] = eCmd;            n++;

    if ((pPayload != NULL_PTR) && (nLen > 0U))
    {
        (void)memcpy(&pDst[n], pPayload, (size_t)nLen);
        n = (U16)(n + nLen);
    }

    nCrc = Crc16_Ccitt(&pDst[2], (U16)(nLen + 2U), 0xFFFFU);

    pDst[n] = (U8)(nCrc & 0xFFU);         n++;
    pDst[n] = (U8)((nCrc >> 8U) & 0xFFU); n++;

    return n;
}

//--------------------------------------------------------------------------------------------------

/** Push bytes in the way a UART would, then let the super-loop parse them. */
static U16 Feed(const U8* const pData, const U16 nLength)
{
    U16 i;

    for (i = 0U; i < nLength; i++)
    {
        CtrlLink_RxByte(pData[i]);
    }

    return CtrlLink_Poll();
}


void Test_CtrlLink(void)
{
    U8  aFrame[PROTO_FRAME_MAX + 16U];
    U16 nLen;

    /* ---- a command in, a reply out ------------------------------------------- */
    TEST_BEGIN("ping is answered with pong");
    {
        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_PING, NULL_PTR, 0U);
        CHECK_EQ_U32(nLen, (U32)PROTO_OVERHEAD_BYTES);
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);

        CHECK_EQ_U32(nCapFrames, 1UL);
        CHECK_EQ_U32(nCapLen,    (U32)PROTO_OVERHEAD_BYTES);
        CHECK_EQ_U32(aCap[0],    (U32)PROTO_SYNC0);
        CHECK_EQ_U32(aCap[1],    (U32)PROTO_SYNC1);
        CHECK_EQ_U32(aCap[2],    0UL);
        CHECK_EQ_U32(aCap[3],    (U32)PROTO_CMD_PONG);
        CHECK_EQ_U32(CtrlLink_Stats()->nFramesOk, 1UL);
    }
    TEST_END();

    /* ---- the framer and the parser must agree -------------------------------- */
    TEST_BEGIN("a frame this module built parses back");
    {
        /* Feed our own PONG straight back in. If the CRC span or the byte order
         * ever disagreed between the two halves, this is where it shows. */
        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        CHECK(CtrlLink_SendFrame((U8)PROTO_CMD_PING, NULL_PTR, 0U) == RESULT_OK);
        nLen = nCapLen;
        (void)memcpy(aFrame, aCap, (size_t)nLen);

        CaptureReset();
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
        CHECK_EQ_U32(CtrlLink_Stats()->nCrcErrors, 0UL);
    }
    TEST_END();

    /* ---- corruption is caught, not acted on ---------------------------------- */
    TEST_BEGIN("a bad CRC is counted and the frame dropped");
    {
        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_PING, NULL_PTR, 0U);
        aFrame[nLen - 1U] ^= 0xFFU;                 /* flip the CRC high byte */

        CHECK_EQ_U32(Feed(aFrame, nLen), 0UL);
        CHECK_EQ_U32(nCapFrames, 0UL);              /* no pong */
        CHECK_EQ_U32(CtrlLink_Stats()->nFramesOk,  0UL);
        CHECK_EQ_U32(CtrlLink_Stats()->nCrcErrors, 1UL);

        /* And the parser is back at the start, not wedged. */
        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_PING, NULL_PTR, 0U);
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
    }
    TEST_END();

    /* ---- resynchronisation --------------------------------------------------- */
    TEST_BEGIN("garbage before a frame does not eat it");
    {
        static const U8 aJunk[7] = { 0x00U, 0xFFU, 0xA5U, 0x11U, 0x5AU, 0xA5U, 0x00U };

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        (void)Feed(aJunk, 7U);

        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_PING, NULL_PTR, 0U);
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
    }
    TEST_END();

    TEST_BEGIN("a repeated sync byte still starts a frame");
    {
        /* 0xA5 0xA5 0x5A is a legal opening: the first 0xA5 is noise, or the
         * tail of an aborted frame. A parser that resets on any non-0x5A after
         * 0xA5 loses the frame that follows. */
        U8 aWithLead[PROTO_FRAME_MAX + 4U];

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        aWithLead[0] = (U8)PROTO_SYNC0;
        nLen = BuildFrame(&aWithLead[1], (U8)PROTO_CMD_PING, NULL_PTR, 0U);

        CHECK_EQ_U32(Feed(aWithLead, (U16)(nLen + 1U)), 1UL);
    }
    TEST_END();

    /* ---- a length field is not to be trusted --------------------------------- */
    TEST_BEGIN("an impossible length is refused without overrunning");
    {
        static const U8 aBadLen[4] = { (U8)PROTO_SYNC0, (U8)PROTO_SYNC1, 0xFFU, 0x01U };

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        /* 255 is past PROTO_PAYLOAD_MAX. Accepting it would write 255 bytes
         * into a 96 byte buffer, which is the one bug in a parser like this
         * that is worth more than a glitch. */
        (void)Feed(aBadLen, 4U);
        CHECK(CtrlLink_Stats()->nResyncs > 0UL);

        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_PING, NULL_PTR, 0U);
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
    }
    TEST_END();

    /* ---- forward compatibility ----------------------------------------------- */
    TEST_BEGIN("an unknown command is accepted and ignored");
    {
        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        /* A newer interface build sending something this firmware has never
         * heard of. The frame is well formed, so it must NOT be treated as
         * corruption - resynchronising on it would drop the next real frame. */
        nLen = BuildFrame(aFrame, 0x7FU, NULL_PTR, 0U);

        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
        CHECK_EQ_U32(CtrlLink_Stats()->nFramesOk,  1UL);
        CHECK_EQ_U32(CtrlLink_Stats()->nCrcErrors, 0UL);
        CHECK_EQ_U32(nCapFrames, 0UL);              /* silently dropped */
    }
    TEST_END();

    TEST_BEGIN("a known command with the wrong length is ignored");
    {
        U8 aShort[4];

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        aShort[0] = 0U;
        aShort[1] = 0U;
        aShort[2] = 0U;
        aShort[3] = 0U;

        /* SET_CONFIG with a 4 byte payload. Copying it as a PROTO_CFG would
         * read 92 bytes past the end of the parse buffer. */
        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_SET_CONFIG, aShort, 4U);

        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
        CHECK_EQ_U32(nCapFrames, 0UL);              /* no ACK for a bad request */
    }
    TEST_END();

    /* ---- the one frame that must never be lost -------------------------------- */
    TEST_BEGIN("a configuration is applied and acknowledged");
    {
        PROTO_CFG tCfg;
        PROTO_ACK tAck;

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        Test_MakeDefaultCfg(&tCfg, (U8)TOPO_2_STEREO);
        tCfg.aSlot[0][0] = (U8)BLOCK_FX;
        tCfg.aFxSlot[0][0] = FX_VARIANT_FOR_WIDTH((U8)FX_DELAY_M, CHAIN_MAX_WIDTH);

        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_SET_CONFIG,
                          (const U8*)&tCfg, (U8)sizeof(tCfg));

        /* A full configuration is the largest frame the protocol carries. */
        CHECK_EQ_U32(nLen, (U32)(sizeof(PROTO_CFG) + PROTO_OVERHEAD_BYTES));
        CHECK(nLen <= (U16)PROTO_FRAME_MAX);

        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);

        /* The interface waits for this before trusting the sample stream
         * layout, so it has to come back on the same call. */
        CHECK_EQ_U32(nCapFrames, 1UL);
        CHECK_EQ_U32(aCap[3], (U32)PROTO_CMD_ACK);
        CHECK_EQ_U32(aCap[2], (U32)sizeof(PROTO_ACK));

        (void)memcpy(&tAck, &aCap[4], sizeof(tAck));
        CHECK_EQ_U32(tAck.eResult, (U32)PROTO_RES_OK);

        /* And it actually took effect. */
        CHECK_EQ_U32(Grid_Active()->nChainQty, 2UL);
        CHECK_EQ_U32(Grid_Active()->aWidth[0], (U32)CHAIN_MAX_WIDTH);
    }
    TEST_END();

    /* ---- a frame arriving in pieces ------------------------------------------ */
    TEST_BEGIN("a frame split across polls still assembles");
    {
        U16 i;

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_PING, NULL_PTR, 0U);

        /* One byte per poll: the parser keeps its state between calls, which is
         * the whole reason it is a byte machine and not a buffer scan. */
        for (i = 0U; i < (U16)(nLen - 1U); i++)
        {
            CHECK_EQ_U32(Feed(&aFrame[i], 1U), 0UL);
        }

        CHECK_EQ_U32(Feed(&aFrame[nLen - 1U], 1U), 1UL);
        CHECK_EQ_U32(nCapFrames, 1UL);
    }
    TEST_END();

    /* ---- the receive ring ---------------------------------------------------- */
    TEST_BEGIN("a receive overflow is counted, not silently lost");
    {
        U16 i;

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        /* Fill past the ring without polling - what a super-loop stalled for
         * 25 ms would do to a link running at 11.5 KB/s. */
        for (i = 0U; i < (U16)(CTRL_RX_RING_BYTES + 8U); i++)
        {
            CtrlLink_RxByte(0x00U);
        }

        CHECK(CtrlLink_Stats()->nRxOverflows > 0UL);

        /* Drain, then check the link still works rather than being wedged. */
        (void)CtrlLink_Poll();
        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_PING, NULL_PTR, 0U);
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
    }
    TEST_END();

    /* ---- outgoing frames ----------------------------------------------------- */
    TEST_BEGIN("telemetry is a well formed frame");
    {
        U16 nCrc;

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        CHECK(CtrlLink_SendTelemetry() == RESULT_OK);

        CHECK_EQ_U32(nCapLen, (U32)(sizeof(PROTO_TELEMETRY) + PROTO_OVERHEAD_BYTES));
        CHECK_EQ_U32(aCap[0], (U32)PROTO_SYNC0);
        CHECK_EQ_U32(aCap[1], (U32)PROTO_SYNC1);
        CHECK_EQ_U32(aCap[2], (U32)sizeof(PROTO_TELEMETRY));
        CHECK_EQ_U32(aCap[3], (U32)PROTO_CMD_TELEMETRY);

        /* The CRC covers LEN, CMD and payload - not the sync word. Both ends
         * have to agree on that span or nothing ever validates. */
        nCrc = Crc16_Ccitt(&aCap[2], (U16)(sizeof(PROTO_TELEMETRY) + 2U), 0xFFFFU);
        CHECK_EQ_U32((U32)aCap[nCapLen - 2U], (U32)(nCrc & 0xFFU));
        CHECK_EQ_U32((U32)aCap[nCapLen - 1U], (U32)((nCrc >> 8U) & 0xFFU));

        /* Feeding it back in must parse cleanly, even though this firmware
         * ignores its own telemetry command. */
        (void)memcpy(aFrame, aCap, (size_t)nCapLen);
        CHECK_EQ_U32(Feed(aFrame, nCapLen), 1UL);
        CHECK_EQ_U32(CtrlLink_Stats()->nCrcErrors, 0UL);
    }
    TEST_END();

    TEST_BEGIN("an oversize payload is refused rather than truncated");
    {
        static U8 aBig[PROTO_PAYLOAD_MAX + 1U];

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CaptureReset();

        CHECK(CtrlLink_SendFrame(0x7FU, aBig, (U8)(PROTO_PAYLOAD_MAX + 1U))
              == RESULT_INVALID_PARAM_2);
        CHECK_EQ_U32(nCapFrames, 0UL);

        /* The maximum is still accepted. */
        CHECK(CtrlLink_SendFrame(0x7FU, aBig, (U8)PROTO_PAYLOAD_MAX) == RESULT_OK);
    }
    TEST_END();

    /* ---- the recorder stream gate -------------------------------------------- */
    TEST_BEGIN("the stream command reaches the staging layer");
    {
        PROTO_STREAM tCmd;

        CHECK(CtrlLink_Init(&CaptureTx) == RESULT_OK);
        CHECK(RecStream_Init() == RESULT_OK);

        /* Nothing is transmitted until asked. This is the whole safety property
           of PROTO_CMD_STREAM: the interface must have armed its receive DMA
           first, or it de-interleaves the stream at the wrong phase. */
        CHECK(RecStream_IsEnabled() == FALSE);

        tCmd.bEnable      = (U8)TRUE;
        tCmd.nReserved[0] = 0U;
        tCmd.nReserved[1] = 0U;
        tCmd.nReserved[2] = 0U;

        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_STREAM, (const U8*)&tCmd, (U8)sizeof(tCmd));
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
        CHECK(RecStream_IsEnabled() != FALSE);

        tCmd.bEnable = (U8)FALSE;
        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_STREAM, (const U8*)&tCmd, (U8)sizeof(tCmd));
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);
        CHECK(RecStream_IsEnabled() == FALSE);

        /* A truncated payload must be ignored, not acted on with whatever
           happened to be in the parse buffer - the difference between the
           stream staying off and it starting at an unknown moment. */
        tCmd.bEnable = (U8)TRUE;
        nLen = BuildFrame(aFrame, (U8)PROTO_CMD_STREAM, (const U8*)&tCmd, 2U);
        CHECK_EQ_U32(Feed(aFrame, nLen), 1UL);      /* well formed frame... */
        CHECK(RecStream_IsEnabled() == FALSE);      /* ...but wrong length, so ignored */

        /* Leave it off for whatever runs next. */
        CHECK(RecStream_Init() == RESULT_OK);
    }
    TEST_END();

    /* ---- the diagnostic frame ------------------------------------------------ */
    TEST_BEGIN("the diagnostic command is additive");
    {
        /* PROTO_DIAG was added after both firmwares were already talking. The
         * existing ids must not have moved, and the new one must not collide. */
        CHECK_EQ_U32((U32)PROTO_CMD_ACK,       0x81UL);
        CHECK_EQ_U32((U32)PROTO_CMD_TELEMETRY, 0x83UL);
        CHECK_EQ_U32((U32)PROTO_CMD_DIAG,      0x84UL);
        CHECK_EQ_U32((U32)PROTO_CMD_PONG,      0x85UL);

        /* 48, not the original 36: the recorder stream counters were added and
         * PROTO_VERSION went to 3 with them. Growing a struct is the one kind of
         * change that is NOT additive, which is why the version had to move -
         * and why this number is asserted rather than trusted. */
        CHECK_EQ_U32(sizeof(PROTO_DIAG), 48UL);
        CHECK(sizeof(PROTO_DIAG) <= (size_t)PROTO_PAYLOAD_MAX);
        CHECK_EQ_U32((U32)PROTO_VERSION, 3UL);

        /* The stream command, by contrast, IS additive: a new id in the
         * interface-to-audio range that an older build simply drops. */
        CHECK_EQ_U32((U32)PROTO_CMD_STREAM, 0x06UL);
        CHECK(((U8)PROTO_CMD_STREAM & 0x80U) == 0U);

        /* An older interface build meets it as an unknown command, which the
         * test above already proves is harmless. */
    }
    TEST_END();

    /* Leave the grid in a state the tests that follow expect. */
    {
        PROTO_CFG tCfg;
        PROTO_ACK tAck;

        Test_MakeDefaultCfg(&tCfg, (U8)TOPO_4_MONO);
        (void)Grid_Apply(&tCfg, &tAck);
    }
}
