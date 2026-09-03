/**
 * @file      fx_defs.h
 *
 * @details   Single definition of the effect pool: which effects exist, how many
 *            parameters each has, what those parameters are called and which of
 *            them can be tempo-synced.
 *
 *            ############################################################
 *            #  THIS FILE IS DUPLICATED IN THE INTERFACE CONTROLLER.    #
 *            #  The two copies MUST stay byte-identical. If you add an  #
 *            #  effect or a parameter here, copy this file and          #
 *            #  fx_defs.c to the interface project in the same commit.  #
 *            ############################################################
 *
 *            The audio side owns the physics (how a normalised 0..1 value maps
 *            to seconds, Hz or dB). The interface side owns the presentation
 *            (names, formatting, widget choice). This file is the contract
 *            between them and contains nothing else.
 *
 *            ------------------------------------------------------------------
 *            THERE ARE NO WIDTH-AGNOSTIC EFFECTS
 *            ------------------------------------------------------------------
 *
 *            Every effect is EITHER mono-only OR stereo-only. A mono delay and a
 *            stereo delay are two different effect types with two different
 *            parameter lists, not one effect with a width flag.
 *
 *            This is not bookkeeping - it is what the audio actually needs:
 *
 *              Delay      stereo gains Ping-pong and Spread
 *              Amp        mono has no Pan; stereo has Pan and Width
 *              Compressor stereo gains Link, the amount the two detectors share
 *              Tremolo    stereo gains Phase - at 180 degrees it is an auto-panner
 *              Reverb     stereo gains Width
 *              everything stereo modulation gains Spread, the LFO phase offset
 *                         between the two planes
 *
 *            The GUI may show both variants under the same name; the user picks
 *            "Delay" and the interface resolves which id to send from the chain's
 *            width. FX_VARIANT_FOR_WIDTH below does that in one line.
 *
 *            CONSEQUENCE, and it is the reason this is worth the extra ids:
 *            changing a chain from mono to stereo cannot preserve its effects,
 *            because a mono delay is not convertible into a stereo delay. The
 *            interface must send a cleared chain with the new topology, and the
 *            audio side refuses anything else - see PROTO_RES_BAD_WIDTH.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 * \date      01.09.2026 - 2.0.0 - mono and stereo split into distinct effect types
 *
 * @copyright RAT Synthesizers
 */

#ifndef FX_DEFS_H
#define FX_DEFS_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"

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

/* ------------------------------------------------------------------------------------------------
 * Shared dimensions.
 *
 * These are part of the wire protocol and the preset format, so both firmwares
 * must agree on them. They live here rather than in audio_cfg.h precisely so
 * there is only one definition. audio_cfg.h includes this file.
 * ---------------------------------------------------------------------------------------------- */

/** Sampling frequency, Hz. */
#define AUDIO_SAMPLE_RATE_HZ            (48000UL)

/** Physical mono I/O channels (2 stereo codecs). */
#define AUDIO_CH_QTY                    (4U)

/**
 * Audio planes. One plane is one mono stream inside a chain.
 *
 * INVARIANT: chain widths always sum to exactly AUDIO_CH_QTY, in every topology.
 * The plane count is therefore a compile-time constant, every per-plane array in
 * the audio controller is statically sized, and no allocator is needed anywhere.
 */
#define AUDIO_PLANE_QTY                 (AUDIO_CH_QTY)

/** Maximum simultaneous chains (reached in TOPO_4_MONO). */
#define CHAIN_MAX_QTY                   (AUDIO_CH_QTY)

/** Maximum planes in one chain (a stereo chain). */
#define CHAIN_MAX_WIDTH                 (2U)

/** Block slots per chain. GUI columns, signal flowing IN -> OUT; index 0 is IN. */
#define GRID_SLOT_QTY                   (4U)

/** Effect slots inside one FX block. */
#define FXBLOCK_SLOT_QTY                (4U)

/** Parameters per effect. */
#define FX_PARAM_QTY                    (8U)

/** Logical loopers. Planes 0..1 -> looper 0, planes 2..3 -> looper 1, always. */
#define LOOPER_QTY                      (2U)

/**
 * Longest recordable loop, seconds. The GUI needs this to bound its bar picker.
 *
 * WHY THIS DROPPED FROM 60 TO 20
 *
 * Loop audio used to live in QSPI PSRAM, where 64 MiB bought 60 s per plane and
 * there was no undo. It now lives in the two SDRAM banks, 11 MiB of each, and
 * that 11 MiB holds a stereo looper TWICE - the take and the pre-overdub
 * snapshot that undo restores:
 *
 *   11 MiB / 4 plane buffers = 2 883 584 B = 20.02 s
 *   20 s * 48000 * 3 B       = 2 880 000 B per plane, x4 = 11 520 000 B
 *
 * So the trade is 60 s without undo against 20 s with it, and no dependence on
 * a QSPI part at all. mem_map.h proves the arithmetic; if this is raised past
 * what the banks hold, the build fails on a named assertion rather than the
 * looper silently wrapping.
 *
 * THE INTERFACE STAGING SLOT IS SIZED FROM THIS. One active stereo loop is
 * 5 760 000 B and a staging slot is 5.5 MiB, so a loop that the audio board can
 * hold is always a loop the interface can accept. Raising this without raising
 * SDRAM_LOOP_A / SDRAM_LOOP_B in the interface linker script breaks that.
 *
 * On the 64 MiB parts this scales straight back up.
 */
#define LOOP_MAX_SEC                    (20U)

/** Recorder channels streamed to the interface controller over SPI. */
#define REC_SLOT_QTY                    (AUDIO_CH_QTY)

/** Marker for "this chain has no recorder slot assigned". */
#define REC_SLOT_NONE                   (0xFFU)

/** Marker for "no mixer placed on the grid". */
#define GRID_MIXER_COL_NONE             ((S8)-1)


/**
 * @brief Portable compile-time assertion.
 *
 * Used to pin wire structs to an exact size and to check memory budgets, so
 * that a mistake becomes a build error rather than a runtime surprise.
 * Pre-C11 compatible on purpose - the interface project may not enable C11.
 */
#define FXC_STATIC_ASSERT(cond, tag)    typedef char fxc_assert_##tag[(cond) ? 1 : -1]



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief Effect pool. Every entry is mono-only or stereo-only.
 *
 * Enum values are part of the wire protocol and of the preset format.
 * APPEND ONLY from here on - never renumber, never remove. To retire an effect,
 * leave its id in place and drop it from the GUI.
 *
 * LAYOUT GUARANTEE, relied on by FX_VARIANT_FOR_WIDTH and checked by the tests:
 * the two variants of one effect are ADJACENT, mono first at an EVEN id, stereo
 * second at that id plus one. Keep it that way when adding effects.
 */
typedef enum enFX_TYPE
{
    FX_AMP_M          = 0U,     FX_AMP_S          = 1U,
    FX_CHORUS_M       = 2U,     FX_CHORUS_S       = 3U,
    FX_COMPRESSOR_M   = 4U,     FX_COMPRESSOR_S   = 5U,
    FX_DELAY_M        = 6U,     FX_DELAY_S        = 7U,
    FX_DISTORTION_M   = 8U,     FX_DISTORTION_S   = 9U,
    FX_FLANGER_M      = 10U,    FX_FLANGER_S      = 11U,
    FX_OVERDRIVE_M    = 12U,    FX_OVERDRIVE_S    = 13U,
    FX_PHASER_M       = 14U,    FX_PHASER_S       = 15U,
    FX_REVERB_M       = 16U,    FX_REVERB_S       = 17U,
    FX_TREMOLO_M      = 18U,    FX_TREMOLO_S      = 19U,
    FX_VIBRATO_M      = 20U,    FX_VIBRATO_S      = 21U,

    FX_TYPE_QTY       = 22U,    /**< effects that exist today                    */
    FX_TYPE_NONE      = 0xFFU   /**< empty effect slot inside an FX block        */

} FX_TYPE;

/** Reserved id space. Raising this costs nothing until the ids are used. */
#define FX_TYPE_MAX                 (64U)

/**
 * @brief Pick the variant of an effect that fits a chain of the given width.
 *
 * The GUI holds one concept - "Delay" - as the mono id, and resolves the id to
 * send from the width of the chain the user dropped it into:
 *
 *     U8 eFx = FX_VARIANT_FOR_WIDTH(FX_DELAY_M, chainWidth);
 *
 * Relies on the adjacency guarantee above.
 */
#define FX_VARIANT_FOR_WIDTH(baseMonoId, nWidth)                                \
            ((U8)((U8)(baseMonoId) + (((nWidth) == CHAIN_MAX_WIDTH) ? 1U : 0U)))

/** TRUE when the id is the stereo variant. */
#define FX_IS_STEREO(eFxType)       ((((U8)(eFxType)) & 1U) != 0U)

/**
 * @brief Block types placeable in a grid slot.
 *
 * Part of the wire protocol. APPEND ONLY.
 *
 * NOTE: the grid is exactly saturated today - GRID_SLOT_QTY is 4 and there are
 * 4 block types with a one-per-chain rule, so a chain can hold exactly one of
 * each. Adding a fifth block type means a chain can no longer hold one of
 * everything. That is a UI decision, not a technical limit.
 */
typedef enum enBLOCK_TYPE
{
    BLOCK_NONE      = 0U,       /**< empty slot - passthrough                    */
    BLOCK_FX        = 1U,       /**< FX block, holds FXBLOCK_SLOT_QTY effects    */
    BLOCK_RECORDER  = 2U,       /**< tap to the interface controller over SPI    */
    BLOCK_LOOPER    = 3U,       /**< loop record / overdub / play                */
    BLOCK_MIXER     = 4U,       /**< occupies a WHOLE COLUMN across all chains   */

    BLOCK_TYPE_QTY  = 5U

} BLOCK_TYPE;

/**
 * @brief Input topology. Selects how the 4 physical mono channels group into chains.
 *
 * Part of the wire protocol and the preset format. APPEND ONLY.
 */
typedef enum enTOPOLOGY
{
    TOPO_4_MONO     = 0U,       /**< m1 | m2 | m3 | m4                           */
    TOPO_ST1_2MONO  = 1U,       /**< (m1+m2) | m3 | m4                           */
    TOPO_2MONO_ST2  = 2U,       /**< m1 | m2 | (m3+m4)                           */
    TOPO_2_STEREO   = 3U,       /**< (m1+m2) | (m3+m4)                           */

    TOPO_QTY        = 4U

} TOPOLOGY;

/**
 * @brief Parameter descriptor flags.
 */
typedef enum enFX_PARAM_FLAG
{
    FX_PF_NONE      = 0x00U,
    FX_PF_SYNCABLE  = 0x01U,    /**< GUI may offer a note-division picker        */
    FX_PF_STEPPED   = 0x02U     /**< value is a small integer, not continuous    */

} FX_PARAM_FLAG;

/**
 * @brief Note divisions available when a parameter is tempo-synced.
 *
 * Part of the wire protocol and the preset format. APPEND ONLY.
 * The quarter-note weight of each division lives in g_aDivQuarters[].
 */
typedef enum enNOTE_DIV
{
    DIV_1_1         = 0U,       /**< whole            */
    DIV_1_2D        = 1U,       /**< dotted half      */
    DIV_1_2         = 2U,
    DIV_1_2T        = 3U,       /**< half triplet     */
    DIV_1_4D        = 4U,
    DIV_1_4         = 5U,
    DIV_1_4T        = 6U,
    DIV_1_8D        = 7U,
    DIV_1_8         = 8U,
    DIV_1_8T        = 9U,
    DIV_1_16D       = 10U,
    DIV_1_16        = 11U,
    DIV_1_16T       = 12U,
    DIV_1_32        = 13U,

    DIV_QTY         = 14U

} NOTE_DIV;

/**
 * @brief One parameter as the user sees it.
 *
 * fValue is ALWAYS normalised 0..1. The mapping to seconds / Hz / dB lives in
 * the effect that owns the parameter, never in the GUI. When bSync is TRUE the
 * effect ignores fValue for time-like parameters and derives the value from the
 * tempo and eDivision instead.
 */
typedef struct stFX_PARAM
{
    FLOAT32 fValue;             /**< normalised 0..1                             */
    U8      bSync;              /**< TRUE: derive from tempo and eDivision       */
    U8      eDivision;          /**< NOTE_DIV, used when bSync is TRUE           */
    U8      nReserved[2];       /**< keep the struct 8 bytes and 4-byte aligned  */

} FX_PARAM;

/**
 * @brief Static description of one parameter.
 */
typedef struct stFX_PARAM_DESC
{
    const char* pName;          /**< short display name, <= 11 chars             */
    U8          nFlags;         /**< FX_PARAM_FLAG bitmask                       */

} FX_PARAM_DESC;

/**
 * @brief Static description of one effect.
 */
typedef struct stFX_DESC
{
    const char*          pName;         /**< display name; the two variants of one
                                             effect deliberately share it       */
    U8                   nParamQty;     /**< <= FX_PARAM_QTY                     */
    U8                   nWidth;        /**< 1 = mono-only, 2 = stereo-only.
                                             MUST equal the chain's width       */
    U8                   nReserved[2];
    const FX_PARAM_DESC* pParam;        /**< nParamQty entries                   */

} FX_DESC;

/**
 * @brief Global tempo state.
 *
 * CONVENTION, and both firmwares must agree on it:
 *   fBpm defines the QUARTER-NOTE duration, regardless of time signature.
 *   Bar length in quarter notes = nBeatsPerBar * (4 / nBeatUnit).
 *
 *   4/4 at 120 BPM -> 4 * 1.0 * 0.5 s = 2.00 s
 *   6/8 at 120 BPM -> 6 * 0.5 * 0.5 s = 1.50 s   (six eighths = three quarters)
 */
typedef struct stTEMPO
{
    FLOAT32 fBpm;               /**< quarter notes per minute                    */
    U8      nBeatsPerBar;       /**< time signature numerator, e.g. 6 in 6/8     */
    U8      nBeatUnit;          /**< time signature denominator, e.g. 8 in 6/8   */
    U8      eSource;            /**< TEMPO_SOURCE                                */
    U8      nReserved;
    U32     nSampleInBar;       /**< position inside the current bar, samples    */

} TEMPO;

/**
 * @brief Where the tempo comes from.
 *
 * Only TEMPO_SRC_INTERNAL is implemented today. TAP and MIDI write into the same
 * TEMPO struct, so adding them touches ctrl_link.c and nothing else.
 */
typedef enum enTEMPO_SOURCE
{
    TEMPO_SRC_INTERNAL = 0U,
    TEMPO_SRC_TAP      = 1U,    /**< not implemented yet                         */
    TEMPO_SRC_MIDI     = 2U     /**< not implemented yet                         */

} TEMPO_SOURCE;



/***************************************************************************************************
* Parameter indices
***************************************************************************************************/

/* Written out explicitly rather than generated by macro: these names are grepped
 * constantly while writing effects, and heavy macro generation would hide them.
 *
 * A stereo variant repeats its mono variant's parameters in the same order and
 * appends the stereo-only ones. That is a convention, not a rule the code
 * depends on, but it keeps the GUI's two parameter pages aligned. */

typedef enum { FX_AMPM_P_GAIN = 0U,
               FX_AMPM_P_QTY } FX_AMPM_PARAM;
typedef enum { FX_AMPS_P_GAIN = 0U, FX_AMPS_P_PAN, FX_AMPS_P_WIDTH,
               FX_AMPS_P_QTY } FX_AMPS_PARAM;

typedef enum { FX_CHORUSM_P_RATE = 0U, FX_CHORUSM_P_DEPTH, FX_CHORUSM_P_DELAY,
               FX_CHORUSM_P_MIX,
               FX_CHORUSM_P_QTY } FX_CHORUSM_PARAM;
typedef enum { FX_CHORUSS_P_RATE = 0U, FX_CHORUSS_P_DEPTH, FX_CHORUSS_P_DELAY,
               FX_CHORUSS_P_MIX, FX_CHORUSS_P_SPREAD,
               FX_CHORUSS_P_QTY } FX_CHORUSS_PARAM;

typedef enum { FX_COMPM_P_THRESHOLD = 0U, FX_COMPM_P_RATIO, FX_COMPM_P_ATTACK,
               FX_COMPM_P_RELEASE, FX_COMPM_P_MAKEUP,
               FX_COMPM_P_QTY } FX_COMPM_PARAM;
typedef enum { FX_COMPS_P_THRESHOLD = 0U, FX_COMPS_P_RATIO, FX_COMPS_P_ATTACK,
               FX_COMPS_P_RELEASE, FX_COMPS_P_MAKEUP, FX_COMPS_P_LINK,
               FX_COMPS_P_QTY } FX_COMPS_PARAM;

typedef enum { FX_DELAYM_P_TIME = 0U, FX_DELAYM_P_FEEDBACK, FX_DELAYM_P_TONE,
               FX_DELAYM_P_MIX,
               FX_DELAYM_P_QTY } FX_DELAYM_PARAM;
typedef enum { FX_DELAYS_P_TIME = 0U, FX_DELAYS_P_FEEDBACK, FX_DELAYS_P_TONE,
               FX_DELAYS_P_MIX, FX_DELAYS_P_PINGPONG, FX_DELAYS_P_SPREAD,
               FX_DELAYS_P_QTY } FX_DELAYS_PARAM;

typedef enum { FX_DISTM_P_DRIVE = 0U, FX_DISTM_P_TONE, FX_DISTM_P_LEVEL, FX_DISTM_P_MIX,
               FX_DISTM_P_QTY } FX_DISTM_PARAM;
typedef enum { FX_DISTS_P_DRIVE = 0U, FX_DISTS_P_TONE, FX_DISTS_P_LEVEL, FX_DISTS_P_MIX,
               FX_DISTS_P_SPREAD,
               FX_DISTS_P_QTY } FX_DISTS_PARAM;

typedef enum { FX_FLANGERM_P_RATE = 0U, FX_FLANGERM_P_DEPTH, FX_FLANGERM_P_FEEDBACK,
               FX_FLANGERM_P_MIX,
               FX_FLANGERM_P_QTY } FX_FLANGERM_PARAM;
typedef enum { FX_FLANGERS_P_RATE = 0U, FX_FLANGERS_P_DEPTH, FX_FLANGERS_P_FEEDBACK,
               FX_FLANGERS_P_MIX, FX_FLANGERS_P_SPREAD,
               FX_FLANGERS_P_QTY } FX_FLANGERS_PARAM;

typedef enum { FX_ODM_P_DRIVE = 0U, FX_ODM_P_BIAS, FX_ODM_P_LEVEL, FX_ODM_P_MIX,
               FX_ODM_P_QTY } FX_ODM_PARAM;
typedef enum { FX_ODS_P_DRIVE = 0U, FX_ODS_P_BIAS, FX_ODS_P_LEVEL, FX_ODS_P_MIX,
               FX_ODS_P_SPREAD,
               FX_ODS_P_QTY } FX_ODS_PARAM;

typedef enum { FX_PHASERM_P_RATE = 0U, FX_PHASERM_P_DEPTH, FX_PHASERM_P_FEEDBACK,
               FX_PHASERM_P_STAGES, FX_PHASERM_P_MIX,
               FX_PHASERM_P_QTY } FX_PHASERM_PARAM;
typedef enum { FX_PHASERS_P_RATE = 0U, FX_PHASERS_P_DEPTH, FX_PHASERS_P_FEEDBACK,
               FX_PHASERS_P_STAGES, FX_PHASERS_P_MIX, FX_PHASERS_P_SPREAD,
               FX_PHASERS_P_QTY } FX_PHASERS_PARAM;

/* SIZE scales every network line together: how big the room is, as opposed to
 * how long it rings for. The two are independent and both are wanted. */
typedef enum { FX_REVERBM_P_DECAY = 0U, FX_REVERBM_P_PREDELAY, FX_REVERBM_P_SIZE,
               FX_REVERBM_P_DAMPING, FX_REVERBM_P_DIFFUSION, FX_REVERBM_P_MIX,
               FX_REVERBM_P_QTY } FX_REVERBM_PARAM;
typedef enum { FX_REVERBS_P_DECAY = 0U, FX_REVERBS_P_PREDELAY, FX_REVERBS_P_SIZE,
               FX_REVERBS_P_DAMPING, FX_REVERBS_P_DIFFUSION, FX_REVERBS_P_MIX,
               FX_REVERBS_P_WIDTH,
               FX_REVERBS_P_QTY } FX_REVERBS_PARAM;

typedef enum { FX_TREMOLOM_P_RATE = 0U, FX_TREMOLOM_P_DEPTH, FX_TREMOLOM_P_SHAPE,
               FX_TREMOLOM_P_QTY } FX_TREMOLOM_PARAM;
typedef enum { FX_TREMOLOS_P_RATE = 0U, FX_TREMOLOS_P_DEPTH, FX_TREMOLOS_P_SHAPE,
               FX_TREMOLOS_P_PHASE,
               FX_TREMOLOS_P_QTY } FX_TREMOLOS_PARAM;

typedef enum { FX_VIBRATOM_P_RATE = 0U, FX_VIBRATOM_P_DEPTH,
               FX_VIBRATOM_P_QTY } FX_VIBRATOM_PARAM;
typedef enum { FX_VIBRATOS_P_RATE = 0U, FX_VIBRATOS_P_DEPTH, FX_VIBRATOS_P_SPREAD,
               FX_VIBRATOS_P_QTY } FX_VIBRATOS_PARAM;



/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/** Descriptor of every effect in the pool. Indexed by FX_TYPE. Lives in flash. */
extern const FX_DESC   g_aFxDesc[FX_TYPE_QTY];

/** Quarter-note weight of each note division. Indexed by NOTE_DIV. Lives in flash. */
extern const FLOAT32   g_aDivQuarters[DIV_QTY];

/** Display name of each note division. Indexed by NOTE_DIV. */
extern const char*const g_aDivName[DIV_QTY];




#ifdef __cplusplus
}
#endif

#endif // #ifndef FX_DEFS_H

/****************************************** end of file *******************************************/
