/**
 * @file      fx_registry.c
 *
 * @details   The effect registry: one row per FX_TYPE, in flash.
 *
 *            This table IS the dispatch mechanism. There are no vtables, no
 *            per-instance function pointers, and no module objects - an effect
 *            "instance" is just the pair (chain, plane) applied to one of these
 *            rows.
 *
 *            Row order must match the FX_TYPE enum in fx_defs.h exactly, mono
 *            variant first. The static assertion at the bottom catches the case
 *            where an effect was added to the enum but not here; the tests catch
 *            a variant pair that got out of order.
 *
 * @version   2.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - 2.0.0 - mono and stereo split
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "fx_common.h"

#include "fx_amp.h"
#include "fx_compressor.h"
#include "fx_reverb.h"
#include "fx_delay.h"
#include "fx_overdrive.h"
#include "fx_tremolo.h"
#include "fx_distortion.h"
#include "fx_modulation.h"
#include "fx_phaser.h"



/***************************************************************************************************
* Definitions of global (public) variables
***************************************************************************************************/

const FX_ENTRY g_aFxEntry[FX_TYPE_QTY] =
{
    /* FX_AMP_M        */ { FxAmpM_Process,         FxAmpM_Reset         },
    /* FX_AMP_S        */ { FxAmpS_Process,         FxAmpS_Reset         },
    /* FX_CHORUS_M     */ { FxChorusM_Process,      FxChorusM_Reset      },
    /* FX_CHORUS_S     */ { FxChorusS_Process,      FxChorusS_Reset      },
    /* FX_COMPRESSOR_M */ { FxCompressorM_Process,  FxCompressorM_Reset  },
    /* FX_COMPRESSOR_S */ { FxCompressorS_Process,  FxCompressorS_Reset  },
    /* FX_DELAY_M      */ { FxDelayM_Process,       FxDelayM_Reset       },
    /* FX_DELAY_S      */ { FxDelayS_Process,       FxDelayS_Reset       },
    /* FX_DISTORTION_M */ { FxDistortionM_Process,  FxDistortionM_Reset  },
    /* FX_DISTORTION_S */ { FxDistortionS_Process,  FxDistortionS_Reset  },
    /* FX_FLANGER_M    */ { FxFlangerM_Process,     FxFlangerM_Reset     },
    /* FX_FLANGER_S    */ { FxFlangerS_Process,     FxFlangerS_Reset     },
    /* FX_OVERDRIVE_M  */ { FxOverdriveM_Process,   FxOverdriveM_Reset   },
    /* FX_OVERDRIVE_S  */ { FxOverdriveS_Process,   FxOverdriveS_Reset   },
    /* FX_PHASER_M     */ { FxPhaserM_Process,      FxPhaserM_Reset      },
    /* FX_PHASER_S     */ { FxPhaserS_Process,      FxPhaserS_Reset      },
    /* FX_REVERB_M     */ { FxReverbM_Process,      FxReverbM_Reset      },
    /* FX_REVERB_S     */ { FxReverbS_Process,      FxReverbS_Reset      },
    /* FX_TREMOLO_M    */ { FxTremoloM_Process,     FxTremoloM_Reset     },
    /* FX_TREMOLO_S    */ { FxTremoloS_Process,     FxTremoloS_Reset     },
    /* FX_VIBRATO_M    */ { FxVibratoM_Process,     FxVibratoM_Reset     },
    /* FX_VIBRATO_S    */ { FxVibratoS_Process,     FxVibratoS_Reset     },
};

FXC_STATIC_ASSERT((sizeof(g_aFxEntry) / sizeof(g_aFxEntry[0])) == (U32)FX_TYPE_QTY,
                  fx_registry_complete);

/****************************************** end of file *******************************************/
