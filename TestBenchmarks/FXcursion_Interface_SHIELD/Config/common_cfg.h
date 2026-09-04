/**
 * @file      common_cfg.h
 *
 * @details   Common configuration parameters
 *
 * @version   1.0.0
 *
 * @authors   Predtechenskii Dmitrii (predtech4@yandex.ru)
 *
 * \date      12.08.2025 - First release
 *
 */



#ifndef COMMON_CFG_H
#define COMMON_CFG_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

// Get general definitions
#include "general.h"

/* The wire frame layout is shared with the audio controller, so it comes from
 * the protocol library rather than being restated here. Everything in the
 * recorder geometry below is derived from it. */
#include "fx_frame.h"

/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define PUBSUB_TOPIC_UI		(char* const)"ui_survey"
#define PUBSUB_TOPIC_REC	(char* const)"recorder_cfg"

/*
 * Topics carrying the audio controller's side of the control link. Published by
 * the CtrlLink task, consumed in Model::tick.
 *
 * ACK is the one with a hard requirement attached: it carries the recorder slot
 * map the audio side committed to, and the interface must reprogram its MDMA
 * de-interleave from it BEFORE asking for the stream. See fx_protocol.h.
 */
#define PUBSUB_TOPIC_TELEMETRY	(char* const)"fx_telemetry"
#define PUBSUB_TOPIC_ACK	(char* const)"fx_ack"
#define PUBSUB_TOPIC_DIAG	(char* const)"fx_diag"

/*
 * Buffers that a bus master reads or writes go here: RAM_D2, mapped
 * non-cacheable by MPU region 0. Required now that the D-cache is on -
 * see MPU_Config in Init.c.
 *
 * 32-byte aligned, which is the cache line size: that keeps a buffer from
 * sharing a line with anything else, and is what the CMSIS maintenance
 * functions require should one ever be used on it.
 *
 * The section is NOLOAD, so declare and then INITIALISE - nothing placed
 * here is zeroed at reset.
 */
#define IN_DMA_BUF  __attribute__((section(".dma_buffers"), used, aligned(32)))

#define SAMPLE_RATE 48000
#define MAX_RECORD_FILES 4

/*
 * RECORDER STREAM GEOMETRY
 *
 * Every name here says what it counts. The two it replaces did not:
 * AUDIO_RX_BUF_SIZE was in uint16 units, and RECORD_BUF_SIZE was used both
 * as a sample count (array dimension) and as a byte count (the f_write
 * length). Those two readings agreed only while a sample was 2 bytes and the
 * write was half the ring - a coincidence that does not survive S32.
 *
 * The wire format is fixed by the audio controller and by fx_frame.h: a sync
 * slot, REC_SLOTS_PER_FRAME recorder planes, then the loop slots. One S32 per
 * slot carrying a 24-bit value. See fx_interleave.h for the strides.
 */
#define REC_SLOTS_PER_FRAME     (FX_FRAME_REC_SLOT_QTY)
#define REC_BYTES_PER_SLOT      (FX_FRAME_BYTES_PER_SLOT)

/**
 * Bytes per WIRE frame - all 32 slots, not just the recorder's four.
 *
 * This used to be REC_SLOTS_PER_FRAME * 4 = 16, which was the whole frame back
 * when the recorder was the only thing on the link. It is 128 now, and the
 * distinction matters: the recorder's share is REC_SLOTS_PER_FRAME slots, the
 * STRIDE between one plane's samples is the full frame.
 */
#define REC_BYTES_PER_FRAME     (FX_FRAME_BYTES)

/* S32 words in the whole SPI receive ring - what the DMA counts, since SPI1 is
   SPI_DATASIZE_32BIT. 8192 words is 32 KiB. */
#define REC_RX_WORDS            (8192U)

/**
 * Words in one half of the ring. The half-transfer interrupt fires here.
 *
 * THIS IS THE FIXED QUANTITY the frame has to divide. 4096 / 32 = 128 frames,
 * exactly, which is why the frame is 32 slots wide and not 20.
 */
#define REC_RX_HALF_WORDS       (REC_RX_WORDS / 2U)

/**
 * Frames in one half of the ring. A CONSTANT again.
 *
 * This was REC_FRAMES_PER_HALF(w), a division by a width that arrived in an
 * ACK, because the frame widened for the life of a loop transfer. The frame is
 * fixed now, so the compiler knows this number - and the bug where the
 * de-interleave was told 1024 frames of a half holding 128 cannot be written.
 */
#define REC_FRAMES_PER_HALF     (REC_RX_HALF_WORDS / FX_FRAME_SLOT_QTY)

/* Kept as the same number under its old name: the ring holds this many frames
   in a half, and now nothing can change that at run time. */
#define REC_RX_HALF_FRAMES      (REC_FRAMES_PER_HALF)
#define REC_RX_FRAMES           (REC_RX_WORDS / FX_FRAME_SLOT_QTY)

/* One de-interleave moves half the SPI ring, so a per-channel ring of
   REC_CHUNKS of them wraps after REC_CHUNKS half-buffers. One chunk is
   128 samples, 2.67 ms of audio. */
#define REC_CHUNK_SAMPLES       (REC_FRAMES_PER_HALF)

/*
 * Ring depth, and therefore how long the card may stall before audio is lost.
 * Sized for five seconds, rounded up to the power of two that lands the ring
 * exactly on the 4 MiB SDRAM_REC region:
 *
 *   2048 * 128 samples / 48000       = 5.46 s
 *   2048 * 128 * 4 planes * 4 B      = 4194304 B = 4 MiB
 *
 * 2048, not the old 256, because a chunk is one half-buffer and the frame is
 * now 32 slots wide instead of 4 - so a half holds 128 frames rather than
 * 1024. The RING IS THE SAME 4 MiB and still absorbs the same 5.46 s; only the
 * granularity changed. Get this wrong and the absorption silently becomes
 * 0.68 s, which is the sort of thing that only shows up as a dropout when a
 * card stalls in the field.
 *
 * A power of two also turns the ring wrap in RecorderDrainChunks and
 * MDMA_Trigger_Deinterleave into a mask rather than a division - the latter
 * runs in interrupt context.
 *
 * Changing this moves the region: SDRAM_REC in the linker script has to match,
 * and ld says so if it does not.
 */
#define REC_CHUNKS              (2048U)

/* Samples per mono channel in the SDRAM ring. */
#define RECORD_BUF_SAMPLES      (REC_CHUNKS * REC_CHUNK_SAMPLES)

/*
 * RING SIZE IS NOT WRITE GRANULARITY.
 *
 * The writer used to ping-pong on halves of the ring: it slept until one half
 * was full, then wrote all of it. That tied two unrelated things together.
 * REC_CHUNKS sets how long a card stall can be before audio is lost, and it
 * wants to be as large as the memory allows. The write batch sets how much
 * work one pass at the card does, and it wants to be a few hundred
 * milliseconds - large enough that f_write is not called constantly, small
 * enough that stopping a recording does not strand a huge tail.
 *
 * Coupled, growing the ring to 50 MiB for stall absorption would have made the
 * batch 34 SECONDS - the writer emitting 25 MiB bursts, and up to 34 s of
 * audio needing a partial flush on stop.
 *
 * The writer now chases the MDMA's chunk counter instead. REC_WRITE_CHUNKS is
 * only the threshold at which it is worth waking; once awake it drains
 * everything available, so a slow card is caught up on rather than dropped.
 *
 *   128 chunks * 128 samples / 48000 = 341 ms per wake
 *   mono file   16384 samples * 3 B  = 49152 B per pass
 *   stereo file                      = 98304 B per pass
 *
 * 128, not the old 16, for the same reason REC_CHUNKS moved: a chunk is eight
 * times smaller now. Left at 16 the writer would have woken every 42 ms with
 * 2048-sample batches - eight times the f_write calls for the same audio,
 * which is exactly the coupling the comment above says to avoid.
 *
 * REC_CHUNKS can be raised on its own, and is the only thing that needs
 * changing to grow the ring.
 */
#define REC_WRITE_CHUNKS        (128U)

/* 24-bit on the card: the codec is 24-bit and so is the wire, and a take a
   musician will gain-stage in a DAW should not be truncated to 16 on the way
   past. Three bytes per sample, packed little-endian from the low 24 bits of
   each S32 - which is why the write path cannot hand SDRAM straight to
   f_write any more. */
#define REC_WAV_BITS            (24U)
#define REC_WAV_BYTES_PER_SAMPLE (3U)

/* Staging chunk for that packing. 1024 frames x 2 channels x 3 bytes is the
   worst case, so this covers a stereo file in one pass per chunk. */
#define REC_PACK_SAMPLES        (2048U)
#define REC_PACK_BYTES          (REC_PACK_SAMPLES * REC_WAV_BYTES_PER_SAMPLE)

/***************************************************************************************************
* Declarations of global (public) data types
***************************************************************************************************/

typedef enum enUIObjectType
{
	BTN_YES		= 0,
	BTN_NO		= 1,
	BTN_UP		= 2,
	BTN_DOWN	= 3,
	BTN_FOOT	= 4,
	BTN_FUNC	= 5,
	BTN_PARAM	= 6,
	BTN_PLAY	= 7,
	BTN_STOP	= 8,
	BTN_REC		= 9,
	BTN_MENU	= 10,

	ENC_MENU	= 11,
	ENC_PARAM	= 12,

} UIObjectType;

typedef struct stUIObjectInfo
{
	UIObjectType eName;
	U8 nID;
	S8 nValue;

} UIObjectInfo_t;

typedef struct stRecorderInfo
{
	BOOLEAN stereo1;
	BOOLEAN stereo2;
	BOOLEAN mono[4];

} RecorderInfo_t;


typedef struct {
    char     chunkID[4];
    uint32_t chunkSize;
    char     format[4];
    char     subchunk1ID[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char     subchunk2ID[4];
    uint32_t subchunk2Size;
} WAV_Header;

/***************************************************************************************************
* Declarations of global (public) variables
***************************************************************************************************/

/// None.

#endif  // #ifndef COMMON_CFG_H

