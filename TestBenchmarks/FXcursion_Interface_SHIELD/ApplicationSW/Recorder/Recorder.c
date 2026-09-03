/*
 * Recorder.c
 *
 *  Created on: 22 февр. 2026 г.
 *      Author: ga
 */

#include "Recorder.h"

#include "stdio.h"

#include "fatfs.h"

#include "pubsub.h"

#include "cmsis_os.h"

#include "common_cfg.h"

#include "mdma.h"

/* The SPI peripheral, its clock and its callbacks belong to the transport now;
   nothing in this file touches hspi1. */
#include "spi_tp.h"

/* Stream geometry, shared byte for byte with the audio controller so the two
   boards cannot disagree about where a slot is. */
#include "fx_interleave.h"
#include "ctrl_link_if.h"

/* The card is shared with the loop spooler, which owns the lock. */
#include "LoopSpool.h"

/* Fills the transmit half during a loop load. Recorder.h does not include this,
   so the two headers do not cycle. */
#include "LoopSession.h"

/** One de-interleave destination: which slots, and where they go. */
typedef struct stREC_ROUTE
{
    uint8_t  nSlot;
    uint8_t  nWidth;
    uint32_t nDstAddr;

} REC_ROUTE;

/**
 * One open file, and which planes feed it.
 *
 * This replaced four arrays - wavFiles, fileChannelCount, fileChannelMap and
 * totalDataBytes - that were all indexed by the same counter. Four parallel
 * arrays is the shape that lets one of them get out of step with the others,
 * and the drain below has to walk them together on every pass.
 */
typedef struct stREC_SINK
{
    FIL*     pFile;
    uint32_t nDataBytes;      /**< running total, for the header on close   */
    uint8_t  aPlane[2];       /**< physical planes into recorder[]          */
    uint8_t  nPlaneQty;       /**< 1 or 2                                   */

} REC_SINK;

/* Should never move. Non-static so it can be read in a debugger without
   ceremony: if it is climbing, the ACKed stream layout and the recorder
   routing disagree and nothing is being recorded. */
volatile uint32_t recDeinterleaveRefused = 0;

/* The MDMA block repeat count is a 12-bit field, so one de-interleave cannot
   cover more than 4096 frames. At REC_RX_HALF_FRAMES of 1024 the ring can still
   grow twice; a third time would silently truncate every transfer instead of
   failing, which is why this is pinned rather than commented. */
FXC_STATIC_ASSERT(REC_RX_HALF_FRAMES <= 4096U, rec_half_fits_mdma_block_repeat);

/* Below this the writer would never wake, because the difference between the
   two counters can never reach the threshold. */
FXC_STATIC_ASSERT(REC_WRITE_CHUNKS < REC_CHUNKS, rec_write_batch_fits_ring);
FXC_STATIC_ASSERT(REC_WRITE_CHUNKS >= 1U, rec_write_batch_nonzero);

volatile uint8_t halfBufferFlag = 0;

/*
 * RING POSITION: two free-running counters, one owner each.
 *
 * recWrChunks is written ONLY by the MDMA completion callback and recRdChunks
 * ONLY by the recorder thread, so neither needs a lock - each is a single
 * 32-bit aligned store, atomic on this core, and no code path ever
 * read-modify-writes the other side's counter.
 *
 * They count chunks completed and chunks consumed since boot, and never wrap
 * to a ring index themselves. That is deliberate: a bare pair of ring indices
 * cannot tell "empty" from "completely full" - both read as equal - whereas
 * the DIFFERENCE of two free-running counters says exactly how much audio is
 * waiting, right up to and past the point of overrun, which is what makes the
 * overrun detectable at all.
 *
 * Unsigned subtraction stays correct across the 32-bit wrap. At 48000/1024 =
 * 46.875 chunks per second that wrap is 2.9 years of continuous recording
 * away, but the arithmetic does not care either way.
 *
 * These replaced bufferIndex, a uint8_t that wrapped at REC_CHUNKS. It was
 * also a trap for the ring resize: at REC_CHUNKS 3200 a uint8_t silently
 * truncates, and the de-interleave would have written four planes' worth of
 * audio into the first 256 chunks forever.
 */
volatile uint32_t recWrChunks = 0UL;
volatile uint32_t recRdChunks = 0UL;

/* Chunks the MDMA overwrote before the writer reached them. Non-static so it
   can be watched without ceremony: anything but zero means audio is missing
   from the take, and the card or REC_CHUNKS is where to look. */
volatile uint32_t recOverruns = 0UL;

/*
 * TRUE while files are open. Without it "is the recorder caught up" cannot be
 * answered: the MDMA keeps advancing recWrChunks whenever the stream is
 * running, so an idle recorder has an ever-growing backlog that nobody is
 * behind on. Only a recorder that is actually writing can be late.
 */
volatile uint8_t recIsWriting = 0U;

/*
 * ======== THE LOOP ROUTE'S DESTINATION ========
 *
 * Where the de-interleave puts the loop slots while a transfer is running, and
 * how far along it has got. Set by Recorder_ArmLoopDest when a session starts,
 * cleared when it ends; the de-interleave advances the offset itself so the
 * write position cannot drift from the number of blocks that actually landed.
 *
 * WORDS, NOT PACKED BYTES. The MDMA writes what arrives on the wire, which is
 * one 32-bit slot per sample carrying 24 bits of payload - it cannot narrow on
 * the way past. So the staging area fills with S32 and something has to pack it
 * to 3 bytes before it reaches the card. Deciding WHERE that happens is part of
 * wiring the session up, and it changes how big a staging slot has to be: 4/3
 * of the payload if it lands as S32, exactly the payload if the pack happens
 * first. Nothing here presumes either.
 */
static volatile uint32_t nLoopDstBase = 0UL;
static volatile uint32_t nLoopDstOfs  = 0UL;
static volatile uint32_t nLoopDstEnd  = 0UL;
static volatile uint8_t  bLoopDstArmed = 0U;

/** Loop blocks refused because the destination was full. Non-zero means the
    session and the staging area disagree about how much is coming. */
volatile uint32_t recLoopOverruns = 0UL;

/* Was .RAM_D1, which the D-cache now covers. SPI1 fills this by DMA, but
   RecorderInit zeroes it with the CPU first - so a dirty cache line could
   evict later, landing on top of audio that has since arrived.

   S32 now, not uint16: the wire carries REC_SLOTS_PER_FRAME slots of S32 per
   frame. Held at REC_RX_FRAMES frames, so the buffer doubled to 32 KiB and
   every downstream ratio stayed put. */
int32_t audioRxBuffer[REC_RX_WORDS] IN_DMA_BUF;

/*
 * The transmit side of the same frames, for loading a loop INTO the audio
 * board.
 *
 * ARMED FOR THE LIFE OF THE LINK, not just during a load. The SPI is
 * full-duplex and the master clocks both directions whether or not this side
 * has anything to say, so there is always something on MISO; the only question
 * is whether it is meaningful. Arming once and leaving it means the receive
 * side is never re-armed - and re-arming a positionally framed stream is the
 * one thing that rotates every recorder channel into the wrong file.
 *
 * The cost is 32 KiB of RAM_D2 that spends most of its life full of zeros.
 * That is the price of never touching a running stream.
 */
int32_t audioTxBuffer[REC_RX_WORDS] IN_DMA_BUF;

/*
 * De-interleaved audio, PLANAR: one ring per physical channel, S32.
 *
 * This replaced six rings - four mono plus recorderCh12/recorderCh34 - which
 * cost 4 MiB to hold 2 MiB of distinct audio. The stereo pair rings existed
 * only so the MDMA could move a pair as one contiguous 8-byte beat, which is
 * already WAV frame order; 2 MiB of SDRAM to save an interleave step in the
 * writer is not a trade worth making on this board.
 *
 * Planar also makes the de-interleave indifferent to stereo: every slot goes
 * to its own plane, so the routing is four identical transfers and "stereo"
 * becomes purely a question of how the writer reads these - which is where a
 * file-format decision belongs.
 */
int32_t recorder[REC_SLOTS_PER_FRAME][RECORD_BUF_SAMPLES]
    __attribute__((section(".sdram_rec")));


static void HAL_MDMA_XferCpltCallback(MDMA_HandleTypeDef *hmdma);
static void HAL_MDMA_XferErrorCallback(MDMA_HandleTypeDef *hmdma);

static STD_RESULT RecorderThreadInit(void);
static void MDMA_Trigger_Deinterleave(void);

/* Defined below MDMA_Trigger_Deinterleave but called from the recorder
   thread above it, so it needs declaring here. */
static uint32_t RecorderWritePacked24(FIL* const pFile,
                                      const int32_t* const pSrcL,
                                      const int32_t* const pSrcR,
                                      const uint32_t nFrames);

static uint32_t RecorderDrainChunks(REC_SINK* const aSink,
                                    const uint8_t  nSinkQty,
                                    const uint32_t nFirstChunk,
                                    const uint32_t nChunkQty);

static uint32_t RecorderClaimChunks(void);

/* Registered with SPI_TP in RecorderInit, defined near the de-interleave it
   drives. */
static void Recorder_OnSpiHalf(const BOOLEAN bSecondHalf);

static STD_RESULT RecorderLoopDest(uint32_t* const pnAddr, const uint8_t nSlotQty);

static void RecorderThreadWrapper(void const *arg);

static osThreadId xRecorderThreadHandle;

static SUB_HANDLE xRecorderHandle;

osSemaphoreId sem_RecorderData;

osSemaphoreId sem_RecStart;

RecorderInfo_t recInfo =
{
		.mono = { FALSE, FALSE, FALSE, FALSE },
		.stereo1 = FALSE,
		.stereo2 = FALSE
};

STD_RESULT RecorderInit()
{
	if (RESULT_OK != RecorderThreadInit())
	{
		return RESULT_NOT_OK;
	}

	HAL_MDMA_RegisterCallback(&hmdma_mdma_channel1_sw_0, HAL_MDMA_XFER_CPLT_CB_ID, HAL_MDMA_XferCpltCallback);
	HAL_MDMA_RegisterCallback(&hmdma_mdma_channel1_sw_0, HAL_MDMA_XFER_ERROR_CB_ID, HAL_MDMA_XferErrorCallback);

	for(U32 i = 0; i < REC_RX_WORDS; i++)
	{
		audioRxBuffer[i] = 0;
	}
	/* The peripheral, its clock and its callbacks belong to SPI_TP now. This
	   used to call HAL_SPI_Receive_DMA on hspi1 and own the two Rx callbacks
	   directly; the transport owns those weak symbols because it also owns the
	   error path and the transmit side on the audio board. */
	if (SPI_TP_Init() != RESULT_OK)
	{
		return RESULT_NOT_OK;
	}

	if (SPI_TP_RegisterHalfCb(&Recorder_OnSpiHalf) != RESULT_OK)
	{
		return RESULT_NOT_OK;
	}

	/* Size counts DATA FRAMES, and SPI1 is SPI_DATASIZE_32BIT, so this is S32
	   words - REC_RX_WORDS of them, 32 KiB.

	   ARMED BEFORE THE AUDIO BOARD IS TOLD TO STREAM. The stream is
	   de-interleaved by POSITION with no framing of its own, so joining it
	   mid-block rotates every channel into the wrong file, silently and with
	   plausible audio in it. CtrlLinkIf_Stream is what says "go", and it is
	   sent later - see the note over PROTO_STREAM. */
	for(U32 i = 0; i < REC_RX_WORDS; i++)
	{
		audioTxBuffer[i] = 0;
	}

	/* FULL DUPLEX, armed once. Receive carries the recorder stream and, during
	   a save, the loop; transmit carries a loop being loaded and zeros the rest
	   of the time. See audioTxBuffer for why it is always armed. */
	if (SPI_TP_StartTransceive(audioRxBuffer, audioTxBuffer,
	                           (U16)REC_RX_WORDS) != RESULT_OK)
	{
		return RESULT_NOT_OK;
	}

	return RESULT_OK;
}

static STD_RESULT RecorderThreadInit(void)
{
	osSemaphoreDef(semRecData);
	sem_RecorderData = osSemaphoreCreate(osSemaphore(semRecData), 1);

    if (NULL == sem_RecorderData)
    {
        return RESULT_NOT_OK;
    }
	osSemaphoreWait(sem_RecorderData, 0); // Initialize to empty

	osSemaphoreDef(semRecStart);
	sem_RecStart = osSemaphoreCreate(osSemaphore(semRecStart), 1);

    if (NULL == sem_RecStart)
    {
        return RESULT_NOT_OK;
    }
	osSemaphoreWait(sem_RecStart, 0); // Initialize to empty

	osThreadDef(RecorderThread, RecorderThreadWrapper, osPriorityRealtime, 0, 8192U);
	xRecorderThreadHandle = osThreadCreate(osThread(RecorderThread), NULL);

    if (NULL == xRecorderThreadHandle)
    {
        return RESULT_NOT_OK;
    }

    return RESULT_OK;
}

/**
 * @fn        void UISurveyThreadWrapper(void const *argument)
 *
 * @brief     Thread for hardware UI state update.
 *
 * @param[in] argument - pointer to input arguments.
 *
 * @return    None.
 */
static void RecorderThreadWrapper(void const *argument)
{
    PUBSUB_CreateTopic(PUBSUB_TOPIC_REC, sizeof(RecorderInfo_t));
    xRecorderHandle = PUBSUB_Subscribe(PUBSUB_TOPIC_REC, NULL);

    static U8 state = 0;

    static RecorderInfo_t activeConfig = {0};

    FIL wavFiles[MAX_RECORD_FILES];
    WAV_Header headers[MAX_RECORD_FILES];
    UINT bytesWritten;

    /* Which planes feed which file, and how much has gone into each. */
    REC_SINK aSink[MAX_RECORD_FILES];

    /* No write pointers: with planar rings a file is identified by the
       planes in aSink[i], and the writer indexes recorder[] directly. */
    uint8_t activeFileCount = 0;

    BOOLEAN isFileOpen = FALSE;

    for(;;)
    {
        if (osOK == osSemaphoreWait(sem_RecStart, 0))
        {
            if(state == 0)
            {
            	memcpy(&activeConfig, &recInfo, sizeof(RecorderInfo_t));
                state = 1;
            }
            else if(state == 1)
            {
                state = 2;
            }
        }

        switch(state)
        {
        // =========================================================
        case 1: // RECORDING
        // =========================================================
            if (!isFileOpen)
            {
                activeFileCount = 0;
                BOOLEAN chUsed[4] = {FALSE, FALSE, FALSE, FALSE};

                // ---- Handle stereo first ----
                if(activeConfig.stereo1)
                {
                    aSink[activeFileCount].pFile      = &wavFiles[activeFileCount];
                    aSink[activeFileCount].nDataBytes = 0UL;
                    aSink[activeFileCount].nPlaneQty  = 2U;
                    aSink[activeFileCount].aPlane[0]  = 0U;
                    aSink[activeFileCount].aPlane[1]  = 1U;

                    chUsed[0] = chUsed[1] = TRUE;
                    activeFileCount++;
                }

                if(activeConfig.stereo2)
                {
                    aSink[activeFileCount].pFile      = &wavFiles[activeFileCount];
                    aSink[activeFileCount].nDataBytes = 0UL;
                    aSink[activeFileCount].nPlaneQty  = 2U;
                    aSink[activeFileCount].aPlane[0]  = 2U;
                    aSink[activeFileCount].aPlane[1]  = 3U;

                    chUsed[2] = chUsed[3] = TRUE;
                    activeFileCount++;
                }

                // ---- Handle mono ----
				for(int ch = 0; ch < 4; ch++)
				{
					if(activeConfig.mono[ch] && !chUsed[ch])
					{
                        aSink[activeFileCount].pFile      = &wavFiles[activeFileCount];
                        aSink[activeFileCount].nDataBytes = 0UL;
                        aSink[activeFileCount].nPlaneQty  = 1U;
                        aSink[activeFileCount].aPlane[0]  = (uint8_t)ch;
                        aSink[activeFileCount].aPlane[1]  = 0U;

						activeFileCount++;
					}
				}

				if(activeFileCount == 0)
				{
					state = 0;
					break;
				}

                // ---- Open files ----
                for(int i = 0; i < activeFileCount; i++)
                {
                    WAV_Header *h = &headers[i];

                    memcpy(h->chunkID, "RIFF", 4);
                    memcpy(h->format, "WAVE", 4);
                    memcpy(h->subchunk1ID, "fmt ", 4);
                    memcpy(h->subchunk2ID, "data", 4);

                    h->subchunk1Size = 16;
                    h->audioFormat = 1;
                    h->numChannels = aSink[i].nPlaneQty;
                    h->sampleRate = SAMPLE_RATE;
                    /* 24-bit, packed to three bytes per sample on the card.
                       The codec and the wire are both 24-bit; truncating to 16
                       here would throw away the bottom 8 bits of every take. */
                    h->bitsPerSample = REC_WAV_BITS;
                    h->byteRate = SAMPLE_RATE * h->numChannels * REC_WAV_BYTES_PER_SAMPLE;
                    h->blockAlign = h->numChannels * REC_WAV_BYTES_PER_SAMPLE;
                    h->subchunk2Size = 0;
                    h->chunkSize = 36;

                    char filename[20];

                    if(h->numChannels == 2)
                    {
                        if(aSink[i].aPlane[0] == 0)
                            strcpy(filename, "recStereo12.wav");
                        else
                            strcpy(filename, "recStereo34.wav");
                    }
                    else
                    {
                        sprintf(filename, "recMono%d.wav",
                                aSink[i].aPlane[0] + 1);
                    }

                    if(f_open(&wavFiles[i],
                              (const TCHAR*)filename,
                              FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
                    {
                    	f_expand(&wavFiles[i], 50000000UL, 0);
                        f_write(&wavFiles[i], h, sizeof(WAV_Header), &bytesWritten);
                    }
                }

                // Clear any pending semaphores before starting
				osSemaphoreWait(sem_RecorderData, 0);

				/* Start from live audio. Everything already in the ring is from
				   before the user pressed record, and without this the first
				   pass would write up to a whole ring of it into the file. */
				recRdChunks = recWrChunks;

                isFileOpen   = TRUE;
                recIsWriting = 1U;
            }

			/* Sleep until there is a batch worth taking to the card. */
			osSemaphoreWait(sem_RecorderData, osWaitForever);

			/* Then drain everything that has arrived, not just one batch. A
			   card that stalled for two seconds leaves ~94 chunks waiting; the
			   loop clears the backlog at full speed instead of dribbling out
			   one batch per wake and never catching up. */
			/* THE CARD IS SHARED WITH THE LOOP STORE.

			   Taken around the whole drain, not per file: the point is that a
			   card sees one sequential stream at a time, and releasing between
			   files would let a loop write interleave into the middle of a set
			   of takes. Waiting forever is correct - the ring is 5.46 s deep
			   and a loop write is a few seconds, so the backlog this builds is
			   a fraction of the ring and drains at four times real time
			   afterwards. */
			if(RESULT_OK == LoopSpool_SdLock(osWaitForever))
			{
				for(;;)
				{
					const uint32_t nAvail = RecorderClaimChunks();
					uint32_t       nStart;

					if(nAvail < (uint32_t)REC_WRITE_CHUNKS)
					{
						break;
					}

					/* Read once into a local rather than "recRdChunks +=
					   Drain(..., recRdChunks, ...)". recRdChunks is volatile,
					   and in a compound assignment whose right side also reads
					   it the order of those accesses is not something to rely
					   on. */
					nStart      = recRdChunks;
					recRdChunks = nStart + RecorderDrainChunks(aSink,
					                                           activeFileCount,
					                                           nStart,
					                                           nAvail);
				}

				LoopSpool_SdUnlock();
			}

            break;

        // =========================================================
        case 2: // STOPPING
        // =========================================================
        	if (isFileOpen)
			{
        		/* Flush the tail: everything the MDMA has completed and the
        		   writer has not taken yet, however little that is. There is no
        		   partial-chunk case to handle - the de-interleave moves a whole
        		   chunk or none, so the finest granularity in the ring is one
        		   chunk, and up to 21.3 ms can be lost at the very end.

        		   This is the whole point of chasing the pointer. Under the old
        		   half-buffer scheme the tail was whatever had accumulated since
        		   the last half filled, which at a 50 MiB ring would have been up
        		   to 34 seconds of audio to write in one go on the stop key. */
        		const uint32_t nTail  = RecorderClaimChunks();
        		const uint32_t nStart = recRdChunks;

        		/* Held across the tail AND the header rewrites: a close is a
        		   seek back to zero and a short write, which is the worst
        		   possible thing to interleave with a large sequential one. */
        		(void)LoopSpool_SdLock(osWaitForever);

        		recRdChunks = nStart + RecorderDrainChunks(aSink,
        		                                           activeFileCount,
        		                                           nStart,
        		                                           nTail);

				for(int i = 0; i < activeFileCount; i++)
				{
					// Update header sizes and close
					headers[i].subchunk2Size = aSink[i].nDataBytes;
					headers[i].chunkSize = 36 + aSink[i].nDataBytes;
					f_lseek(&wavFiles[i], 0);
					f_write(&wavFiles[i], &headers[i], sizeof(WAV_Header), &bytesWritten);
					f_close(&wavFiles[i]);

					/* NO f_truncate here. It cuts the file at the CURRENT position,
					   which after rewriting the header is 44 - the header and nothing
					   else. It did no damage only because it ran on an already closed
					   file and FatFs rejected it; reorder those two lines and every
					   recording on the card becomes 44 bytes. There is nothing to
					   truncate anyway - the file is exactly as long as what was
					   written to it. */
				}
				LoopSpool_SdUnlock();

				isFileOpen      = FALSE;
				activeFileCount = 0;
				recIsWriting    = 0U;
			}

            state = 0;
            break;

        // =========================================================
        default:

        	PUBSUB_Update(xRecorderHandle,
						  &recInfo,
						  sizeof(RecorderInfo_t),
						  10);
            break;
        }
    }
}

/**
 * @brief Write S32 samples to a file as packed 24-bit, little endian.
 *
 * The de-interleaved rings hold S32 with the sample already sign extended in
 * the low 24 bits by the audio controller, so the three bytes a 24-bit WAV
 * wants ARE bits 0..23 - no shifting, no rounding, no dither decision. What
 * the change does cost is a copy: f_write can no longer be pointed straight at
 * SDRAM, because three-byte samples are not what is stored there.
 *
 * The staging buffer is static, not automatic - the recorder thread has an
 * 8 KiB stack and this is 6 KiB of it. It is ordinary cacheable memory on
 * purpose: sd_diskio.c cleans the caller's buffer before the SD DMA reads it
 * (ENABLE_SD_DMA_CACHE_MAINTENANCE), so placement is handled. 32-byte aligned
 * so that clean covers whole cache lines and the driver takes its aligned
 * fast path rather than bouncing through its own scratch block.
 *
 * @return bytes actually written, which is short of the request when the card
 *         is full - the caller adds it to the running total either way, so a
 *         truncated recording still gets a correct header
 */
/**
 * @brief Write one channel, or an interleaved pair, as packed 24-bit LE.
 *
 * The planes hold S32 with the sample already sign extended in the low 24
 * bits by the audio controller, so the three bytes a 24-bit WAV wants ARE
 * bits 0..23 - no shifting, no rounding, no dither decision.
 *
 * pSrcR is what stereo costs now that the rings are planar: a second
 * sequential read, interleaved into the staging buffer as L,R per frame,
 * which is WAV frame order. Pass NULL_PTR for a mono file.
 *
 * The staging buffer is static, not automatic - the recorder thread has an
 * 8 KiB stack and this is 6 KiB of it - and it is ordinary cacheable memory
 * on purpose, because sd_diskio.c cleans the caller's buffer before the SD
 * DMA reads it (ENABLE_SD_DMA_CACHE_MAINTENANCE). 32-byte aligned so that
 * clean covers whole cache lines and the driver takes its aligned fast path.
 *
 * @param nFrames  frames, NOT samples - a stereo call writes twice as many
 *                 samples as a mono call with the same nFrames
 *
 * @return bytes actually written, short of the request when the card is
 *         full - the caller adds it to the running total either way, so a
 *         truncated recording still gets a correct header
 */
static uint32_t RecorderWritePacked24(FIL* const pFile,
                                      const int32_t* const pSrcL,
                                      const int32_t* const pSrcR,
                                      const uint32_t nFrames)
{
    static uint8_t aPack[REC_PACK_BYTES] __attribute__((aligned(32)));

    const uint32_t nChans = (pSrcR != NULL_PTR) ? 2UL : 1UL;

    /* Whole frames per batch, so a stereo pair is never split across two
       writes - which would put an L sample at an odd frame boundary. */
    const uint32_t nBatchFrames = (uint32_t)REC_PACK_SAMPLES / nChans;

    uint32_t nWrittenTotal = 0UL;
    uint32_t nDone         = 0UL;

    if ((pFile == NULL_PTR) || (pSrcL == NULL_PTR) || (nBatchFrames == 0UL))
    {
        return 0UL;
    }

    while (nDone < nFrames)
    {
        uint32_t nBatch = nFrames - nDone;
        uint32_t nBytes;
        uint32_t i;
        UINT     nWritten = 0U;

        if (nBatch > nBatchFrames)
        {
            nBatch = nBatchFrames;
        }

        for (i = 0UL; i < nBatch; i++)
        {
            uint8_t* pOut = &aPack[i * nChans * REC_WAV_BYTES_PER_SAMPLE];
            uint32_t nSample = (uint32_t)pSrcL[nDone + i];

            pOut[0] = (uint8_t)(nSample & 0xFFUL);
            pOut[1] = (uint8_t)((nSample >> 8U) & 0xFFUL);
            pOut[2] = (uint8_t)((nSample >> 16U) & 0xFFUL);

            if (pSrcR != NULL_PTR)
            {
                pOut += REC_WAV_BYTES_PER_SAMPLE;
                nSample = (uint32_t)pSrcR[nDone + i];

                pOut[0] = (uint8_t)(nSample & 0xFFUL);
                pOut[1] = (uint8_t)((nSample >> 8U) & 0xFFUL);
                pOut[2] = (uint8_t)((nSample >> 16U) & 0xFFUL);
            }
        }

        nBytes = nBatch * nChans * REC_WAV_BYTES_PER_SAMPLE;

        if (FR_OK != f_write(pFile, aPack, (UINT)nBytes, &nWritten))
        {
            break;
        }

        nWrittenTotal += (uint32_t)nWritten;
        nDone         += nBatch;

        /* Short write: the card is full. Stop instead of spinning on it. */
        if (nWritten != (UINT)nBytes)
        {
            break;
        }
    }

    return nWrittenTotal;
}


STD_RESULT Recorder_ArmLoopDest(const uint32_t nBase, const uint32_t nBytes)
{
    /* Word aligned, because the MDMA writes 32-bit slots and an unaligned
       destination would fault rather than merely be slow. */
    if ((nBase == 0UL) || (nBytes == 0UL) || ((nBase & 3UL) != 0UL))
    {
        return RESULT_INVALID_PARAM_1;
    }

    nLoopDstBase  = nBase;
    nLoopDstOfs   = 0UL;
    nLoopDstEnd   = nBytes;
    bLoopDstArmed = 1U;

    return RESULT_OK;
}


void Recorder_DisarmLoopDest(void)
{
    /* Cleared BEFORE the stream narrows, not after: while this is armed the
       de-interleave will happily route slots that the audio board has already
       stopped sending, writing whatever the wire leaves in them. */
    bLoopDstArmed = 0U;
    nLoopDstOfs   = 0UL;
}


uint32_t Recorder_LoopBytesTaken(void)
{
    return nLoopDstOfs;
}


/**
 * @brief Next destination for the loop route, and advance past it.
 *
 * Called from the de-interleave with interrupts already in the right state.
 * Returns NOT_OK when nothing is armed, or when the destination is full - in
 * which case the loop route is simply omitted from this block rather than
 * writing past the end of the staging area.
 */
static STD_RESULT RecorderLoopDest(uint32_t* const pnAddr, const uint8_t nSlotQty)
{
    /*
     * One block of loop slots: nSlotQty words per frame, over the half SPI
     * buffer the de-interleave moves.
     *
     * The slot count has to come in rather than be assumed. It is negotiated
     * per session, so a fixed step here would advance the destination at the
     * wrong rate for every width but one - and the symptom would be a loop file
     * with silence woven through it rather than an obvious failure.
     */
    const uint32_t nStep = (uint32_t)nSlotQty
                         * (uint32_t)REC_RX_HALF_FRAMES
                         * sizeof(int32_t);

    if ((bLoopDstArmed == 0U) || (pnAddr == NULL_PTR) || (nSlotQty == 0U))
    {
        return RESULT_NOT_OK;
    }

    if ((nLoopDstOfs + nStep) > nLoopDstEnd)
    {
        /* The session said fewer bytes than the stream is delivering. Dropping
           the route is the safe half of that disagreement; the count is how
           anyone finds out it happened. */
        recLoopOverruns++;
        return RESULT_NOT_OK;
    }

    *pnAddr = nLoopDstBase + nLoopDstOfs;

    nLoopDstOfs += nStep;

    return RESULT_OK;
}


uint32_t Recorder_BacklogChunks(void)
{
    /* Both are volatile and each has exactly one writer, so this is a reading
       of the gap at some instant - which is all any caller can act on anyway. */
    return recWrChunks - recRdChunks;
}


BOOLEAN Recorder_IsCaughtUp(void)
{
    /* An idle recorder is caught up by definition: nothing is consuming the
       ring, so the gap is meaningless rather than large. */
    if (recIsWriting == 0U)
    {
        return TRUE;
    }

    /*
     * Not "zero". In steady recording the backlog swings between 0 and
     * REC_WRITE_CHUNKS because that is the threshold the writer wakes on, so
     * demanding zero would be a condition that is only briefly true and would
     * hold a "saving" indicator up more or less permanently. Caught up means
     * there is nothing the writer would currently act on.
     */
    return (Recorder_BacklogChunks() < (uint32_t)REC_WRITE_CHUNKS) ? TRUE : FALSE;
}


/**
 * @brief How many chunks are waiting, after dealing with any overrun.
 *
 * Overrun is handled HERE rather than in the MDMA callback so that exactly one
 * piece of code reasons about it, and so that recRdChunks keeps a single
 * owner - the callback increments recWrChunks and touches nothing else.
 *
 * When the difference exceeds the ring, the MDMA has lapped the writer and the
 * oldest chunks have already been overwritten by newer audio. Writing them
 * anyway would splice a jump into the file at the point of the overrun and
 * report nothing. Skipping to the oldest chunk that is still intact loses the
 * same audio but keeps everything that reaches the card contiguous, and leaves
 * a count behind saying how much went missing.
 *
 * @return chunks available to write, never more than REC_CHUNKS
 */
static uint32_t RecorderClaimChunks(void)
{
    /* One read each. recWrChunks can advance while this runs - that only makes
       the answer conservative, and the next pass picks up the remainder. */
    const uint32_t nWr    = recWrChunks;
    uint32_t       nAvail = nWr - recRdChunks;

    if (nAvail > (uint32_t)REC_CHUNKS)
    {
        recOverruns += nAvail - (uint32_t)REC_CHUNKS;
        recRdChunks  = nWr - (uint32_t)REC_CHUNKS;
        nAvail       = (uint32_t)REC_CHUNKS;
    }

    return nAvail;
}

/**
 * @brief Write a run of ring chunks to every open file.
 *
 * Splits at the end of the ring so that no single call to the packer ever
 * straddles the wrap - each f_write then sees a contiguous run of samples in
 * every plane, which is what lets the pack loop stay a plain indexed read.
 *
 * @param nFirstChunk  free-running counter value, NOT a ring index; the
 *                     modulo happens here so callers never hold a wrapped
 *                     position that could go stale
 *
 * @return chunks consumed, which is nChunkQty even when the card refused the
 *         data - a full card stops the recording, it does not stall the ring
 */
static uint32_t RecorderDrainChunks(REC_SINK* const aSink,
                                    const uint8_t  nSinkQty,
                                    const uint32_t nFirstChunk,
                                    const uint32_t nChunkQty)
{
    uint32_t nDone = 0UL;

    if ((aSink == NULL_PTR) || (nSinkQty == 0U))
    {
        return 0UL;
    }

    while (nDone < nChunkQty)
    {
        const uint32_t nPos = (nFirstChunk + nDone) % (uint32_t)REC_CHUNKS;

        /* Up to the wrap, or the rest of the request - whichever ends first. */
        uint32_t nRun = (uint32_t)REC_CHUNKS - nPos;
        uint32_t nOffs;
        uint32_t nFrames;
        uint8_t  i;

        if (nRun > (nChunkQty - nDone))
        {
            nRun = nChunkQty - nDone;
        }

        nOffs   = nPos * (uint32_t)REC_CHUNK_SAMPLES;
        nFrames = nRun * (uint32_t)REC_CHUNK_SAMPLES;

        for (i = 0U; i < nSinkQty; i++)
        {
            /* Every plane is the same length, so the offset is the same for a
               mono and a stereo file - the second plane is what makes it
               stereo, not a different stride. */
            aSink[i].nDataBytes += RecorderWritePacked24(
                aSink[i].pFile,
                &recorder[aSink[i].aPlane[0]][nOffs],
                (aSink[i].nPlaneQty == 2U)
                    ? &recorder[aSink[i].aPlane[1]][nOffs]
                    : NULL_PTR,
                nFrames);
        }

        nDone += nRun;
    }

    return nDone;
}

//--------------------------------------------------------------------------------------------------

void MDMA_Trigger_Deinterleave()
{
    /* Four nodes plus the channel = five routes: one per recorder plane, and
       one for the loop transport's contiguous slot run. */
    static MDMA_LinkNodeTypeDef* const apNodes[4] =
    {
        &node_mdma_channel1_sw_1,
        &node_mdma_channel1_sw_2,
        &node_mdma_channel1_sw_3,
        &node_mdma_channel1_sw_4
    };

    MDMA_Channel_TypeDef* const ch = hmdma_mdma_channel1_sw_0.Instance;

    /* One route per destination buffer. All REC_SLOTS_PER_FRAME slots are always
       covered, so there are between two (both pairs stereo) and four (all mono)
       of them - which is exactly the channel plus up to three nodes. */
    /* One extra for the loop route. */
    REC_ROUTE  aRoute[REC_SLOTS_PER_FRAME + 1U];
    FX_IL_XFER aXfer[REC_SLOTS_PER_FRAME + 1U];
    PROTO_ACK  tAck;
    uint8_t    nRoutes = 0U;
    uint8_t    nStreamWidth;
    uint32_t   nSrcBase;
    uint32_t   nDstOffs;
    uint8_t    i;

    /*
     * The stream width comes from the audio controller's ACK rather than from a
     * constant here, because the ACK reports what it actually committed to.
     * Until one has arrived the compiled-in default is all there is to go on -
     * and nothing is recorded before then anyway.
     */
    nStreamWidth = (CtrlLinkIf_GetAck(&tAck) == RESULT_OK)
                       ? tAck.nStreamWidth
                       : (uint8_t)REC_SLOTS_PER_FRAME;

    /* Source: the half the SPI DMA is NOT filling. */
    nSrcBase = (uint32_t)audioRxBuffer;

    if (halfBufferFlag)
    {
        nSrcBase += (uint32_t)REC_RX_HALF_FRAMES * REC_BYTES_PER_FRAME;
    }

    /*
     * Destination: one chunk along the per-channel ring.
     *
     * recWrChunks counts COMPLETED chunks, so it names the one about to be
     * filled - and it is advanced in the completion callback, not here. That
     * ordering matters: if the routing check below refuses this transfer, or
     * the MDMA errors, the counter does not move and the next trigger reuses
     * the same slot instead of leaving an unwritten hole for the writer to
     * emit as audio.
     */
    nDstOffs = (recWrChunks % (uint32_t)REC_CHUNKS)
             * (uint32_t)REC_CHUNK_SAMPLES * sizeof(int32_t);

    /* ---- routing ---------------------------------------------------------- */
    /*
     * One slot to one plane, every time. Nothing here consults recInfo any
     * more: with planar rings there is no stereo destination, so mono and
     * stereo differ only in how the WRITER reads these planes. That removed
     * four branches and, with them, the possibility of a stereo destination
     * offset being doubled in one place and not the other.
     */
    for (nRoutes = 0U; nRoutes < (uint8_t)REC_SLOTS_PER_FRAME; nRoutes++)
    {
        aRoute[nRoutes].nSlot    = nRoutes;
        aRoute[nRoutes].nWidth   = 1U;
        aRoute[nRoutes].nDstAddr = (uint32_t)&recorder[nRoutes][0] + nDstOffs;
    }

    /*
     * ---- the loop route, when a transfer is running ----------------------
     *
     * ONE route, not one per slot: the loop slots are contiguous on the wire,
     * so FxInterleave_Xfer can lift the whole run in a single transfer. That is
     * the only reason this fits - four planes plus one loop run is five routes,
     * exactly the channel plus four nodes.
     *
     * nStreamWidth already came from the ACK above, so it is the width the
     * audio board actually committed to rather than what this side hopes for.
     * A stale width would put the loop route over the recorder's slots, which
     * is why it is not recomputed here.
     */
    {
        uint32_t nLoopDst  = 0UL;
        uint8_t  nLoopQty  = 0U;

        nLoopQty = (nStreamWidth > (uint8_t)REC_SLOTS_PER_FRAME)
                       ? (uint8_t)(nStreamWidth - (uint8_t)REC_SLOTS_PER_FRAME)
                       : 0U;

        if ((nLoopQty > 0U) && (RecorderLoopDest(&nLoopDst, nLoopQty) == RESULT_OK))
        {
            aRoute[nRoutes].nSlot    = (uint8_t)REC_SLOTS_PER_FRAME;
            aRoute[nRoutes].nWidth   = nLoopQty;
            aRoute[nRoutes].nDstAddr = nLoopDst;

            nRoutes++;
        }
    }

    /* ---- geometry, checked in full before any register is touched --------- */
    for (i = 0U; i < nRoutes; i++)
    {
        if (FxInterleave_Xfer(&aXfer[i], aRoute[i].nSlot, aRoute[i].nWidth,
                              nStreamWidth, (uint32_t)REC_RX_HALF_FRAMES) != RESULT_OK)
        {
            /* The ACKed layout cannot hold this routing. Programming it anyway
               would record real audio into the wrong files and report nothing,
               so transfer NOTHING and leave the count for a human to find. */
            recDeinterleaveRefused++;
            return;
        }
    }

    /* ---- programme the nodes: routes 1..n-1 ------------------------------- */
    for (i = 1U; i < nRoutes; i++)
    {
        MDMA_LinkNodeTypeDef* const pNode = apNodes[i - 1U];

        pNode->CSAR = nSrcBase + aXfer[i].nSrcOffsetBytes;
        pNode->CDAR = aRoute[i].nDstAddr;
        pNode->CBNDTR =
            (((aXfer[i].nBeats - 1UL) << MDMA_CBNDTR_BRC_Pos) & MDMA_CBNDTR_BRC)
            | (aXfer[i].nBytesPerBeat & MDMA_CBNDTR_BNDT);
        pNode->CBRUR = aXfer[i].nSrcSkipBytes;
        pNode->CLAR = (i < (uint8_t)(nRoutes - 1U)) ? (uint32_t)apNodes[i] : 0UL;
    }

    /*
     * CBRUR is the one register HAL_MDMA_Start_IT leaves alone. MDMA_SetConfig
     * writes CBNDTR, CSAR, CDAR, CTBR and CLAR - but not the block-repeat
     * address update - so the channel's source skip is written here and
     * survives the call below. CLAR needs nothing: SetConfig points it at
     * FirstLinkedListNodeAddress, which is node 1, and there are always at
     * least two routes so node 1 is always the next one.
     */
    ch->CBRUR = aXfer[0].nSrcSkipBytes;

    /*
     * CACHE & FIRE: push the nodes to RAM, then trigger.
     *
     * The MDMA engine fetches the nodes from RAM and the loop above has just
     * rewritten them with the CPU. They live in a non-cacheable section, so
     * this costs a few cycles and does nothing - it is here because mdma.c is
     * CubeMX generated: regenerate the project and that section attribute
     * vanishes, and without this the engine would quietly read stale
     * descriptors. Both buffers are already non-cacheable, so only the
     * descriptors need it.
     */
    SCB_CleanDCache_by_Addr((uint32_t*)(void*)&node_mdma_channel1_sw_1,
                            sizeof(node_mdma_channel1_sw_1));
    SCB_CleanDCache_by_Addr((uint32_t*)(void*)&node_mdma_channel1_sw_2,
                            sizeof(node_mdma_channel1_sw_2));
    SCB_CleanDCache_by_Addr((uint32_t*)(void*)&node_mdma_channel1_sw_3,
                            sizeof(node_mdma_channel1_sw_3));

    /* BlockDataLength is BYTES per block and BlockCount is the number of them,
       so this is "move nBytesPerBeat bytes, nBeats times, stepping the source
       by CBRUR in between". The old formula derived the count from the buffer
       size and got 2048 for a stereo pair - twice a half-buffer - which ran the
       de-interleave into the half the SPI DMA was still filling. */
    (void)HAL_MDMA_Start_IT(&hmdma_mdma_channel1_sw_0,
                            nSrcBase + aXfer[0].nSrcOffsetBytes,
                            aRoute[0].nDstAddr,
                            aXfer[0].nBytesPerBeat,
                            aXfer[0].nBeats);
}

/**
 * @brief One half of the SPI receive ring has filled. Registered with SPI_TP.
 *
 * Was two separate HAL callbacks. The transport owns those weak symbols now and
 * calls this with which half completed, so the only thing that changed here is
 * that the two bodies collapsed into one - they always differed by exactly this
 * flag.
 *
 * bSecondHalf FALSE means the FIRST half just filled, so the de-interleave
 * takes it while the DMA moves into the second. halfBufferFlag carries that to
 * MDMA_Trigger_Deinterleave, which uses it to pick the source half.
 */
static void Recorder_OnSpiHalf(const BOOLEAN bSecondHalf)
{
    halfBufferFlag = (bSecondHalf == TRUE) ? 1U : 0U;

    MDMA_Trigger_Deinterleave();

    /*
     * Refill the transmit half the master has just finished clocking, while it
     * clocks the other. Filling the half being clocked would put a partly
     * written frame on the wire.
     *
     * The stream width comes from the same ACK the de-interleave used, so the
     * two directions cannot disagree about where a frame ends.
     */
    {
        PROTO_ACK tAck;
        const uint8_t nWidth = (CtrlLinkIf_GetAck(&tAck) == RESULT_OK)
                                   ? tAck.nStreamWidth
                                   : (uint8_t)REC_SLOTS_PER_FRAME;

        if (nWidth > (uint8_t)REC_SLOTS_PER_FRAME)
        {
            const uint32_t nOfs = (bSecondHalf == TRUE)
                                      ? 0UL
                                      : ((uint32_t)REC_RX_HALF_FRAMES * nWidth);

            LoopSession_FillTx(&audioTxBuffer[nOfs],
                               (uint32_t)REC_RX_HALF_FRAMES, nWidth);
        }
    }
}

static void HAL_MDMA_XferCpltCallback(MDMA_HandleTypeDef *hmdma) {
    if (hmdma == &hmdma_mdma_channel1_sw_0) {
        /* One chunk of every plane is now in the ring. This is the ONLY place
           recWrChunks moves, which is what lets the writer read it without a
           lock. */
        recWrChunks++;

        /* Wake the writer once there is a batch worth a pass at the card. The
           semaphore is binary, so releases while the writer is already running
           collapse into one - which is harmless, because the writer drains
           everything available before going back to sleep rather than assuming
           one release means one batch. */
        if ((recWrChunks - recRdChunks) >= (uint32_t)REC_WRITE_CHUNKS)
        {
            osSemaphoreRelease(sem_RecorderData);
        }
    }
}

static void HAL_MDMA_XferErrorCallback(MDMA_HandleTypeDef *hmdma)
{
    if (hmdma == &hmdma_mdma_channel1_sw_0)
    {
        // Handle error: abort the MDMA and still release the semaphore
        HAL_MDMA_Abort(hmdma);
    }
}

