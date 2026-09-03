/**
 * @file      ctrl_link.c
 *
 * @details   The audio controller's dispatch. See ctrl_link.h for the split
 *            between this and Shared/fx_link.c.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 * \date      02.09.2026 - Framing moved to Shared/fx_link.c
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "ctrl_link.h"

#include "fx_link.h"
#include "grid.h"
#include "params.h"
#include "looper.h"
#include "audio_sys.h"
#include "rec_stream.h"

#include <string.h>



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief One accepted frame.
 *
 * Every payload is COPIED out of the parse buffer before use. That buffer is
 * reused by the next frame, and unlike the design this replaced, nothing here
 * ever writes back into a live DMA buffer.
 *
 * A length that does not match the command's struct is ignored rather than
 * acted on with whatever the buffer happened to hold - see the truncated-frame
 * case in Test/test_ctrl_link.c.
 */
static void Dispatch(const U8 eCmd, const U8* const pPayload, const U8 nLength)
{
    PROTO_ACK tAck;

    switch (eCmd)
    {
        case (U8)PROTO_CMD_SET_CONFIG:
            if (nLength == (U8)sizeof(PROTO_CFG))
            {
                PROTO_CFG tCfg;

                (void)memcpy(&tCfg, pPayload, sizeof(tCfg));

                (void)Params_SetTempo(tCfg.nBpmX10, tCfg.nBeatsPerBar, tCfg.nBeatUnit);
                (void)Grid_Apply(&tCfg, &tAck);

                // The interface MUST wait for this ACK before trusting the SPI
                // stream layout. See the handshake note in fx_protocol.h.
                (void)FxLink_Send((U8)PROTO_CMD_ACK, (const U8*)&tAck, (U8)sizeof(tAck));
            }
            break;

        case (U8)PROTO_CMD_SET_PARAM:
            if (nLength == (U8)sizeof(PROTO_SET_PARAM))
            {
                PROTO_SET_PARAM tCmd;

                (void)memcpy(&tCmd, pPayload, sizeof(tCmd));
                (void)Params_Set(&tCmd);
            }
            break;

        case (U8)PROTO_CMD_SET_TEMPO:
            if (nLength == (U8)sizeof(PROTO_SET_TEMPO))
            {
                PROTO_SET_TEMPO tCmd;

                (void)memcpy(&tCmd, pPayload, sizeof(tCmd));
                (void)Params_SetTempo(tCmd.nBpmX10, tCmd.nBeatsPerBar, tCmd.nBeatUnit);
            }
            break;

        case (U8)PROTO_CMD_TRANSPORT:
            if (nLength == (U8)sizeof(PROTO_TRANSPORT))
            {
                PROTO_TRANSPORT tCmd;

                (void)memcpy(&tCmd, pPayload, sizeof(tCmd));
                (void)Looper_Transport(tCmd.nChain, tCmd.eAction);
            }
            break;

        case (U8)PROTO_CMD_STREAM:
            if (nLength == (U8)sizeof(PROTO_STREAM))
            {
                PROTO_STREAM tCmd;

                (void)memcpy(&tCmd, pPayload, sizeof(tCmd));

                // The interface arms its own receive DMA before asking, which
                // is the entire point of having a command for this rather than
                // streaming unconditionally. See fx_protocol.h.
                RecStream_Enable((tCmd.bEnable != (U8)FALSE) ? TRUE : FALSE);
            }
            break;

        case (U8)PROTO_CMD_PING:
            (void)FxLink_Send((U8)PROTO_CMD_PONG, NULL_PTR, 0U);
            break;

        default:
            // Unknown command from a newer interface build. Ignore it rather
            // than resynchronising: the frame was well formed and CRC-clean.
            do_nothing();
            break;
    }
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT CtrlLink_Init(const CTRL_TX_FN pfTx)
{
    return FxLink_Init(pfTx, &Dispatch);
}

//--------------------------------------------------------------------------------------------------

void CtrlLink_RxByte(const U8 nByte)
{
    FxLink_RxByte(nByte);
}

//--------------------------------------------------------------------------------------------------

U16 CtrlLink_Poll(void)
{
    return FxLink_Poll();
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLink_SendFrame(const U8 eCmd, const U8* const pPayload, const U8 nLength)
{
    return FxLink_Send(eCmd, pPayload, nLength);
}

//--------------------------------------------------------------------------------------------------

STD_RESULT CtrlLink_SendTelemetry(void)
{
    PROTO_TELEMETRY tTelem;

    AudioSys_GetTelemetry(&tTelem);

    return FxLink_Send((U8)PROTO_CMD_TELEMETRY, (const U8*)&tTelem, (U8)sizeof(tTelem));
}

//--------------------------------------------------------------------------------------------------

const CTRL_STATS* CtrlLink_Stats(void)
{
    return FxLink_Stats();
}

/****************************************** end of file *******************************************/
