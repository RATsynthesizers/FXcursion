/**
 * @file      audio_sys.c
 *
 * @details   Block pipeline, meters and telemetry.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      31.08.2026 - First release
 *
 * @copyright RAT Synthesizers
 */



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "audio_sys.h"

#include "mem_map.h"
#include "grid.h"
#include "mixer.h"
#include "looper.h"
#include "loop_mem.h"
#include "recorder.h"
#include "params.h"
#include "Effects/fx_common.h"

#include <math.h>



/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/* AUDIO_SAMPLE_MAX / AUDIO_SAMPLE_MIN now live in audio_cfg.h - see the note
 * there about why all three clamping sites share one definition. */



/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * Working buffers. DTCM: zero wait state and never cached, so no cache
 * maintenance is ever needed on them.
 *
 * 4 planes x 64 frames x 4 B = 1 KiB. That is the entire working memory of the
 * audio path.
 */
static FLOAT32  aPlane[AUDIO_PLANE_QTY][AUDIO_BLOCK_FRAMES] IN_DTCM;
static FLOAT32* apPlane[AUDIO_PLANE_QTY]                    IN_DTCM;

/* Meters and load, written here, read by the super-loop. Torn reads are harmless. */
static FLOAT32  aPeak[AUDIO_PLANE_QTY] IN_DTCM;
static FLOAT32  aRms[AUDIO_PLANE_QTY]  IN_DTCM;

static U16      nCpuPermille     IN_DTCM;
static U16      nCpuPeakPermille IN_DTCM;
static U16      nOverruns        IN_DTCM;
static U16      nTelemSeq        IN_DTCM;
static U16      nClipCount       IN_DTCM;



/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static S32 ToHw(const FLOAT32 fValue)
{
    S32 nSample;

    // Hard clamp, not soft clip. A soft clip with unity small-signal gain still
    // bends the whole curve, which colours clean signals; clipping is counted
    // and reported instead so the player can see it on the meters and turn
    // something down. If you want a limiter here later, put it in as an explicit
    // output stage, not as a silent transfer curve.
    if (fValue >= 1.0f)
    {
        nSample = AUDIO_SAMPLE_MAX;
        nClipCount++;
    }
    else if (fValue <= -1.0f)
    {
        nSample = AUDIO_SAMPLE_MIN;
        nClipCount++;
    }
    else
    {
        nSample = (S32)(fValue * AUDIO_FULLSCALE);
    }

    return nSample;
}

//--------------------------------------------------------------------------------------------------

static U16 ToWire(const FLOAT32 fValue)
{
    const FLOAT32 f = FxUtil_Clamp(fValue, 0.0f, 1.0f);

    return (U16)(f * 65535.0f);
}



/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT AudioSys_Init(void)
{
    STD_RESULT eResult = RESULT_OK;
    U8         p;
    U16        i;

    // Order matters: Grid_Init must run before anything that calls Grid_Active().
    if (Params_Init()    != RESULT_OK) { eResult = RESULT_NOT_OK; }
    if (Grid_Init()      != RESULT_OK) { eResult = RESULT_NOT_OK; }
    if (Mixer_Init()     != RESULT_OK) { eResult = RESULT_NOT_OK; }
    if (LoopMem_Init() != RESULT_OK) { eResult = RESULT_NOT_OK; }
    if (Looper_Init()    != RESULT_OK) { eResult = RESULT_NOT_OK; }
    if (Recorder_Init()  != RESULT_OK) { eResult = RESULT_NOT_OK; }

    for (p = 0U; p < AUDIO_PLANE_QTY; p++)
    {
        apPlane[p] = aPlane[p];
        aPeak[p]   = 0.0f;
        aRms[p]    = 0.0f;

        for (i = 0U; i < AUDIO_BLOCK_FRAMES; i++)
        {
            aPlane[p][i] = 0.0f;
        }
    }

    // Every effect starts from a known state, not from whatever the SDRAM and
    // DTCM happened to contain at power-on.
    for (i = 0U; i < (U16)FX_TYPE_QTY; i++)
    {
        if (g_aFxEntry[i].pfReset != NULL_PTR)
        {
            g_aFxEntry[i].pfReset(0U, (U8)AUDIO_PLANE_QTY);
        }
    }

    nCpuPermille     = 0U;
    nCpuPeakPermille = 0U;
    nOverruns        = 0U;
    nTelemSeq        = 0U;
    nClipCount       = 0U;

    return eResult;
}

//--------------------------------------------------------------------------------------------------

void AudioSys_ProcessBlock(const S32* const pIn, S32* const pOut, const U16 nFrames)
{
    const U16 nCount = (nFrames > (U16)AUDIO_BLOCK_FRAMES) ? (U16)AUDIO_BLOCK_FRAMES : nFrames;
    U8        p;
    U16       i;

    /* 1. De-interleave and convert to planar FLOAT32.
     *
     * Planar, not interleaved: mono effects become "run the loop once", stereo
     * effects get clean access to both planes, and CMSIS-DSP block functions
     * become usable. On hardware this loop is a good candidate for MDMA, which
     * the interface controller already uses for exactly this in Recorder.c. */
    for (i = 0U; i < nCount; i++)
    {
        for (p = 0U; p < AUDIO_PLANE_QTY; p++)
        {
            aPlane[p][i] = (FLOAT32)pIn[((U32)i * AUDIO_CH_QTY) + p] * (1.0f / AUDIO_FULLSCALE);
        }
    }

    /* 2. Silence recorder slots that no chain taps this block. */
    Recorder_BeginBlock(nCount);

    /* 3. Take ownership of the loop windows back from the QSPI DMA. FALSE means
     *    the previous block's chain is still running, and the loopers will pass
     *    audio through untouched rather than race it. */
    (void)LoopMem_BeginBlock();

    /* 4. The engine. */
    Grid_Process(apPlane, nCount);

    /* 5. Hand the windows back. The transfers now run in the background,
     *    overlapping the metering and output conversion below, and have the
     *    rest of this block plus most of the next to complete. */
    LoopMem_Kick();

    /* 6. Meters, post-everything. */
    for (p = 0U; p < AUDIO_PLANE_QTY; p++)
    {
        FLOAT32 fPeak = 0.0f;
        FLOAT32 fSum  = 0.0f;

        for (i = 0U; i < nCount; i++)
        {
            const FLOAT32 fMag = fabsf(aPlane[p][i]);

            if (fMag > fPeak)
            {
                fPeak = fMag;
            }
            fSum += aPlane[p][i] * aPlane[p][i];
        }

        // Peak hold with decay, so the GUI sees a readable meter rather than one
        // block's worth of noise.
        aPeak[p] *= 0.90f;
        if (fPeak > aPeak[p])
        {
            aPeak[p] = fPeak;
        }

        aRms[p] = sqrtf(fSum / (FLOAT32)nCount);
    }

    /* 7. Convert and interleave the output. */
    for (i = 0U; i < nCount; i++)
    {
        for (p = 0U; p < AUDIO_PLANE_QTY; p++)
        {
            pOut[((U32)i * AUDIO_CH_QTY) + p] = ToHw(aPlane[p][i]);
        }
    }

    /* 8. Keep the bar position current for tempo-locked features. */
    Params_TempoAdvance(nCount);
}

//--------------------------------------------------------------------------------------------------

void AudioSys_ReportLoad(const U32 nCycles, const U32 nBudget)
{
    if (nBudget > 0UL)
    {
        U32 nPermille = (nCycles * 1000UL) / nBudget;

        if (nPermille > 65535UL)
        {
            nPermille = 65535UL;
        }

        nCpuPermille = (U16)nPermille;

        if (nCpuPermille > nCpuPeakPermille)
        {
            nCpuPeakPermille = nCpuPermille;
        }
    }
}

//--------------------------------------------------------------------------------------------------

void AudioSys_NotifyOverrun(void)
{
    if (nOverruns < 0xFFFFU)
    {
        nOverruns++;
    }
}

//--------------------------------------------------------------------------------------------------

void AudioSys_GetTelemetry(PROTO_TELEMETRY* const pTelem)
{
    U8 p;

    if (pTelem == NULL_PTR)
    {
        return;
    }

    pTelem->nVersion         = (U8)PROTO_VERSION;
    pTelem->nSeq             = nTelemSeq;
    pTelem->nCpuPermille     = nCpuPermille;
    pTelem->nCpuPeakPermille = nCpuPeakPermille;
    pTelem->nOverruns        = nOverruns;

    nTelemSeq++;

    for (p = 0U; p < AUDIO_PLANE_QTY; p++)
    {
        pTelem->aPeak[p] = ToWire(aPeak[p]);
        pTelem->aRms[p]  = ToWire(aRms[p]);
    }

    // TODO: pitch detection is a MODULE, not a service - see the note in the
    // README. Until FX_TUNER exists these stay zero.
    pTelem->nTunerHzX10      = 0U;
    pTelem->nTunerConfidence = 0U;

    Looper_GetTelemetry(pTelem);
}

/****************************************** end of file *******************************************/
