/**
 * @file      fx_defs.c
 *
 * @details   Const tables described by fx_defs.h. All of it lives in flash.
 *
 *            ############################################################
 *            #  DUPLICATED IN THE INTERFACE CONTROLLER - keep in sync.  #
 *            ############################################################
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - 2.0.0 - mono and stereo split into distinct types
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_defs.h"



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/* ---- Amp ------------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aAmpMParam[FX_AMPM_P_QTY] =
{
    { "Gain",       FX_PF_NONE     },
};
static const FX_PARAM_DESC aAmpSParam[FX_AMPS_P_QTY] =
{
    { "Gain",       FX_PF_NONE     },
    { "Pan",        FX_PF_NONE     },
    { "Width",      FX_PF_NONE     },      /* mid/side width - meaningless mono   */
};

/* ---- Chorus ---------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aChorusMParam[FX_CHORUSM_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Delay",      FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
};
static const FX_PARAM_DESC aChorusSParam[FX_CHORUSS_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Delay",      FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
    { "Spread",     FX_PF_NONE     },      /* LFO phase offset between L and R    */
};

/* ---- Compressor ------------------------------------------------------------------------------ */
static const FX_PARAM_DESC aCompMParam[FX_COMPM_P_QTY] =
{
    { "Threshold",  FX_PF_NONE     },
    { "Ratio",      FX_PF_NONE     },
    { "Attack",     FX_PF_NONE     },
    { "Release",    FX_PF_NONE     },
    { "Makeup",     FX_PF_NONE     },
};
static const FX_PARAM_DESC aCompSParam[FX_COMPS_P_QTY] =
{
    { "Threshold",  FX_PF_NONE     },
    { "Ratio",      FX_PF_NONE     },
    { "Attack",     FX_PF_NONE     },
    { "Release",    FX_PF_NONE     },
    { "Makeup",     FX_PF_NONE     },
    { "Link",       FX_PF_NONE     },      /* 0 = dual mono, 1 = one detector     */
};

/* ---- Delay ----------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aDelayMParam[FX_DELAYM_P_QTY] =
{
    { "Time",       FX_PF_SYNCABLE },
    { "Feedback",   FX_PF_NONE     },
    { "Tone",       FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
};
static const FX_PARAM_DESC aDelaySParam[FX_DELAYS_P_QTY] =
{
    { "Time",       FX_PF_SYNCABLE },
    { "Feedback",   FX_PF_NONE     },
    { "Tone",       FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
    { "Ping-pong",  FX_PF_NONE     },      /* how much feedback crosses over      */
    { "Spread",     FX_PF_NONE     },      /* R tap offset from L, 0..100%        */
};

/* ---- Distortion ------------------------------------------------------------------------------ */
static const FX_PARAM_DESC aDistMParam[FX_DISTM_P_QTY] =
{
    { "Drive",      FX_PF_NONE     },
    { "Tone",       FX_PF_NONE     },
    { "Level",      FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
};
static const FX_PARAM_DESC aDistSParam[FX_DISTS_P_QTY] =
{
    { "Drive",      FX_PF_NONE     },
    { "Tone",       FX_PF_NONE     },
    { "Level",      FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
    { "Spread",     FX_PF_NONE     },      /* drive offset between the two sides  */
};

/* ---- Flanger --------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aFlangerMParam[FX_FLANGERM_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Feedback",   FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
};
static const FX_PARAM_DESC aFlangerSParam[FX_FLANGERS_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Feedback",   FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
    { "Spread",     FX_PF_NONE     },
};

/* ---- Overdrive ------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aOdMParam[FX_ODM_P_QTY] =
{
    { "Drive",      FX_PF_NONE     },
    { "Bias",       FX_PF_NONE     },
    { "Level",      FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
};
static const FX_PARAM_DESC aOdSParam[FX_ODS_P_QTY] =
{
    { "Drive",      FX_PF_NONE     },
    { "Bias",       FX_PF_NONE     },
    { "Level",      FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
    { "Spread",     FX_PF_NONE     },      /* R driven harder than L, for width   */
};

/* ---- Phaser ---------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aPhaserMParam[FX_PHASERM_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Feedback",   FX_PF_NONE     },
    { "Stages",     FX_PF_STEPPED  },
    { "Mix",        FX_PF_NONE     },
};
static const FX_PARAM_DESC aPhaserSParam[FX_PHASERS_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Feedback",   FX_PF_NONE     },
    { "Stages",     FX_PF_STEPPED  },
    { "Mix",        FX_PF_NONE     },
    { "Spread",     FX_PF_NONE     },
};

/* ---- Reverb ---------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aReverbMParam[FX_REVERBM_P_QTY] =
{
    { "Decay",      FX_PF_NONE     },
    { "Pre-delay",  FX_PF_SYNCABLE },
    { "Size",       FX_PF_NONE     },      /* how big the room is                 */
    { "Damping",    FX_PF_NONE     },
    { "Diffusion",  FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
};
static const FX_PARAM_DESC aReverbSParam[FX_REVERBS_P_QTY] =
{
    { "Decay",      FX_PF_NONE     },
    { "Pre-delay",  FX_PF_SYNCABLE },
    { "Size",       FX_PF_NONE     },      /* how big the room is                 */
    { "Damping",    FX_PF_NONE     },
    { "Diffusion",  FX_PF_NONE     },
    { "Mix",        FX_PF_NONE     },
    { "Width",      FX_PF_NONE     },      /* how decorrelated the tail is        */
};

/* ---- Tremolo --------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aTremoloMParam[FX_TREMOLOM_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Shape",      FX_PF_NONE     },
};
static const FX_PARAM_DESC aTremoloSParam[FX_TREMOLOS_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Shape",      FX_PF_NONE     },
    { "Phase",      FX_PF_NONE     },      /* 0 = together, 180 = auto-panner     */
};

/* ---- Vibrato --------------------------------------------------------------------------------- */
static const FX_PARAM_DESC aVibratoMParam[FX_VIBRATOM_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
};
static const FX_PARAM_DESC aVibratoSParam[FX_VIBRATOS_P_QTY] =
{
    { "Rate",       FX_PF_SYNCABLE },
    { "Depth",      FX_PF_NONE     },
    { "Spread",     FX_PF_NONE     },
};



/***************************************************************************************************
* Definitions of global (public) variables
***************************************************************************************************/

/*
 * The two variants of each effect share a display name on purpose: the user
 * picks "Delay" and the interface resolves the id from the chain's width with
 * FX_VARIANT_FOR_WIDTH. What differs is the parameter list, and in every case
 * the stereo variant has at least one parameter that is meaningless in mono.
 *
 * nWidth must match the chain's width EXACTLY. There is no width-agnostic
 * effect, which is why a mono chain turning stereo cannot keep its effects.
 */
const FX_DESC g_aFxDesc[FX_TYPE_QTY] =
{                          /* name         params                    w   reserved  param table    */
    /* FX_AMP_M        */ { "Amp",        (U8)FX_AMPM_P_QTY,        1U, {0U,0U}, aAmpMParam     },
    /* FX_AMP_S        */ { "Amp",        (U8)FX_AMPS_P_QTY,        2U, {0U,0U}, aAmpSParam     },
    /* FX_CHORUS_M     */ { "Chorus",     (U8)FX_CHORUSM_P_QTY,     1U, {0U,0U}, aChorusMParam  },
    /* FX_CHORUS_S     */ { "Chorus",     (U8)FX_CHORUSS_P_QTY,     2U, {0U,0U}, aChorusSParam  },
    /* FX_COMPRESSOR_M */ { "Compressor", (U8)FX_COMPM_P_QTY,       1U, {0U,0U}, aCompMParam    },
    /* FX_COMPRESSOR_S */ { "Compressor", (U8)FX_COMPS_P_QTY,       2U, {0U,0U}, aCompSParam    },
    /* FX_DELAY_M      */ { "Delay",      (U8)FX_DELAYM_P_QTY,      1U, {0U,0U}, aDelayMParam   },
    /* FX_DELAY_S      */ { "Delay",      (U8)FX_DELAYS_P_QTY,      2U, {0U,0U}, aDelaySParam   },
    /* FX_DISTORTION_M */ { "Distortion", (U8)FX_DISTM_P_QTY,       1U, {0U,0U}, aDistMParam    },
    /* FX_DISTORTION_S */ { "Distortion", (U8)FX_DISTS_P_QTY,       2U, {0U,0U}, aDistSParam    },
    /* FX_FLANGER_M    */ { "Flanger",    (U8)FX_FLANGERM_P_QTY,    1U, {0U,0U}, aFlangerMParam },
    /* FX_FLANGER_S    */ { "Flanger",    (U8)FX_FLANGERS_P_QTY,    2U, {0U,0U}, aFlangerSParam },
    /* FX_OVERDRIVE_M  */ { "Overdrive",  (U8)FX_ODM_P_QTY,         1U, {0U,0U}, aOdMParam      },
    /* FX_OVERDRIVE_S  */ { "Overdrive",  (U8)FX_ODS_P_QTY,         2U, {0U,0U}, aOdSParam      },
    /* FX_PHASER_M     */ { "Phaser",     (U8)FX_PHASERM_P_QTY,     1U, {0U,0U}, aPhaserMParam  },
    /* FX_PHASER_S     */ { "Phaser",     (U8)FX_PHASERS_P_QTY,     2U, {0U,0U}, aPhaserSParam  },
    /* FX_REVERB_M     */ { "Reverb",     (U8)FX_REVERBM_P_QTY,     1U, {0U,0U}, aReverbMParam  },
    /* FX_REVERB_S     */ { "Reverb",     (U8)FX_REVERBS_P_QTY,     2U, {0U,0U}, aReverbSParam  },
    /* FX_TREMOLO_M    */ { "Tremolo",    (U8)FX_TREMOLOM_P_QTY,    1U, {0U,0U}, aTremoloMParam },
    /* FX_TREMOLO_S    */ { "Tremolo",    (U8)FX_TREMOLOS_P_QTY,    2U, {0U,0U}, aTremoloSParam },
    /* FX_VIBRATO_M    */ { "Vibrato",    (U8)FX_VIBRATOM_P_QTY,    1U, {0U,0U}, aVibratoMParam },
    /* FX_VIBRATO_S    */ { "Vibrato",    (U8)FX_VIBRATOS_P_QTY,    2U, {0U,0U}, aVibratoSParam },
};

/* Weight of each division in QUARTER NOTES. See the TEMPO convention in fx_defs.h. */
const FLOAT32 g_aDivQuarters[DIV_QTY] =
{
    /* DIV_1_1   */ 4.000000f,
    /* DIV_1_2D  */ 3.000000f,
    /* DIV_1_2   */ 2.000000f,
    /* DIV_1_2T  */ 1.333333f,
    /* DIV_1_4D  */ 1.500000f,
    /* DIV_1_4   */ 1.000000f,
    /* DIV_1_4T  */ 0.666667f,
    /* DIV_1_8D  */ 0.750000f,
    /* DIV_1_8   */ 0.500000f,
    /* DIV_1_8T  */ 0.333333f,
    /* DIV_1_16D */ 0.375000f,
    /* DIV_1_16  */ 0.250000f,
    /* DIV_1_16T */ 0.166667f,
    /* DIV_1_32  */ 0.125000f,
};

const char* const g_aDivName[DIV_QTY] =
{
    "1/1", "1/2.", "1/2", "1/2T",
    "1/4.", "1/4", "1/4T",
    "1/8.", "1/8", "1/8T",
    "1/16.", "1/16", "1/16T",
    "1/32",
};

/****************************************** end of file *******************************************/
