/**
 * @file      mem_map.h
 *
 * @details   Where every byte of audio state lives, and the compile-time proof
 *            that it fits.
 *
 *            ------------------------------------------------------------------
 *            WHY THERE IS NO ALLOCATOR
 *            ------------------------------------------------------------------
 *
 *            Two constraints from the GUI model make dynamic allocation
 *            unnecessary:
 *
 *              1. Chain widths always sum to AUDIO_CH_QTY, so the plane count is
 *                 a compile-time constant in every topology.
 *              2. At most one instance of each effect TYPE exists per chain.
 *
 *            Together these mean memory is reserved per (plane, effect type)
 *            rather than per instance. Every effect declares one static array
 *            indexed by plane, in its own .c file, using the section macros
 *            below. Nothing is allocated, nothing can fail, nothing can
 *            fragment, and no configuration the GUI permits can ever be
 *            rejected for lack of memory.
 *
 *            ------------------------------------------------------------------
 *            PHYSICAL MEMORY ON THIS BOARD
 *            ------------------------------------------------------------------
 *
 *            DTCM        128 KiB   0x20000000  zero wait state, never cached
 *            ITCM         64 KiB   0x00000000  zero wait state, code only
 *            AXI SRAM    512 KiB   0x24000000  D1, cacheable
 *            SRAM1/2/3   288 KiB   0x30000000  D2, the audio DMA buffers
 *            SRAM4        64 KiB   0x38000000  D3
 *            SDRAM bank1  16 MiB   0xC0000000  W9812G6KH, 16-bit, SDCLK 120 MHz
 *            SDRAM bank2  16 MiB   0xD0000000  W9812G6KH
 *
 *            There is no QSPI PSRAM. Loop audio used to live there; it is now
 *            11 MiB of each SDRAM bank - see loop_mem.h.
 *
 *            ------------------------------------------------------------------
 *            THE TIERING, AND WHY
 *            ------------------------------------------------------------------
 *
 *            DTCM       scratch, parameters, meters, the loop staging windows,
 *                       and the HOT state of every effect - filter memory, LFO
 *                       phase, allpass state. Small and touched constantly.
 *                       Zero wait state and never cached, so no cache
 *                       maintenance is ever required, which is also why the
 *                       loop windows have to live here and not in AXI SRAM.
 *
 *            AXI SRAM   the SHORT modulation lines only: chorus, flanger,
 *                       vibrato. ~6 KiB each per plane.
 *
 *            SDRAM 1    long delay lines.
 *            SDRAM 2    reverb.
 *
 *                       These two are split across separate chip selects on
 *                       purpose. Delay and reverb have completely different
 *                       access patterns; sharing a bank would make them thrash
 *                       each other's open rows.
 *
 *                       The cost of putting them in SDRAM at all is about 10%
 *                       of the block budget in cache linefills, worst case
 *                       (every effect on every plane: ~84 active regions x 8
 *                       lines x ~200 ns = 134 us of 1333 us). That is the right
 *                       trade: a reverb that fits in internal SRAM is a small
 *                       reverb, and a good one is worth 10%.
 *
 *            SDRAM 1+2  ALSO one looper each, 11 MiB per bank: planes 0,1 in
 *                       bank 1 and planes 2,3 in bank 2, each held twice for
 *                       undo. One looper per bank rather than both in one, for
 *                       the same reason delay and reverb are apart - the two
 *                       are played independently and would evict each other's
 *                       open rows.
 *
 *                       The loopers do not stream from SDRAM per sample. A
 *                       one-block window per plane is copied into DTCM and back
 *                       - see loop_mem.h - so the SDRAM access is one
 *                       sequential burst in and one out per block rather than
 *                       a row activate per sample.
 *
 *            MPU: map both SDRAM banks write-through, no write-allocate. Delay
 *            lines are streamed with no reuse, so write-allocate only buys a
 *            read-for-ownership on data that is never read back. That means one
 *            region covering 0xC0000000 for 512 MiB - 256 MiB reaches bank 1
 *            only and leaves bank 2 on the default write-back mapping.
 *
 *            RAM_D2 is mapped non-cacheable, because the audio DMA buffers live
 *            there and the alternative is a clean or invalidate around every
 *            block. That region is 256 KiB - the largest power of two that fits
 *            the MPU's alignment rules - while the device has 288 KiB, so the
 *            linker script deliberately declares RAM_D2 as 256 KiB too. Anything
 *            placed past that would be cacheable and read by DMA, which is a
 *            fault that shows up as occasional clicks rather than a build error;
 *            narrowing the region makes the linker refuse it instead. To use the
 *            last 32 KiB, add a second MPU region for it first.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef MEM_MAP_H
#define MEM_MAP_H



/***************************************************************************************************
* Module includes
***************************************************************************************************/

#include "general.h"
#include "audio_cfg.h"



/***************************************************************************************************
* Section placement macros
***************************************************************************************************/

#ifdef FXC_HOST_BUILD

/* Host test build: everything is ordinary memory. */
#define IN_DTCM
#define IN_ITCM
#define IN_SRAM_FAST
#define IN_SDRAM_DELAY
#define IN_SDRAM_REVERB
#define IN_SDRAM_LOOP_A
#define IN_SDRAM_LOOP_B
#define IN_DMA_BUF
#define MEM_ALIGN(n)

#else

/*
 * NOTE ON THE LINKER SCRIPT
 *
 * These sections must be added to the .ld, and .dtcm in particular must be
 * given real attention:
 *
 *   - If .dtcm is (NOLOAD) it gets neither the .data copy nor the .bss zero
 *     fill, so anything placed there is GARBAGE at reset. In C there are no
 *     constructors to hide it, so every IN_DTCM object below MUST be explicitly
 *     initialised by its module's Init function. AudioSys_Init does this.
 *
 *   - .itcm needs "AT> FLASH" plus a copy loop in Reset_Handler before any code
 *     is placed there, exactly like .data.
 *
 *   - .sdram_delay -> 0xC0000000, .sdram_reverb -> 0xD0000000, both NOLOAD.
 *
 *   - .ramd2dma -> RAM_D2. NOT AXI SRAM and emphatically not DTCM: DMA1 and
 *     DMA2 are masters in domain D2 and cannot address the tightly coupled
 *     memories at all, so an audio buffer placed in DTCM never transfers a
 *     single byte. D2 SRAM also avoids the D2-to-D1 bridge that AXI SRAM
 *     would cost on every beat.
 */
#define IN_DTCM         __attribute__((section(".dtcm"),         used))
#define IN_ITCM         __attribute__((section(".itcm"),         used))
#define IN_SRAM_FAST    __attribute__((section(".ramd1"),        used))
#define IN_SDRAM_DELAY  __attribute__((section(".sdram_delay"),  used))
#define IN_SDRAM_REVERB __attribute__((section(".sdram_reverb"), used))
#define IN_SDRAM_LOOP_A __attribute__((section(".sdram_loop_a"), used))
#define IN_SDRAM_LOOP_B __attribute__((section(".sdram_loop_b"), used))
#define IN_DMA_BUF      __attribute__((section(".ramd2dma"),     used))
#define MEM_ALIGN(n)    __attribute__((aligned(n)))

#endif /* FXC_HOST_BUILD */

/** Cache line size. Align anything DMA touches, and anything streamed. */
#define MEM_CACHE_LINE_BYTES        (32U)



/***************************************************************************************************
* Per-plane reservations
***************************************************************************************************/

/** Long delay line, FLOAT32, SDRAM bank 1. */
#define MEM_DELAY_BYTES_PER_PLANE       (DELAY_MAX_FRAMES * 4UL)

/** Reverb delay memory, FLOAT32, SDRAM bank 2. */
#define MEM_REVERB_BYTES_PER_PLANE      (REVERB_FRAMES_PER_PLANE * 4UL)

/** Modulation delay line, FLOAT32, AXI SRAM. Chorus, flanger and vibrato each. */
#define MEM_MODDELAY_BYTES_PER_PLANE    (MODDELAY_MAX_FRAMES * 4UL)

/**
 * Loop audio, packed 24-bit, SDRAM.
 *
 * Both a take and its pre-overdub snapshot are this size, and both live in the
 * SAME bank as each other - undo has to be a copy within one chip select or it
 * would cross banks on every overdub.
 */
#define MEM_LOOP_BYTES_PER_PLANE        (LOOP_MAX_BYTES)

/**
 * One looper: a stereo pair, held twice.
 *
 *   planes 0,1 + their undo  -> SDRAM bank 1, alongside the delay lines
 *   planes 2,3 + their undo  -> SDRAM bank 2, alongside the reverb
 *
 * One looper per bank rather than both loopers in one is deliberate. The two
 * are played and overdubbed independently, so putting them on separate chip
 * selects means each keeps its own open rows instead of evicting the other's -
 * the same reasoning that already separates delay from reverb.
 */
#define MEM_LOOP_PLANES_PER_LOOPER      (AUDIO_PLANE_QTY / LOOPER_QTY)

#define MEM_LOOP_BYTES_PER_LOOPER                                                           \
            (MEM_LOOP_BYTES_PER_PLANE * MEM_LOOP_PLANES_PER_LOOPER * 2UL)



/***************************************************************************************************
* Budgets
***************************************************************************************************/

#define MEM_SDRAM_BANK_BYTES            (16UL * 1024UL * 1024UL)

/**
 * Budget for short delay lines in internal SRAM.
 *
 * RAM_D1 is 512 KiB and also holds .data, .bss, the heap and the stack - about
 * 6 KiB between them - plus the .ramd1dma staging. 320 KiB is a self-imposed
 * cap that leaves comfortable room for all of that, not a hardware limit.
 */
#define MEM_SRAM_FAST_BUDGET_BYTES      (320UL * 1024UL)

/* --- SDRAM bank 1: time-domain delay lines ---------------------------------------------------- */
#define MEM_SDRAM_DELAY_TOTAL           (MEM_DELAY_BYTES_PER_PLANE * AUDIO_PLANE_QTY)

/* --- SDRAM bank 2: reverberation -------------------------------------------------------------- */
#define MEM_SDRAM_REVERB_TOTAL          (MEM_REVERB_BYTES_PER_PLANE * AUDIO_PLANE_QTY)

/* --- AXI SRAM: three modulation effects ------------------------------------------------------- */
#define MEM_SRAM_FAST_TOTAL             (MEM_MODDELAY_BYTES_PER_PLANE * 3UL * AUDIO_PLANE_QTY)

/* --- The looper reservation inside each bank --------------------------------------------------- */
/**
 * Carved out of both banks and matched by SDRAM_LOOP_A / SDRAM_LOOP_B in the
 * linker script. 11 MiB is not derived from anything here - it is the number
 * the interface controller's staging region was sized against, so the two move
 * together or a loop that fits one side will not fit the other.
 */
#define MEM_SDRAM_LOOP_REGION_BYTES     (11UL * 1024UL * 1024UL)

/**
 * Loop seconds a bank's looper region can actually hold, for the diagnostic in
 * the assertion below. Four plane buffers per region: stereo, held twice.
 */
#define MEM_LOOP_SEC_THAT_FIT                                                               \
            (MEM_SDRAM_LOOP_REGION_BYTES /                                                  \
             (MEM_LOOP_PLANES_PER_LOOPER * 2UL *                                            \
              AUDIO_SAMPLE_RATE_HZ * LOOP_BYTES_PER_SAMPLE))

/*
 * Budget at the values in audio_cfg.h - 4 planes, 4 s delay, 20 s loop, with
 * the loopers now in SDRAM rather than PSRAM:
 *
 *   SDRAM bank 1   looper 0   11 520 000 B   10.99 MiB
 *                  delay       3 072 000 B    2.93 MiB
 *                             -----------------------
 *                             14 592 000 B   13.92 MiB of 16 MiB  -> 2.08 spare
 *
 *   SDRAM bank 2   looper 1   11 520 000 B   10.99 MiB
 *                  reverb      1 048 576 B    1.00 MiB
 *                             -----------------------
 *                             12 568 576 B   11.99 MiB of 16 MiB  -> 4.01 spare
 *
 *   AXI SRAM           69 120 B     68 KiB of 320 KiB
 *   DTCM               10 816 B     11 KiB of 128 KiB
 *
 * The SDRAM and AXI figures are the static asserts below; the DTCM figure is
 * the .dtcm output section of a linked image.
 *
 * WHAT THIS COSTS, AND WHAT IT BUYS
 *
 * PSRAM held 60 s per plane and had no undo. The banks hold 20 s with undo, on
 * parts already fitted, with no QSPI part in the loop path at all. On the
 * 64 MiB SDRAMs this scales straight back up - raise LOOP_MAX_SEC and the two
 * region constants together.
 *
 * WHAT IT COSTS THAT IS NOT A NUMBER
 *
 * Each bank now carries a second access pattern. Bank 1 has delay lines AND a
 * looper walking their own rows, bank 2 the same with reverb, against four
 * internal banks - four open rows - per chip. Both patterns are sequential, so
 * a row lasts 512 samples and the activate cost is small; but this is the thing
 * to look at first if block-time headroom gets tight, and it is the reason the
 * two loopers are on separate chip selects rather than sharing one.
 */

FXC_STATIC_ASSERT(MEM_SDRAM_DELAY_TOTAL  <= MEM_SDRAM_BANK_BYTES,       mem_sdram_delay_fits);
FXC_STATIC_ASSERT(MEM_SDRAM_REVERB_TOTAL <= MEM_SDRAM_BANK_BYTES,       mem_sdram_reverb_fits);
FXC_STATIC_ASSERT(MEM_SRAM_FAST_TOTAL    <= MEM_SRAM_FAST_BUDGET_BYTES, mem_sram_fast_fits);

/*
 * If this one fails, LOOP_MAX_SEC in Shared/fx_defs.h is longer than a bank's
 * looper region can hold. MEM_LOOP_SEC_THAT_FIT is the honest number - either
 * lower LOOP_MAX_SEC to it, or raise MEM_SDRAM_LOOP_REGION_BYTES here AND
 * SDRAM_LOOP_A / SDRAM_LOOP_B in the linker script AND the staging slots in the
 * interface controller, which all have to agree.
 */
FXC_STATIC_ASSERT(MEM_LOOP_BYTES_PER_LOOPER <= MEM_SDRAM_LOOP_REGION_BYTES,
                  mem_sdram_loops_fit);

/* And the whole of each bank, looper included. */
FXC_STATIC_ASSERT((MEM_SDRAM_LOOP_REGION_BYTES + MEM_SDRAM_DELAY_TOTAL)
                      <= MEM_SDRAM_BANK_BYTES, mem_bank1_fits);
FXC_STATIC_ASSERT((MEM_SDRAM_LOOP_REGION_BYTES + MEM_SDRAM_REVERB_TOTAL)
                      <= MEM_SDRAM_BANK_BYTES, mem_bank2_fits);



#endif // #ifndef MEM_MAP_H

/****************************************** end of file *******************************************/
