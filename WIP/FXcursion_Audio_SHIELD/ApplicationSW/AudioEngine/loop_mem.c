/***************************************************************************************************
* @file     loop_mem.c
*
* @brief    Loop audio memory and its per-block window. See loop_mem.h.
*
***************************************************************************************************/

/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "loop_mem.h"

#include <string.h>

/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/** Planes in one looper: a stereo pair. */
#define LOOP_PLANES_PER_LOOPER          (MEM_LOOP_PLANES_PER_LOOPER)

/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

/*
 * THE LOOP BUFFERS.
 *
 * Two arrays rather than one indexed by plane, because the two loopers are on
 * different chip selects and a single array cannot straddle two output
 * sections. LoopMem_PlaneBase is the only place that mapping is written down.
 *
 *   [copy][plane within the looper][bytes]
 *
 * 2 copies x 2 planes x LOOP_MAX_BYTES = 11 520 000 B per bank, against the
 * 11 MiB regions the linker script reserves. mem_map.h asserts that fit.
 */
static U8 aLoopA[LOOP_COPY_QTY][LOOP_PLANES_PER_LOOPER][MEM_LOOP_BYTES_PER_PLANE]
    IN_SDRAM_LOOP_A MEM_ALIGN(MEM_CACHE_LINE_BYTES);

static U8 aLoopB[LOOP_COPY_QTY][LOOP_PLANES_PER_LOOPER][MEM_LOOP_BYTES_PER_PLANE]
    IN_SDRAM_LOOP_B MEM_ALIGN(MEM_CACHE_LINE_BYTES);

/*
 * The windows, in DTCM. Zero wait state and never cached, so nothing here needs
 * cache maintenance and the loopers can touch a window as often as they like
 * within a block without paying SDRAM latency per sample.
 */
static U8 aWindow[AUDIO_PLANE_QTY][LOOP_WINDOW_BYTES] IN_DTCM MEM_ALIGN(4);

/** Where each window came from, and where it goes back. */
static U32     aPos[AUDIO_PLANE_QTY];
static U32     aLen[AUDIO_PLANE_QTY];
static BOOLEAN aValid[AUDIO_PLANE_QTY];
static BOOLEAN aArmed[AUDIO_PLANE_QTY];

/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

/**
 * @brief Copy between a window and the plane buffer, handling the loop wrap.
 *
 * THE WRAP IS THE WHOLE REASON THIS FUNCTION EXISTS. A block that straddles the
 * loop end is two runs in the buffer, not one. Every caller doing this itself
 * is how a click at the loop point appears at some loop lengths and not others.
 *
 * @param bOut FALSE reads the buffer into the window, TRUE writes it back
 */
static void LoopMem_Move(const U8 nPlane,
                         const U32 nPos,
                         const U32 nLen,
                         const BOOLEAN bOut)
{
    U8* const pBase = LoopMem_PlaneBase(nPlane, LOOP_COPY_TAKE);
    U32       nHead;
    U32       nTail;

    if ((pBase == NULL_PTR) || (nLen == 0UL))
    {
        return;
    }

    /* Frames from nPos to the loop end, capped at one window. */
    nHead = nLen - nPos;

    if (nHead > (U32)LOOP_WINDOW_FRAMES)
    {
        nHead = (U32)LOOP_WINDOW_FRAMES;
    }

    nTail = (U32)LOOP_WINDOW_FRAMES - nHead;

    if (bOut == TRUE)
    {
        (void)memcpy(&pBase[nPos * LOOP_BYTES_PER_SAMPLE],
                     &aWindow[nPlane][0],
                     (size_t)(nHead * LOOP_BYTES_PER_SAMPLE));

        if (nTail > 0UL)
        {
            (void)memcpy(&pBase[0],
                         &aWindow[nPlane][nHead * LOOP_BYTES_PER_SAMPLE],
                         (size_t)(nTail * LOOP_BYTES_PER_SAMPLE));
        }
    }
    else
    {
        (void)memcpy(&aWindow[nPlane][0],
                     &pBase[nPos * LOOP_BYTES_PER_SAMPLE],
                     (size_t)(nHead * LOOP_BYTES_PER_SAMPLE));

        if (nTail > 0UL)
        {
            (void)memcpy(&aWindow[nPlane][nHead * LOOP_BYTES_PER_SAMPLE],
                         &pBase[0],
                         (size_t)(nTail * LOOP_BYTES_PER_SAMPLE));
        }
    }
}

/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT LoopMem_Init(void)
{
    LoopMem_Invalidate();

    return RESULT_OK;
}


BOOLEAN LoopMem_BeginBlock(void)
{
    U8 p;

    /*
     * Fill anything armed since the last block. Synchronous, so a plane armed
     * from the super-loop is valid on the very next block rather than whenever
     * a DMA chain happened to drain.
     */
    for (p = 0U; p < (U8)AUDIO_PLANE_QTY; p++)
    {
        if (aArmed[p] == TRUE)
        {
            if (aLen[p] >= (U32)LOOP_WINDOW_FRAMES)
            {
                LoopMem_Move(p, aPos[p], aLen[p], FALSE);
                aValid[p] = TRUE;
            }

            aArmed[p] = FALSE;
        }
    }

    return TRUE;
}


BOOLEAN LoopMem_Ready(void)
{
    return TRUE;
}


BOOLEAN LoopMem_Valid(const U8 nPlane)
{
    if (nPlane >= (U8)AUDIO_PLANE_QTY)
    {
        return FALSE;
    }

    return aValid[nPlane];
}


U8* LoopMem_Window(const U8 nPlane)
{
    if (nPlane >= (U8)AUDIO_PLANE_QTY)
    {
        return NULL_PTR;
    }

    return &aWindow[nPlane][0];
}


void LoopMem_Arm(const U8 nPlane, const U32 nPos, const U32 nLen)
{
    if (nPlane >= (U8)AUDIO_PLANE_QTY)
    {
        return;
    }

    /* Zeroed and invalid straight away, so nothing stale is heard in the gap
       between arming and the fill. */
    (void)memset(&aWindow[nPlane][0], 0, (size_t)LOOP_WINDOW_BYTES);

    aValid[nPlane] = FALSE;
    aPos[nPlane]   = nPos;
    aLen[nPlane]   = nLen;
    aArmed[nPlane] = (nLen >= (U32)LOOP_WINDOW_FRAMES) ? TRUE : FALSE;
}


void LoopMem_ArmBlank(const U8 nPlane, const U32 nPos, const U32 nLen)
{
    if (nPlane >= (U8)AUDIO_PLANE_QTY)
    {
        return;
    }

    (void)memset(&aWindow[nPlane][0], 0, (size_t)LOOP_WINDOW_BYTES);

    aPos[nPlane]   = nPos;
    aLen[nPlane]   = nLen;
    aArmed[nPlane] = FALSE;

    /* Valid immediately: the caller is about to overwrite every sample, so
       there is nothing to read back and waiting a block would start the take
       one block late. */
    aValid[nPlane] = (nLen >= (U32)LOOP_WINDOW_FRAMES) ? TRUE : FALSE;
}


void LoopMem_Commit(const U8 nPlane,
                    const U32 nNextPos,
                    const U32 nLen,
                    const BOOLEAN bDirty)
{
    if (nPlane >= (U8)AUDIO_PLANE_QTY)
    {
        return;
    }

    if (nLen < (U32)LOOP_WINDOW_FRAMES)
    {
        aValid[nPlane] = FALSE;
        return;
    }

    /* Write back what was just used, from where it came from - aPos, not
       nNextPos. Using the new position here would put the block one window
       further along the loop than it belongs, which sounds like a stutter that
       gets worse the longer the loop runs. */
    if ((bDirty == TRUE) && (aValid[nPlane] == TRUE))
    {
        LoopMem_Move(nPlane, aPos[nPlane], nLen, TRUE);
    }

    aPos[nPlane] = nNextPos;
    aLen[nPlane] = nLen;

    LoopMem_Move(nPlane, nNextPos, nLen, FALSE);

    aValid[nPlane] = TRUE;
}


void LoopMem_Kick(void)
{
    /* Nothing to start. Kept so audio_sys.c did not have to lose the symmetry
       of opening and closing the block. */
}


void LoopMem_Invalidate(void)
{
    U8 p;

    for (p = 0U; p < (U8)AUDIO_PLANE_QTY; p++)
    {
        (void)memset(&aWindow[p][0], 0, (size_t)LOOP_WINDOW_BYTES);

        aPos[p]   = 0UL;
        aLen[p]   = 0UL;
        aValid[p] = FALSE;
        aArmed[p] = FALSE;
    }
}


U32 LoopMem_Underruns(void)
{
    return 0UL;
}


U32 LoopMem_Errors(void)
{
    return 0UL;
}


U8* LoopMem_PlaneBase(const U8 nPlane, const U8 nCopy)
{
    if ((nPlane >= (U8)AUDIO_PLANE_QTY) || (nCopy >= (U8)LOOP_COPY_QTY))
    {
        return NULL_PTR;
    }

    /* Planes 0,1 are looper 0 in bank 1; planes 2,3 are looper 1 in bank 2.
       The one place that mapping is written down. */
    if (nPlane < (U8)LOOP_PLANES_PER_LOOPER)
    {
        return &aLoopA[nCopy][nPlane][0];
    }

    return &aLoopB[nCopy][nPlane - (U8)LOOP_PLANES_PER_LOOPER][0];
}


U32 LoopMem_PlaneBytes(void)
{
    return (U32)MEM_LOOP_BYTES_PER_PLANE;
}


STD_RESULT LoopMem_Snapshot(const U8 nLooper,
                            const U32 nFrames,
                            const BOOLEAN bRestore)
{
    const U32 nBytes = nFrames * (U32)LOOP_BYTES_PER_SAMPLE;
    U8        p;

    if (nLooper >= (U8)LOOPER_QTY)
    {
        return RESULT_INVALID_PARAM_1;
    }

    if ((nFrames == 0UL) || (nBytes > MEM_LOOP_BYTES_PER_PLANE))
    {
        return RESULT_INVALID_PARAM_2;
    }

    for (p = 0U; p < (U8)LOOP_PLANES_PER_LOOPER; p++)
    {
        const U8  nPlane = (U8)((nLooper * (U8)LOOP_PLANES_PER_LOOPER) + p);
        U8* const pTake  = LoopMem_PlaneBase(nPlane, (U8)LOOP_COPY_TAKE);
        U8* const pUndo  = LoopMem_PlaneBase(nPlane, (U8)LOOP_COPY_UNDO);

        if ((pTake == NULL_PTR) || (pUndo == NULL_PTR))
        {
            return RESULT_NOT_OK;
        }

        /*
         * Both copies are in the SAME bank, so this is a copy within one chip
         * select. Across banks it would be two chip selects contending on the
         * one FMC data bus for the whole of an overdub arm.
         */
        if (bRestore == TRUE)
        {
            (void)memcpy(pTake, pUndo, (size_t)nBytes);
        }
        else
        {
            (void)memcpy(pUndo, pTake, (size_t)nBytes);
        }
    }

    return RESULT_OK;
}

/****************************************** end of file *******************************************/
