/**
 * @file      params.h
 *
 * @details   Parameter storage, tempo state and the preset blob.
 *
 *            ------------------------------------------------------------------
 *            ADDRESSING
 *            ------------------------------------------------------------------
 *
 *            A parameter is addressed by (chain, effect type, index). Because at
 *            most one instance of each effect type exists per chain, that
 *            address is unique and computable - there is no allocator, no
 *            handle, and no negotiation with the interface controller.
 *
 *                g_aFxParam[nChain][eFxType][nIdx]
 *
 *            Reordering effects inside an FX block therefore does not disturb
 *            their settings, which is what a user expects.
 *
 *            Cost: CHAIN_MAX_QTY * FX_TYPE_QTY * FX_PARAM_QTY * 8 B = 2816 B.
 *
 *            ------------------------------------------------------------------
 *            CONCURRENCY
 *            ------------------------------------------------------------------
 *
 *            Written by the super-loop, read by the audio ISR, with no lock.
 *            An aligned FLOAT32 store is atomic on Cortex-M7, so a value is
 *            never seen half-written. A parameter update does write three
 *            fields (value, bSync, eDivision) non-atomically, so for at most one
 *            block - 1.33 ms - the ISR can see a new value with the old
 *            division. That is inaudible on a knob and not worth a seqlock.
 *
 *            If a future parameter ever needs a genuinely atomic multi-field
 *            update, add a double-buffered copy for that parameter alone rather
 *            than locking the whole array.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef PARAMS_H
#define PARAMS_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"
#include "fx_defs.h"
#include "fx_protocol.h"



/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

/**
 * @brief Preset file header.
 *
 * The format is fixed NOW even though preset save/load is not implemented,
 * because the expensive part of adding it later is discovering that the stored
 * data lacks a field. In particular every loop carries fRecordedBpm (see
 * looper.h) so that tempo re-lock can be added without orphaning saved loops.
 *
 * Layout on disk:
 *      PRESET_HDR
 *      FX_PARAM  [nChainQty][nFxTypeQty][nParamQty]
 *      U16       CRC-16 over everything above
 */
typedef struct stPRESET_HDR
{
    U8        aMagic[4];        /**< 'F','X','C','P'                             */
    U16       nVersion;         /**< PRESET_VERSION                              */
    U16       nHdrBytes;        /**< sizeof(PRESET_HDR), for forward compat      */

    U8        nChainQty;        /**< CHAIN_MAX_QTY at save time                  */
    U8        nFxTypeQty;       /**< FX_TYPE_QTY at save time                    */
    U8        nParamQty;        /**< FX_PARAM_QTY at save time                   */
    U8        nReserved;

    PROTO_CFG tCfg;             /**< the whole grid, mixer and tempo             */

} PRESET_HDR;

#define PRESET_VERSION          (1U)

FXC_STATIC_ASSERT(sizeof(PRESET_HDR) == 108U, preset_hdr_size);



/***************************************************************************************************
* Declarations of global (public) functions
***************************************************************************************************/

/**
 * @brief Load every parameter with its default and set a default tempo.
 *
 * MUST be called before any audio runs. The parameter array lives in DTCM,
 * which the startup code neither copies nor zeroes, so before this call it
 * contains garbage.
 */
extern STD_RESULT Params_Init(void);

/**
 * @brief Parameter block of one effect instance.
 *
 * @return pointer to FX_PARAM_QTY entries, never NULL for valid arguments
 */
extern const FX_PARAM* Params_Get(const U8 nChain, const U8 eFxType);

/**
 * @brief Apply one parameter update from the control link.
 */
extern STD_RESULT Params_Set(const PROTO_SET_PARAM* const pCmd);

/**
 * @brief Restore one effect instance to its defaults.
 *
 * NOT called when an effect is added to a chain. Removing a delay and putting it
 * back returns it with the settings the user left on it, which is the friendlier
 * behaviour; only the effect's audio STATE is cleared on add (see grid.c).
 *
 * This is for an explicit "reset effect" from the GUI, and for factory init.
 */
extern STD_RESULT Params_ResetFx(const U8 nChain, const U8 eFxType);

/** Current tempo. Never NULL. */
extern const TEMPO* Params_Tempo(void);

/** Set tempo and time signature. nBpmX10 is BPM * 10. */
extern STD_RESULT Params_SetTempo(const U16 nBpmX10, const U8 nBeatsPerBar, const U8 nBeatUnit);

/**
 * @brief Length of one bar in frames, at the current tempo and time signature.
 *
 * Never returns 0. The looper multiplies this by its bar count to get a loop
 * length, so the maximum usable bar count depends on both BPM and time
 * signature - the GUI needs the same arithmetic to bound its bar picker.
 */
extern U32 Params_BarFrames(void);

/**
 * @brief Advance the bar-position counter. Called once per block from the audio ISR.
 *
 * Keeps TEMPO.nSampleInBar in [0 ; Params_BarFrames()), which the looper uses to
 * quantise transport changes to bar boundaries.
 */
extern void Params_TempoAdvance(const U16 nFrames);

/** Bytes a preset blob occupies. */
extern U32 Params_PresetSize(void);

/**
 * @brief Serialise the current parameters plus the given grid into a preset blob.
 *
 * @param pBuf      destination, at least Params_PresetSize() bytes
 * @param nBufLen   size of pBuf
 * @param pCfg      grid configuration to embed
 * @param pWritten  bytes written, may be NULL
 */
extern STD_RESULT Params_PresetWrite(U8* const pBuf,
                                     const U32 nBufLen,
                                     const PROTO_CFG* const pCfg,
                                     U32* const pWritten);

/**
 * @brief Validate and load a preset blob. Grid configuration is returned, not applied.
 *
 * The caller is responsible for handing pCfg to Grid_Apply, so that loading a
 * preset takes exactly the same validated path as a live configuration change.
 */
extern STD_RESULT Params_PresetRead(const U8* const pBuf,
                                    const U32 nLen,
                                    PROTO_CFG* const pCfg);



#endif // #ifndef PARAMS_H

/****************************************** end of file *******************************************/
