/**
 * @file      fx_protocol.h
 *
 * @details   Wire format of the UART control link between the interface
 *            controller and the audio controller.
 *
 *            ############################################################
 *            #  DUPLICATED IN THE INTERFACE CONTROLLER - keep in sync.  #
 *            ############################################################
 *
 *            Design rules, in order of importance:
 *
 *            1. SEND STATE, NOT EDITS. There is no "add effect" or "move effect
 *               up" command. The interface owns the UI model and sends the whole
 *               configuration (PROTO_CFG, 96 bytes) whenever anything about the
 *               grid changes. The audio side rebuilds from it. This removes every
 *               dangling-pointer and half-applied-edit failure mode that an
 *               operation-based protocol has, at a cost of 96 bytes - 8.3 ms at
 *               115200 baud.
 *
 *            2. EVERY FRAME IS FRAMED AND CHECKED. Sync word, length, CRC-16.
 *               A dropped byte must resynchronise, not silently rearrange
 *               somebody's signal chain.
 *
 *            3. NOTHING BLOCKS. The interface must never spin waiting for a
 *               reply. Requests are answered asynchronously.
 *
 *            Frame layout:
 *
 *              +------+------+------+------+---------------+--------+--------+
 *              | 0xA5 | 0x5A | LEN  | CMD  | payload[LEN]  | CRC lo | CRC hi |
 *              +------+------+------+------+---------------+--------+--------+
 *
 *            CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over LEN, CMD and
 *            payload. LEN counts payload bytes only.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_PROTOCOL_H
#define FX_PROTOCOL_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "fx_defs.h"

/* One wire contract, two compilers: the audio controller builds this as C, the
 * interface controller includes it from C++. Without this any symbol declared
 * below gets a mangled name and the link fails against the C definition. The
 * guard sits AFTER the includes on purpose - wrapping <string.h> in extern "C"
 * is the classic way to break a C++ build. */
#ifdef __cplusplus
extern "C" {
#endif




/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

#define PROTO_SYNC0                     (0xA5U)
#define PROTO_SYNC1                     (0x5AU)

/** Bytes of framing overhead around the payload. */
#define PROTO_OVERHEAD_BYTES            (6U)

/** Largest payload of any command below. */
#define PROTO_PAYLOAD_MAX               (96U)

#define PROTO_FRAME_MAX                 (PROTO_PAYLOAD_MAX + PROTO_OVERHEAD_BYTES)

/**
 * Bumped whenever any struct in this file changes shape OR the effect pool is
 * renumbered. Version 2 split every effect into mono-only and stereo-only types,
 * which renumbered FX_TYPE - so a version-1 interface must not be allowed to
 * talk to a version-2 audio controller.
 *
 * Version 3 grew PROTO_DIAG from 36 to 48 bytes for the recorder stream
 * counters. Adding PROTO_CMD_STREAM alongside it needed no bump on its own -
 * appending a command is compatible, an unknown command id is dropped - but
 * changing the SHAPE of a struct is not, so the two went together. This is the
 * cheapest possible moment for it: the interface controller has no
 * implementation of this protocol yet, so there is nothing deployed to be
 * incompatible with.
 */
#define PROTO_VERSION                   (3U)


/* Wire structs below are pinned to an exact size with FXC_STATIC_ASSERT (see
 * fx_defs.h), so that a padding difference between the two firmwares becomes a
 * build error rather than a silently corrupted configuration. */



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief Commands. Interface -> audio use 0x00..0x7F, audio -> interface 0x80..0xFF.
 *
 * APPEND ONLY.
 */
typedef enum enPROTO_CMD
{
    /* interface -> audio */
    PROTO_CMD_SET_CONFIG    = 0x01U,    /**< PROTO_CFG        - whole grid state  */
    PROTO_CMD_SET_PARAM     = 0x02U,    /**< PROTO_SET_PARAM  - one parameter     */
    PROTO_CMD_SET_TEMPO     = 0x03U,    /**< PROTO_SET_TEMPO                      */
    PROTO_CMD_TRANSPORT     = 0x04U,    /**< PROTO_TRANSPORT  - looper control    */
    PROTO_CMD_PING          = 0x05U,    /**< no payload                           */
    PROTO_CMD_STREAM        = 0x06U,    /**< PROTO_STREAM - recorder SPI on/off   */
    PROTO_CMD_LOOP_OPEN     = 0x07U,    /**< PROTO_LOOP_OPEN  - negotiate a xfer  */
    PROTO_CMD_LOOP_CTL      = 0x08U,    /**< PROTO_LOOP_CTL   - start / abort     */

    /* audio -> interface */
    PROTO_CMD_ACK           = 0x81U,    /**< PROTO_ACK                            */
    PROTO_CMD_TELEMETRY     = 0x83U,    /**< PROTO_TELEMETRY                      */
    PROTO_CMD_DIAG          = 0x84U,    /**< PROTO_DIAG - bring-up counters       */
    PROTO_CMD_PONG          = 0x85U,    /**< no payload                           */
    PROTO_CMD_LOOP_STAT     = 0x86U     /**< PROTO_LOOP_STAT - session state      */

} PROTO_CMD;

/**
 * @brief Result codes carried by PROTO_ACK.
 */
typedef enum enPROTO_RESULT
{
    PROTO_RES_OK            = 0U,
    PROTO_RES_BAD_VERSION   = 1U,
    PROTO_RES_BAD_TOPOLOGY  = 2U,
    PROTO_RES_BAD_GRID      = 3U,       /**< mixer not a full column, duplicate block, ... */
    PROTO_RES_BAD_PARAM     = 4U,
    PROTO_RES_BUSY          = 5U,       /**< a rebuild is already pending         */
    PROTO_RES_BAD_WIDTH     = 6U,       /**< effect width does not match the chain */
    PROTO_RES_NO_SPACE      = 7U        /**< loop will not fit the other side      */

} PROTO_RESULT;

/**
 * @brief Looper transport actions.
 *
 * Transport is PER CHAIN. Loop LENGTH is per looper pair (planes 0..1, 2..3).
 */
typedef enum enPROTO_TRANSPORT_ACT
{
    TRANSPORT_STOP          = 0U,
    TRANSPORT_RECORD        = 1U,
    TRANSPORT_OVERDUB       = 2U,
    TRANSPORT_PLAY          = 3U,
    TRANSPORT_CLEAR         = 4U

} PROTO_TRANSPORT_ACT;


/**
 * @brief The whole machine configuration. Payload of PROTO_CMD_SET_CONFIG.
 *
 * Field order is chosen so that every member is naturally aligned and the struct
 * needs no compiler padding on either side. Do not reorder without re-checking
 * the static assertion below.
 *
 * This struct is also the core of the preset format - see params.h.
 */
typedef struct stPROTO_CFG
{
    /* --- 4-byte aligned block ------------------------------------------------------------- */
    U16 aMixGain[CHAIN_MAX_QTY][CHAIN_MAX_QTY];     /**< 0..65535 -> 0.0..2.0 linear   */

    /* --- byte block ---------------------------------------------------------------------- */
    U8  nVersion;                                   /**< PROTO_VERSION                 */
    U8  eTopology;                                  /**< TOPOLOGY                      */
    S8  nMixerCol;                                  /**< column index, or GRID_MIXER_COL_NONE */
    U8  bAutoGain;                                  /**< TRUE: apply 1/sqrt(N) bus gain */

    U8  aSlot[CHAIN_MAX_QTY][GRID_SLOT_QTY];        /**< BLOCK_TYPE per grid cell      */
    U8  aFxSlot[CHAIN_MAX_QTY][FXBLOCK_SLOT_QTY];   /**< FX_TYPE, or FX_TYPE_NONE      */
    U8  aFxEnabled[CHAIN_MAX_QTY];                  /**< bitmask over FX slots         */
    S8  aMixPan[CHAIN_MAX_QTY][CHAIN_MAX_QTY];      /**< -128..127 -> -1.0..+1.0       */
    U8  aLoopBars[LOOPER_QTY];                      /**< loop length in bars, per pair */
    U8  nBeatsPerBar;                               /**< time signature numerator      */
    U8  nBeatUnit;                                  /**< time signature denominator    */

    /* --- 2-byte aligned tail -------------------------------------------------------------- */
    U16 nBpmX10;                                    /**< tempo * 10, e.g. 1200 = 120.0 */
    U8  nReserved[2];

} PROTO_CFG;

FXC_STATIC_ASSERT(sizeof(PROTO_CFG) == 96U, proto_cfg_size);


/**
 * @brief Payload of PROTO_CMD_SET_PARAM.
 *
 * Parameters are addressed by (chain, effect type, index) - never by grid
 * position. Because at most one instance of each effect type exists per chain,
 * that address is unique and computable, so reordering effects inside an FX
 * block does not disturb their settings.
 */
typedef struct stPROTO_SET_PARAM
{
    U16 nValue;                 /**< 0..65535 -> 0.0..1.0 normalised             */
    U8  nChain;                 /**< 0..CHAIN_MAX_QTY-1                          */
    U8  eFxType;                /**< FX_TYPE                                     */
    U8  nParamIdx;              /**< 0..FX_PARAM_QTY-1                           */
    U8  bSync;                  /**< TRUE: derive value from tempo               */
    U8  eDivision;              /**< NOTE_DIV, used when bSync is TRUE           */
    U8  nReserved;

} PROTO_SET_PARAM;

FXC_STATIC_ASSERT(sizeof(PROTO_SET_PARAM) == 8U, proto_set_param_size);


/**
 * @brief Payload of PROTO_CMD_SET_TEMPO.
 */
typedef struct stPROTO_SET_TEMPO
{
    U16 nBpmX10;
    U8  nBeatsPerBar;
    U8  nBeatUnit;

} PROTO_SET_TEMPO;

FXC_STATIC_ASSERT(sizeof(PROTO_SET_TEMPO) == 4U, proto_set_tempo_size);


/**
 * @brief Payload of PROTO_CMD_TRANSPORT.
 */
typedef struct stPROTO_TRANSPORT
{
    U8 nChain;
    U8 eAction;                 /**< PROTO_TRANSPORT_ACT                         */
    U8 nReserved[2];

} PROTO_TRANSPORT;

FXC_STATIC_ASSERT(sizeof(PROTO_TRANSPORT) == 4U, proto_transport_size);


/**
 * @brief Payload of PROTO_CMD_STREAM.
 *
 * WHO STARTS THE AUDIO STREAM, AND WHY IT IS THE INTERFACE
 *
 * The recorder stream is a continuous interleave with no framing of its own.
 * The interface receives it into a circular DMA and de-interleaves BY POSITION,
 * so it cannot tell where a block begins. If it joins the stream mid-block, or
 * loses a single word, every recorded channel is permanently rotated into the
 * wrong file - silently, and with audio in it, so it looks like it worked.
 *
 * PROTO_ACK already tells the interface which chain landed in which slot. What
 * it cannot express is WHEN the interface has finished arming its own DMA, and
 * the audio side has no way to know. So the order is:
 *
 *      interface: SET_CONFIG        ->  audio: validate, ACK with the slot map
 *      interface: reprogram MDMA, arm the receive DMA
 *      interface: STREAM(enable=1)  ->  audio: start on the next block boundary
 *
 * and on the way down, STREAM(enable=0) before tearing the receive side down.
 *
 * Appending this command needs no PROTO_VERSION bump: no existing struct
 * changes shape, and a firmware that predates it drops the frame rather than
 * resynchronising on it. It does mean an audio build older than this one never
 * streams - which is a missing feature, not a corrupted recording.
 */
typedef struct stPROTO_STREAM
{
    U8 bEnable;                 /**< TRUE: transmit; FALSE: stop                 */
    U8 nReserved[3];

} PROTO_STREAM;

FXC_STATIC_ASSERT(sizeof(PROTO_STREAM) == 4U, proto_stream_size);


/**
 * @brief Payload of PROTO_CMD_ACK.
 *
 * aRecSlot is the recorder slot map the audio side actually committed to.
 *
 * The interface MUST reconfigure its SPI de-interleave descriptor to match this
 * map before it trusts the recorded stream. Sequence:
 *      interface: SET_CONFIG  ->  audio: validate, ACK with slot map
 *      interface: reprogram MDMA  ->  audio: stream in the new format
 * Without this handshake there is a window in which recorded channels are
 * silently rotated on the SD card.
 */
typedef struct stPROTO_ACK
{
    U8 eResult;                             /**< PROTO_RESULT                    */
    U8 eEchoCmd;                            /**< command being acknowledged      */
    U8 nRecSlotQty;                         /**< slots in use, 0..REC_SLOT_QTY   */
    U8 nStreamWidth;                        /**< interleave stride, in samples   */
    U8 aRecSlot[CHAIN_MAX_QTY];             /**< first slot per chain, or REC_SLOT_NONE */

} PROTO_ACK;

FXC_STATIC_ASSERT(sizeof(PROTO_ACK) == 8U, proto_ack_size);


/**
 * @brief Payload of PROTO_CMD_TELEMETRY. Sent every TELEMETRY_PERIOD_MS.
 *
 * Everything the GUI header bar and meters need. Written by the audio ISR, read
 * by the super-loop; a torn read of a meter is harmless, and nSeq lets the
 * interface detect a torn frame if it ever cares.
 */
typedef struct stPROTO_TELEMETRY
{
    U32 aLoopPos[LOOPER_QTY];               /**< playhead, frames                */
    U32 aLoopLen[LOOPER_QTY];               /**< loop length, frames, 0 = empty  */

    U16 nSeq;
    U16 nCpuPermille;                       /**< 0..1000, last block             */
    U16 nCpuPeakPermille;                   /**< 0..1000, worst since boot       */
    U16 nOverruns;                          /**< blocks that missed the deadline */

    U16 aPeak[AUDIO_PLANE_QTY];             /**< 0..65535 -> 0.0..1.0            */
    U16 aRms[AUDIO_PLANE_QTY];

    U16 nTunerHzX10;                        /**< detected pitch * 10, 0 = none   */
    U8  nTunerConfidence;                   /**< 0..255                          */
    U8  nVersion;                           /**< PROTO_VERSION                   */

    U8  aTransport[CHAIN_MAX_QTY];          /**< PROTO_TRANSPORT_ACT per chain   */

} PROTO_TELEMETRY;

FXC_STATIC_ASSERT(sizeof(PROTO_TELEMETRY) == 48U, proto_telemetry_size);


/**
 * @brief Payload of PROTO_CMD_DIAG. Sent every TELEMETRY_DIAG_EVERY telemetry
 *        frames.
 *
 * Counters that should all be ZERO on a healthy board, kept OUT of
 * PROTO_TELEMETRY on purpose. Telemetry is exactly 48 bytes and both firmwares
 * already agree on that layout; growing it would break the contract for the
 * sake of numbers the GUI does not draw.
 *
 * These are bring-up instruments. nPhaseFaults in particular answers a question
 * that is otherwise very hard to ask: whether the two converters really are
 * sharing one frame sync, or whether channels 2 and 3 are quietly a sample out.
 *
 * An interface build that predates this command ignores it - a well formed
 * frame with an unknown command id is dropped, not resynchronised on.
 */
typedef struct stPROTO_DIAG
{
    U32 nBlocks;                            /**< audio blocks since start        */
    U32 nFramesOk;                          /**< control frames accepted         */
    U32 nCrcErrors;
    U32 nResyncs;
    U32 nRxOverflows;
    U32 nHpClips;                           /**< monitor sum clamped             */

    /*
     * The recorder SPI stream. nRecBlocksSent answers "is the link alive at
     * all", which is otherwise unanswerable from the interface side: a stream
     * that never starts and a stream of silence look identical on the wire.
     *
     * nRecDropped is the one that matters. A transfer takes half a block period
     * at this board's SPI clock, so it can only be non-zero if something is
     * stealing time from the audio interrupt - and the recording will have a
     * gap in it that nothing else reports.
     */
    U32 nRecBlocksSent;
    U32 nRecDropped;                        /**< no free staging half            */
    U32 nRecErrors;                         /**< SPI transfer failures           */

    U16 nPhaseFaults;                       /**< SAI2 not in step with SAI1      */
    U16 nStreamErrors;                      /**< converter error interrupts      */
    /*
     * Both read zero since loop audio moved from QSPI PSRAM into the SDRAM
     * banks: the window fill is a memcpy that completes inside the block that
     * asked for it, so there is no chain that can fail to drain and no transfer
     * that can fail to start.
     *
     * Kept rather than removed because PROTO_DIAG is pinned at 48 bytes and
     * both firmwares already agree on that layout - dropping two U16s would
     * cost a PROTO_VERSION bump and a lockstep flash to reclaim four bytes
     * nothing needs. They are the natural home for whatever replaces them.
     */
    U16 nLoopUnderruns;                     /**< reserved, reads 0               */
    U16 nLoopErrors;                        /**< reserved, reads 0               */

    U8  nVersion;                           /**< PROTO_VERSION                   */
    U8  bLoopMemOk;                         /**< loop SDRAM answered at boot     */
    U8  aReserved[2];

} PROTO_DIAG;

FXC_STATIC_ASSERT(sizeof(PROTO_DIAG) == 48U, proto_diag_size);


/***************************************************************************************************
* LOOP TRANSPORT
*
* Moving a recorded loop between the audio board's looper memory and the
* interface board's staging memory, from where it reaches the card.
*
* WHY THE BULK RIDES IN THE RECORDER STREAM
*
* The obvious design is a separate SPI transaction for loop data, framed by its
* own NSS pulse in the idle time between recorder bursts. It cannot be done
* safely here, for the reason PROTO_STREAM already gives: the recorder stream
* is a continuous interleave with no framing, received into a CIRCULAR DMA and
* de-interleaved BY POSITION. Re-arming that DMA between bursts to point at a
* different buffer is exactly the event that rotates every recorded channel
* into the wrong file, silently and with plausible audio in it.
*
* So loop bulk is carried as EXTRA SLOTS in the same frame. The stream widens
* from REC_SLOT_QTY to REC_SLOT_QTY + nSlotQty for the life of the session and
* narrows again afterwards. FxInterleave_Xfer is already parameterised on
* stream width, and because the loop slots are CONTIGUOUS they cost one MDMA
* route rather than one per slot - which matters, since the four recorder
* planes already use the channel and all three linked-list nodes.
*
* Widening the stream is a change of stream geometry, so it uses the same
* arming order PROTO_STREAM documents: the interface reprograms its routing
* first, and only then tells the audio side to begin.
*
* WHO INITIATES
*
* The interface, in BOTH directions. It owns the card and the staging memory,
* so it is the only side that knows whether there is room and whether the file
* exists. On a save the audio board still decides the byte count - it knows how
* long the loop is - which is why PROTO_LOOP_OPEN may ask with nBytes 0 and be
* told the answer in PROTO_LOOP_STAT.
*
* APPEND ONLY, AND NO VERSION BUMP
*
* Three commands and three structs are appended; no existing struct changes
* shape. By the doctrine over PROTO_VERSION that is a compatible change - a
* firmware predating it drops the frames rather than resynchronising on them,
* so an old audio build simply never transfers a loop. That is a missing
* feature, not a corrupted one.
***************************************************************************************************/

/** Which way the bulk moves. */
typedef enum enPROTO_LOOP_DIR
{
    LOOP_DIR_SAVE           = 0U,       /**< audio -> interface -> card          */
    LOOP_DIR_LOAD           = 1U        /**< card -> interface -> audio          */

} PROTO_LOOP_DIR;


/**
 * @brief Sample format ON THE WIRE, which is not the format in either memory.
 *
 * The audio board holds a loop as S32 so that overdubs accumulate with guard
 * bits, and the card wants packed 24-bit because that is what the WAV says.
 * Naming the wire format separately is what lets the narrowing happen once, in
 * whichever place has the cycles to spare, instead of being implied by both
 * ends and agreed by neither.
 */
typedef enum enPROTO_LOOP_FMT
{
    LOOP_FMT_S24            = 0U,       /**< 3 bytes per sample                  */
    LOOP_FMT_S32            = 1U        /**< 4 bytes per sample                  */

} PROTO_LOOP_FMT;


/** Session control actions. Payload of PROTO_CMD_LOOP_CTL. */
typedef enum enPROTO_LOOP_ACT
{
    LOOP_ACT_START          = 0U,       /**< routing is armed, begin the bulk    */
    LOOP_ACT_ABORT          = 1U        /**< tear down, whatever state it is in  */

} PROTO_LOOP_ACT;


/**
 * @brief Payload of PROTO_CMD_LOOP_OPEN. Interface -> audio.
 *
 * nBytes is 0 on a save to mean "you tell me" - the audio board answers in
 * PROTO_LOOP_STAT. On a load it is the size of the file, and the audio board
 * refuses if the loop will not fit.
 */
typedef struct stPROTO_LOOP_OPEN
{
    U32 nBytes;                 /**< payload bytes; 0 on SAVE = ask              */

    U8  nSession;               /**< wraps; a stale STAT is rejected by this     */
    U8  eDir;                   /**< PROTO_LOOP_DIR                              */
    U8  nLooper;                /**< 0 .. LOOPER_QTY-1                           */
    U8  nPlaneQty;              /**< 1 mono, 2 stereo                            */

    U8  eFormat;                /**< PROTO_LOOP_FMT                              */
    U8  nSlotQty;               /**< extra stream slots requested for the bulk   */
    U8  aReserved[2];

} PROTO_LOOP_OPEN;

FXC_STATIC_ASSERT(sizeof(PROTO_LOOP_OPEN) == 12U, proto_loop_open_size);


/** Payload of PROTO_CMD_LOOP_CTL. Interface -> audio. */
typedef struct stPROTO_LOOP_CTL
{
    U8  nSession;
    U8  eAction;                /**< PROTO_LOOP_ACT                              */
    U8  aReserved[2];

} PROTO_LOOP_CTL;

FXC_STATIC_ASSERT(sizeof(PROTO_LOOP_CTL) == 4U, proto_loop_ctl_size);


/**
 * @brief Payload of PROTO_CMD_LOOP_STAT. Audio -> interface.
 *
 * Sent in reply to OPEN, again on completion, and unsolicited on any failure.
 * nCrc is meaningful only once eState reaches FX_LOOP_COMPLETE: it covers the
 * payload exactly as it went onto the wire, so a bad lead shows up as a
 * reported mismatch rather than a loop file that plays back as noise.
 */
typedef struct stPROTO_LOOP_STAT
{
    U32 nBytes;                 /**< byte count agreed for this session          */
    U32 nCrc;                   /**< over the payload; valid on COMPLETE         */

    U8  nSession;
    U8  eState;                 /**< FX_LOOP_STATE                               */
    U8  eResult;                /**< PROTO_RESULT                                */
    U8  nSlotQty;               /**< slots actually granted, <= requested        */

} PROTO_LOOP_STAT;

FXC_STATIC_ASSERT(sizeof(PROTO_LOOP_STAT) == 12U, proto_loop_stat_size);


FXC_STATIC_ASSERT(sizeof(PROTO_CFG) <= PROTO_PAYLOAD_MAX, proto_payload_fits);
FXC_STATIC_ASSERT(sizeof(PROTO_LOOP_OPEN) <= PROTO_PAYLOAD_MAX, proto_loop_open_fits);
FXC_STATIC_ASSERT(sizeof(PROTO_LOOP_STAT) <= PROTO_PAYLOAD_MAX, proto_loop_stat_fits);




#ifdef __cplusplus
}
#endif

#endif // #ifndef FX_PROTOCOL_H

/****************************************** end of file *******************************************/
