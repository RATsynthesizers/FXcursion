/***************************************************************************************************
* @file     LoopSpool.c
*
* @brief    Loop staging slots and their card side. See LoopSpool.h.
*
***************************************************************************************************/

/***************************************************************************************************
* Included header files
***************************************************************************************************/

#include "LoopSpool.h"

#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"
#include "fatfs.h"
#include "common_cfg.h"
#include "fx_crc.h"

/* For Recorder_IsCaughtUp - the recorder's half of the turn-taking rule.
   Recorder.h does not include this file, so the two headers do not cycle. */
#include "Recorder.h"

/***************************************************************************************************
* Definitions of local (private) constants
***************************************************************************************************/

/*
 * The slots, from the linker - NOT fixed addresses and NOT arrays. An array
 * would have to be declared at some size, and that size would become a second
 * opinion about how big a slot is, free to disagree after the next map change.
 */
extern U8 _sloopslot_a[];
extern U8 _eloopslot_a[];
extern U8 _sloopslot_b[];
extern U8 _eloopslot_b[];

/**
 * Bytes moved per f_read / f_write.
 *
 * 32 KiB is 64 sectors: large enough that the per-call FatFs overhead
 * disappears against the transfer, small enough that the operation can notice
 * an abort between calls. Transfers go STRAIGHT to and from SDRAM - there is no
 * staging copy, because the loop payload is already the format the file wants.
 */
#define LOOPSPOOL_IO_BYTES              (32768UL)

#define LOOPSPOOL_WAV_BITS              (24U)
#define LOOPSPOOL_WAV_BYTES             (3UL)

/** How long the spool thread waits between checks while the recorder catches
    up. Short enough that the queued loop starts promptly, long enough not to
    spin: the recorder drains at about four times real time. */
#define LOOPSPOOL_YIELD_MS              (20U)

/** Slot lifecycle. */
typedef enum enSPOOL_STATE
{
    SPOOL_FREE      = 0U,   /**< nothing in it                                   */
    SPOOL_FILLING   = 1U,   /**< claimed, a transfer is receiving into it        */
    SPOOL_PENDING   = 2U,   /**< holds a loop, queued for the card               */
    SPOOL_WRITING   = 3U    /**< being written now                               */

} SPOOL_STATE;

/***************************************************************************************************
* Definitions of local (private) variables
***************************************************************************************************/

typedef struct stSPOOL_SLOT
{
    LOOPSPOOL_INFO tInfo;
    char           aName[LOOPSPOOL_NAME_MAX];
    U32            nSeq;            /**< queue order; 0 when not queued          */
    U8             eState;
    U8             nReserved[3];

} SPOOL_SLOT;

static SPOOL_SLOT aSlot[LOOPSPOOL_SLOT_QTY];

/** Increments on every commit, so the queue is FIFO among pending slots. */
static U32 nSeqNext = 1UL;

static osMutexId    xSdMutex    = NULL;
static osSemaphoreId xWork      = NULL;
static osThreadId   xSpoolThread = NULL;
static BOOLEAN      bInitDone   = FALSE;

/*
 * The tail of "busy". A write holds the card; bAwaitDrain outlives it, because
 * the recorder emerges from being locked out with a backlog that is still real
 * work. Cleared lazily in LoopSpool_IsBusy - there is no periodic context here
 * that could own it, and the condition only matters to whoever is asking.
 */
static volatile BOOLEAN bAwaitDrain = FALSE;

/*
 * FIL is around 550 bytes and deliberately not on any stack: the spool thread
 * is small, and the caller of a load is whichever thread happens to ask.
 * Static because only one card operation runs at a time - the lock guarantees
 * it.
 */
static FIL tFile;

/***************************************************************************************************
* Definitions of local (private) functions
***************************************************************************************************/

static U8* LoopSpool_SlotBase(const U8 nSlot)
{
    if (nSlot == 0U)
    {
        return _sloopslot_a;
    }

    if (nSlot == 1U)
    {
        return _sloopslot_b;
    }

    return NULL_PTR;
}


/**
 * @brief The queued slot that should go next, or LOOPSPOOL_SLOT_NONE.
 *
 * FIFO by commit order. With two slots this is barely a choice, but making the
 * order explicit is what stops it depending on array index - which would mean
 * slot A always beating slot B regardless of which loop the player recorded
 * first.
 */
static U8 LoopSpool_NextPending(void)
{
    U8  nBest = (U8)LOOPSPOOL_SLOT_NONE;
    U32 nBestSeq = 0xFFFFFFFFUL;
    U8  i;

    for (i = 0U; i < (U8)LOOPSPOOL_SLOT_QTY; i++)
    {
        if ((aSlot[i].eState == (U8)SPOOL_PENDING) && (aSlot[i].nSeq < nBestSeq))
        {
            nBestSeq = aSlot[i].nSeq;
            nBest    = i;
        }
    }

    return nBest;
}


/**
 * @brief Walk the RIFF chunks looking for "fmt " and "data".
 *
 * NOT a fixed 44-byte header read. A WAV written by a DAW routinely carries
 * LIST, fact or bext chunks before the data, and assuming the payload starts at
 * offset 44 would load those bytes as audio - a click, and then everything
 * offset. Since the whole point of loading is that a file from the user's PC
 * works, the reader has to walk the chunks.
 */
static STD_RESULT LoopSpool_ParseWav(FIL* const pFile,
                                     U16* const pnChannels,
                                     U16* const pnBits,
                                     U32* const pnRate,
                                     U32* const pnDataOfs,
                                     U32* const pnDataBytes)
{
    U8      aHdr[12];
    UINT    nRead = 0U;
    U32     nOfs;
    BOOLEAN bFmtSeen = FALSE;

    if ((f_read(pFile, aHdr, 12U, &nRead) != FR_OK) || (nRead != 12U))
    {
        return RESULT_NOT_OK;
    }

    if ((memcmp(&aHdr[0], "RIFF", 4) != 0) || (memcmp(&aHdr[8], "WAVE", 4) != 0))
    {
        return RESULT_NOT_OK;
    }

    nOfs = 12UL;

    for (;;)
    {
        U8  aChunk[8];
        U32 nChunkBytes;

        if (f_lseek(pFile, nOfs) != FR_OK)
        {
            return RESULT_NOT_OK;
        }

        if ((f_read(pFile, aChunk, 8U, &nRead) != FR_OK) || (nRead != 8U))
        {
            /* Ran off the end without finding "data". */
            return RESULT_NOT_OK;
        }

        nChunkBytes = ((U32)aChunk[4])
                    | ((U32)aChunk[5] << 8U)
                    | ((U32)aChunk[6] << 16U)
                    | ((U32)aChunk[7] << 24U);

        if (memcmp(aChunk, "fmt ", 4) == 0)
        {
            U8 aFmt[16];

            if (nChunkBytes < 16UL)
            {
                return RESULT_NOT_OK;
            }

            if ((f_read(pFile, aFmt, 16U, &nRead) != FR_OK) || (nRead != 16U))
            {
                return RESULT_NOT_OK;
            }

            *pnChannels = (U16)((U16)aFmt[2] | ((U16)aFmt[3] << 8U));
            *pnRate     = ((U32)aFmt[4])
                        | ((U32)aFmt[5] << 8U)
                        | ((U32)aFmt[6] << 16U)
                        | ((U32)aFmt[7] << 24U);
            *pnBits     = (U16)((U16)aFmt[14] | ((U16)aFmt[15] << 8U));

            bFmtSeen = TRUE;
        }
        else if (memcmp(aChunk, "data", 4) == 0)
        {
            if (bFmtSeen == FALSE)
            {
                /* data before fmt - nothing could be made of the payload. */
                return RESULT_NOT_OK;
            }

            *pnDataOfs   = nOfs + 8UL;
            *pnDataBytes = nChunkBytes;

            return RESULT_OK;
        }

        /* Chunks are word aligned: an odd length carries a pad byte that is not
           counted in the length. Missing this walks into the middle of the next
           chunk header and the search fails on a perfectly good file. */
        nOfs += 8UL + nChunkBytes + (nChunkBytes & 1UL);
    }
}


/**
 * @brief Write one staged slot to the card as a 24-bit WAV. Holds the lock.
 */
static STD_RESULT LoopSpool_WriteSlot(const U8 nSlot)
{
    const LOOPSPOOL_INFO* const pInfo = &aSlot[nSlot].tInfo;
    U8* const  pBase = LoopSpool_SlotBase(nSlot);
    WAV_Header tHdr;
    UINT       nWritten = 0U;
    U32        nDone    = 0UL;
    U32        nPayload;
    STD_RESULT eResult  = RESULT_OK;

    if (pBase == NULL_PTR)
    {
        return RESULT_NOT_OK;
    }

    /* Only 24-bit reaches the card. The audio board holds a loop as S32 for
       overdub headroom and narrows it on the way out; an S32 payload arriving
       here means the narrowing did not happen, and the file would be wrong in a
       way that still plays. */
    if (pInfo->eFormat != (U8)LOOP_FMT_S24)
    {
        return RESULT_INVALID_PARAM_1;
    }

    nPayload = pInfo->nBytes;

    if (LoopSpool_SdLock(osWaitForever) != RESULT_OK)
    {
        return RESULT_TIMEOUT;
    }

    memcpy(tHdr.chunkID,     "RIFF", 4);
    memcpy(tHdr.format,      "WAVE", 4);
    memcpy(tHdr.subchunk1ID, "fmt ", 4);
    memcpy(tHdr.subchunk2ID, "data", 4);

    tHdr.subchunk1Size = 16UL;
    tHdr.audioFormat   = 1U;
    tHdr.numChannels   = (U16)pInfo->nPlaneQty;
    tHdr.sampleRate    = LOOPSPOOL_SAMPLE_RATE;
    tHdr.bitsPerSample = LOOPSPOOL_WAV_BITS;
    tHdr.byteRate      = LOOPSPOOL_SAMPLE_RATE * (U32)pInfo->nPlaneQty * LOOPSPOOL_WAV_BYTES;
    tHdr.blockAlign    = (U16)((U32)pInfo->nPlaneQty * LOOPSPOOL_WAV_BYTES);
    tHdr.subchunk2Size = nPayload;
    tHdr.chunkSize     = 36UL + nPayload;

    if (f_open(&tFile, (const TCHAR*)aSlot[nSlot].aName,
               FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
    {
        LoopSpool_SdUnlock();
        return RESULT_NOT_OK;
    }

    /* The size is known before the first byte, unlike a recording, so the
       header is right on the first pass and f_expand can preallocate the whole
       file - one contiguous run, sparing the card a cluster-chain walk in the
       middle of it. */
    (void)f_expand(&tFile, (FSIZE_t)(sizeof(WAV_Header) + nPayload), 0);

    if (f_write(&tFile, &tHdr, (UINT)sizeof(WAV_Header), &nWritten) != FR_OK)
    {
        eResult = RESULT_NOT_OK;
    }

    while ((eResult == RESULT_OK) && (nDone < nPayload))
    {
        U32 nStep = nPayload - nDone;

        if (nStep > LOOPSPOOL_IO_BYTES)
        {
            nStep = LOOPSPOOL_IO_BYTES;
        }

        if (f_write(&tFile, &pBase[nDone], (UINT)nStep, &nWritten) != FR_OK)
        {
            eResult = RESULT_NOT_OK;
            break;
        }

        nDone += (U32)nWritten;

        if ((U32)nWritten != nStep)
        {
            /* Card full. Stop rather than spin. */
            eResult = RESULT_NOT_OK;
            break;
        }
    }

    /* Whatever happened, the header must describe what is actually there. A
       short file with a full-length header is the one outcome that plays as
       noise instead of failing. */
    if (nDone != nPayload)
    {
        tHdr.subchunk2Size = nDone;
        tHdr.chunkSize     = 36UL + nDone;

        if (f_lseek(&tFile, 0) == FR_OK)
        {
            (void)f_write(&tFile, &tHdr, (UINT)sizeof(WAV_Header), &nWritten);
        }
    }

    (void)f_close(&tFile);

    LoopSpool_SdUnlock();

    return eResult;
}


/**
 * @brief The spool thread: writes queued slots, letting the recorder go first.
 */
static void LoopSpool_Thread(void const* argument)
{
    (void)argument;

    for (;;)
    {
        U8 nSlot;

        /* Sleep until something is queued. The timeout is a safety net, not the
           mechanism - a commit signals directly. */
        (void)osSemaphoreWait(xWork, 1000U);

        nSlot = LoopSpool_NextPending();

        if (nSlot == (U8)LOOPSPOOL_SLOT_NONE)
        {
            continue;
        }

        /*
         * THE RECORDER GOES FIRST.
         *
         * If a loop write just finished, the recorder has been locked out for
         * its whole duration and is carrying a backlog. Starting the next
         * queued loop now would lock it out again on top of that, and a backlog
         * that never gets a chance to drain becomes an overrun - which drops
         * audio that cannot be recovered.
         *
         * So wait for it. This costs the queued loop a second or two and costs
         * the take nothing, which is the right way round: the loop is already
         * safely in SDRAM, the recorder's backlog is not.
         *
         * Recorder_IsCaughtUp is TRUE when the recorder is idle, so nothing
         * waits here when there is no recording running.
         */
        while (Recorder_IsCaughtUp() == FALSE)
        {
            osDelay(LOOPSPOOL_YIELD_MS);
        }

        aSlot[nSlot].eState = (U8)SPOOL_WRITING;

        (void)LoopSpool_WriteSlot(nSlot);

        /*
         * Free the slot, then mark the recovery. Order matters for IsBusy: if
         * another slot is still PENDING it keeps IsBusy TRUE by itself, and if
         * none is, bAwaitDrain carries it until the recorder has caught up. The
         * indicator never blinks false between the two writes.
         */
        aSlot[nSlot].eState = (U8)SPOOL_FREE;
        aSlot[nSlot].nSeq   = 0UL;

        bAwaitDrain = TRUE;

        /* Something else may already be queued - go round without sleeping. */
        if (LoopSpool_NextPending() != (U8)LOOPSPOOL_SLOT_NONE)
        {
            (void)osSemaphoreRelease(xWork);
        }
    }
}

/***************************************************************************************************
* Definitions of global (public) functions
***************************************************************************************************/

STD_RESULT LoopSpool_Init(void)
{
    U8 i;

    if (bInitDone == TRUE)
    {
        return RESULT_ALREDY_INIT;
    }

    /*
     * THE LINKER AND THE CONSTANT MUST AGREE.
     *
     * REC_LOOP_SLOT_BYTES is what the build-time arithmetic in Recorder.c
     * reasons about - that the loop route's step divides this slot exactly, so
     * a full-length take can always be armed. The real size is a linker symbol
     * difference and no static assert can see it.
     *
     * Resize SDRAM_LOOP_A in the linker script without touching the constant
     * and every one of those asserts is still checking the old number, which is
     * the worst kind of passing test. Fail init instead: a board that will not
     * start is a bug report, and a board that saves 511 of 512 possible loop
     * lengths is a mystery.
     */
    if (LoopSpool_SlotBytes() != (U32)REC_LOOP_SLOT_BYTES)
    {
        return RESULT_NOT_OK;
    }

    for (i = 0U; i < (U8)LOOPSPOOL_SLOT_QTY; i++)
    {
        (void)memset(&aSlot[i], 0, sizeof(aSlot[i]));
        aSlot[i].eState = (U8)SPOOL_FREE;
    }

    osMutexDef(loopSdMutex);
    xSdMutex = osMutexCreate(osMutex(loopSdMutex));

    if (xSdMutex == NULL)
    {
        return RESULT_NOT_OK;
    }

    osSemaphoreDef(loopSpoolWork);
    xWork = osSemaphoreCreate(osSemaphore(loopSpoolWork), 1);

    if (xWork == NULL)
    {
        return RESULT_NOT_OK;
    }

    /* Created given; take it so the thread sleeps until real work arrives. */
    (void)osSemaphoreWait(xWork, 0);

    osThreadDef(LoopSpoolThread, LoopSpool_Thread, osPriorityNormal, 0, 1024U);
    xSpoolThread = osThreadCreate(osThread(LoopSpoolThread), NULL);

    if (xSpoolThread == NULL)
    {
        return RESULT_NOT_OK;
    }

    bInitDone = TRUE;

    return RESULT_OK;
}


U8* LoopSpool_Buffer(const U8 nSlot)
{
    return LoopSpool_SlotBase(nSlot);
}


U32 LoopSpool_SlotBytes(void)
{
    return (U32)(_eloopslot_a - _sloopslot_a);
}


U8 LoopSpool_Acquire(void)
{
    U8 i;

    for (i = 0U; i < (U8)LOOPSPOOL_SLOT_QTY; i++)
    {
        if (aSlot[i].eState == (U8)SPOOL_FREE)
        {
            (void)memset(&aSlot[i].tInfo, 0, sizeof(aSlot[i].tInfo));
            aSlot[i].eState = (U8)SPOOL_FILLING;
            return i;
        }
    }

    return (U8)LOOPSPOOL_SLOT_NONE;
}


void LoopSpool_Release(const U8 nSlot)
{
    if (nSlot >= (U8)LOOPSPOOL_SLOT_QTY)
    {
        return;
    }

    /* Only a slot that is being filled may be dropped. One that is queued or
       being written belongs to the spool thread, and releasing it underneath
       would free memory a write is still reading. */
    if (aSlot[nSlot].eState == (U8)SPOOL_FILLING)
    {
        aSlot[nSlot].eState = (U8)SPOOL_FREE;
    }
}


STD_RESULT LoopSpool_Commit(const U8 nSlot,
                            const U32 nBytes,
                            const U8  nPlaneQty,
                            const U8  eFormat,
                            const U32 nCrc,
                            const char* const pName)
{
    U32 nSampleBytes;
    U32 nFrameBytes;

    if (nSlot >= (U8)LOOPSPOOL_SLOT_QTY)
    {
        return RESULT_INVALID_PARAM_1;
    }

    if (aSlot[nSlot].eState != (U8)SPOOL_FILLING)
    {
        return RESULT_NOT_OK;
    }

    if ((nPlaneQty != 1U) && (nPlaneQty != 2U))
    {
        return RESULT_INVALID_PARAM_3;
    }

    if ((eFormat != (U8)LOOP_FMT_S32) && (eFormat != (U8)LOOP_FMT_S24))
    {
        return RESULT_INVALID_PARAM_4;
    }

    if ((nBytes == 0UL) || (nBytes > LoopSpool_SlotBytes()))
    {
        return RESULT_INVALID_PARAM_2;
    }

    nSampleBytes = (eFormat == (U8)LOOP_FMT_S32) ? FX_LOOP_BYTES_S32
                                                 : FX_LOOP_BYTES_S24;
    nFrameBytes  = nSampleBytes * (U32)nPlaneQty;

    /* A byte count that is not a whole number of frames means a sample was
       lost, and the last frame would be half a stereo pair - a file whose
       channels swap at the end. */
    if ((nBytes % nFrameBytes) != 0UL)
    {
        return RESULT_INVALID_PARAM_2;
    }

    aSlot[nSlot].tInfo.nBytes    = nBytes;
    aSlot[nSlot].tInfo.nCrc      = nCrc;
    aSlot[nSlot].tInfo.nSamples  = nBytes / nFrameBytes;
    aSlot[nSlot].tInfo.nPlaneQty = nPlaneQty;
    aSlot[nSlot].tInfo.eFormat   = eFormat;

    if (pName != NULL_PTR)
    {
        (void)snprintf(aSlot[nSlot].aName, sizeof(aSlot[nSlot].aName), "%s", pName);
    }
    else
    {
        (void)snprintf(aSlot[nSlot].aName, sizeof(aSlot[nSlot].aName),
                       "loop%u.wav", (unsigned)nSlot);
    }

    /* Sequence BEFORE state, so the spool thread cannot see a pending slot
       whose queue position has not been assigned yet. */
    aSlot[nSlot].nSeq   = nSeqNext;
    nSeqNext++;

    aSlot[nSlot].eState = (U8)SPOOL_PENDING;

    (void)osSemaphoreRelease(xWork);

    return RESULT_OK;
}


const LOOPSPOOL_INFO* LoopSpool_Info(const U8 nSlot)
{
    if (nSlot >= (U8)LOOPSPOOL_SLOT_QTY)
    {
        return NULL_PTR;
    }

    if (aSlot[nSlot].eState == (U8)SPOOL_FREE)
    {
        return NULL_PTR;
    }

    return &aSlot[nSlot].tInfo;
}


U8 LoopSpool_PendingQty(void)
{
    U8 nQty = 0U;
    U8 i;

    for (i = 0U; i < (U8)LOOPSPOOL_SLOT_QTY; i++)
    {
        if ((aSlot[i].eState == (U8)SPOOL_PENDING) ||
            (aSlot[i].eState == (U8)SPOOL_WRITING))
        {
            nQty++;
        }
    }

    return nQty;
}


BOOLEAN LoopSpool_IsBusy(void)
{
    /* Anything queued or being written. This is what keeps the indicator up
       across the whole sequence: while loop A writes, while the recorder
       recovers, and while loop B writes - because B stays PENDING throughout
       the middle of that. */
    if (LoopSpool_PendingQty() > 0U)
    {
        return TRUE;
    }

    if (bAwaitDrain == TRUE)
    {
        /* Nothing queued, but the recorder is still working off what it could
           not write while the card was held. */
        if (Recorder_IsCaughtUp() == TRUE)
        {
            bAwaitDrain = FALSE;
        }
        else
        {
            return TRUE;
        }
    }

    return FALSE;
}


STD_RESULT LoopSpool_SdLock(const U32 nTimeoutMs)
{
    if (bInitDone == FALSE)
    {
        /* Before Init the recorder is the only user of the card, so there is
           nothing to exclude and refusing here would just stall it. */
        return RESULT_OK;
    }

    if (osMutexWait(xSdMutex, nTimeoutMs) != osOK)
    {
        return RESULT_TIMEOUT;
    }

    return RESULT_OK;
}


void LoopSpool_SdUnlock(void)
{
    if (bInitDone == TRUE)
    {
        (void)osMutexRelease(xSdMutex);
    }
}


STD_RESULT LoopSpool_Load(const char* const pName, U8* const pnSlot)
{
    char       aName[LOOPSPOOL_NAME_MAX];
    U8*        pBase;
    U8         nSlot;
    U16        nChannels  = 0U;
    U16        nBits      = 0U;
    U32        nRate      = 0UL;
    U32        nDataOfs   = 0UL;
    U32        nDataBytes = 0UL;
    U32        nDone      = 0UL;
    U32        nCrc       = 0UL;
    UINT       nRead      = 0U;
    STD_RESULT eResult;

    if (pnSlot == NULL_PTR)
    {
        return RESULT_NOT_OK;
    }

    *pnSlot = (U8)LOOPSPOOL_SLOT_NONE;

    /*
     * REFUSED, NOT QUEUED, WHILE THE MACHINE IS STILL BUSY.
     *
     * Waiting on the lock alone is not enough. After a save the lock is free
     * but the recorder is still working off the backlog that save created, and
     * a read starting now would lock it out a second time before it had
     * recovered - which is how a backlog becomes an overrun.
     *
     * Returning BUSY lets the caller show the same "saving" state the icon is
     * already showing and try again, rather than blocking a UI thread for
     * several seconds inside a file read.
     */
    if (LoopSpool_IsBusy() == TRUE)
    {
        return RESULT_BUSY;
    }

    nSlot = LoopSpool_Acquire();

    if (nSlot == (U8)LOOPSPOOL_SLOT_NONE)
    {
        return RESULT_BUSY;
    }

    pBase = LoopSpool_SlotBase(nSlot);

    if (pBase == NULL_PTR)
    {
        LoopSpool_Release(nSlot);
        return RESULT_NOT_OK;
    }

    if (pName != NULL_PTR)
    {
        (void)snprintf(aName, sizeof(aName), "%s", pName);
    }
    else
    {
        (void)snprintf(aName, sizeof(aName), "loop0.wav");
    }

    if (LoopSpool_SdLock(osWaitForever) != RESULT_OK)
    {
        LoopSpool_Release(nSlot);
        return RESULT_TIMEOUT;
    }

    if (f_open(&tFile, (const TCHAR*)aName, FA_READ) != FR_OK)
    {
        LoopSpool_SdUnlock();
        LoopSpool_Release(nSlot);
        return RESULT_NOT_OK;
    }

    eResult = LoopSpool_ParseWav(&tFile, &nChannels, &nBits, &nRate,
                                 &nDataOfs, &nDataBytes);

    /* Each refusal names the property that was wrong. This file may well be one
       the user copied from a PC, and "it did not load" gives them nothing to
       act on - whereas "wrong sample rate" is a thing they can fix. */
    if (eResult == RESULT_OK)
    {
        if (nRate != LOOPSPOOL_SAMPLE_RATE)
        {
            eResult = RESULT_INVALID_PARAM_2;
        }
        else if (nBits != LOOPSPOOL_WAV_BITS)
        {
            eResult = RESULT_INVALID_PARAM_3;
        }
        else if ((nChannels != 1U) && (nChannels != 2U))
        {
            eResult = RESULT_INVALID_PARAM_4;
        }
        else if ((nDataBytes == 0UL) || (nDataBytes > LoopSpool_SlotBytes()))
        {
            eResult = RESULT_INVALID_PARAM_5;
        }
        else
        {
            /* Trailing partial frame: trim rather than refuse. A data chunk a
               byte or two long is common enough, and dropping an incomplete
               frame loses 20 us rather than the whole loop. */
            const U32 nFrameBytes = LOOPSPOOL_WAV_BYTES * (U32)nChannels;

            nDataBytes -= (nDataBytes % nFrameBytes);

            if (nDataBytes == 0UL)
            {
                eResult = RESULT_INVALID_PARAM_5;
            }
        }
    }

    if (eResult == RESULT_OK)
    {
        if (f_lseek(&tFile, (FSIZE_t)nDataOfs) != FR_OK)
        {
            eResult = RESULT_NOT_OK;
        }
    }

    while ((eResult == RESULT_OK) && (nDone < nDataBytes))
    {
        U32 nStep = nDataBytes - nDone;

        if (nStep > LOOPSPOOL_IO_BYTES)
        {
            nStep = LOOPSPOOL_IO_BYTES;
        }

        if (f_read(&tFile, &pBase[nDone], (UINT)nStep, &nRead) != FR_OK)
        {
            eResult = RESULT_NOT_OK;
            break;
        }

        /* CRC as it lands, in the same pieces, so the value can be compared
           against what the audio board computes over the same bytes on the way
           back out - and against what a desktop crc32 says about the file. */
        nCrc   = Crc32_Ieee(&pBase[nDone], (U32)nRead, nCrc);
        nDone += (U32)nRead;

        if ((U32)nRead != nStep)
        {
            /* The data chunk claimed more than the file holds. Keep what was
               really there rather than sending uninitialised SDRAM to the audio
               board as audio. */
            break;
        }
    }

    (void)f_close(&tFile);

    /* Same recovery as a save: the recorder was locked out for the read. */
    bAwaitDrain = TRUE;

    LoopSpool_SdUnlock();

    if (eResult == RESULT_OK)
    {
        const U32 nFrameBytes = LOOPSPOOL_WAV_BYTES * (U32)nChannels;

        nDone -= (nDone % nFrameBytes);

        if (nDone == 0UL)
        {
            LoopSpool_Release(nSlot);
            return RESULT_NOT_OK;
        }

        aSlot[nSlot].tInfo.nBytes    = nDone;
        aSlot[nSlot].tInfo.nCrc      = nCrc;
        aSlot[nSlot].tInfo.nSamples  = nDone / nFrameBytes;
        aSlot[nSlot].tInfo.nPlaneQty = (U8)nChannels;
        aSlot[nSlot].tInfo.eFormat   = (U8)LOOP_FMT_S24;

        /* Stays FILLING: it is the transport's now, to send to the audio board
           and then release. Marking it PENDING would queue it straight back to
           the card, which is the one thing a load must not do. */
        *pnSlot = nSlot;
    }
    else
    {
        LoopSpool_Release(nSlot);
    }

    return eResult;
}

/****************************************** end of file *******************************************/
