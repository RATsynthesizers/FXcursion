/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author      : STM32CubeIDE
**
**  Abstract    : Linker script for STM32H7 series
**                2048Kbytes FLASH and 1056Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** Copyright (c) 2025 STMicroelectronics.
** All rights reserved.
**
** This software is licensed under terms that can be found in the LICENSE file
** in the root directory of this software component.
** If no LICENSE file comes with this software, it is provided AS-IS.
**
****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM_D1) + LENGTH(RAM_D1);    /* end of RAM */
/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x1000;      /* required amount of heap  */
_Min_Stack_Size = 0x1000; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
  /* ==========================================================================
   * FMC SDRAM BANK 1, 0xC0000000. One W9812G6JH, 16M, divided three ways.
   *
   * This replaced two regions declared as 16M at 0xC0000000 and 14M at
   * 0xC0200000, which OVERLAP: the first one's declared length covered the
   * whole of the second. ld only diagnoses overlapping SECTIONS, never
   * overlapping MEMORY regions, so it linked cleanly and worked only because
   * nothing but the framebuffer lived low. Anything placed in >SDRAM that grew
   * past 2M would have landed silently on top of the recorder ring.
   *
   * ---- SDRAM_DISP, 1M -------------------------------------------------
   * The framebuffer is 320 x 240 x 2 B = 150 KiB, single buffered, and
   * TouchGFXGeneratedHAL.cpp sets it by ABSOLUTE ADDRESS - 0xC0000000, which
   * is this region's base - not by linker section. That file is generated, so
   * the address cannot be moved from here; the region exists to keep anything
   * else away from it and to give DMA2D somewhere to grow into.
   *
   * 1M is 6.8 framebuffers. Double buffering plus animation storage is three
   * of them, 450 KiB, so there is room for both and change. Raise it if a
   * bitmap cache ever moves off flash.
   *
   * ---- SDRAM_REC, 4M --------------------------------------------------
   * The recorder ring, sized for FIVE SECONDS of stall absorption:
   *
   *   4 planes * 4 B * 48000 Hz          = 768000 B/s
   *   REC_CHUNKS 256 * 1024 samples      = 262144 samples per plane
   *   262144 / 48000                     = 5.46 s
   *   256 * 1024 * 4 planes * 4 B        = 4194304 B, exactly 4M
   *
   * The size falls out of REC_CHUNKS, so the region and the constant have to
   * move together - and ld reports "region SDRAM_REC overflowed" if they do
   * not, which is the check.
   *
   * ---- SDRAM_LOOP_A / _B, 5.5M each -----------------------------------
   * Everything left, in TWO slots. Staging for loops in transit between the
   * audio board and the card - received over SPI, written to SD from here, so
   * that neither side waits on the other and the audio board's buffer is
   * released as soon as the transfer ends.
   *
   * Two rather than one because a save must not block the next take. While
   * slot A is being written to the card, slot B can already be receiving the
   * loop the player has just finished - the card is slower than the link, and
   * one slot would make every save wait for the previous one to reach the SD.
   *
   * 5.5M is not arbitrary. It is exactly one ACTIVE stereo loop as the audio
   * board holds it:
   *
   *   LOOP_MAX_SEC 20 * 48000 * 3 B * 2 planes = 5 760 000 B
   *   5.5 MiB                                  = 5 767 168 B   -> 7168 spare
   *
   * So a loop the audio board can record always fits a slot. The audio board's
   * mem_map.h asserts its half of that relationship and Test/test_memory.c
   * checks it on every host run; raising LOOP_MAX_SEC without raising these two
   * regions breaks it.
   * ======================================================================== */
  SDRAM_DISP   (rw) : ORIGIN = 0xC0000000, LENGTH = 1M
  SDRAM_REC    (rw) : ORIGIN = 0xC0100000, LENGTH = 4M
  SDRAM_LOOP_A (rw) : ORIGIN = 0xC0500000, LENGTH = 5632K
  SDRAM_LOOP_B (rw) : ORIGIN = 0xC0A80000, LENGTH = 5632K

  /* FMC SDRAM BANK 2, 0xD0000000. Second W9812G6JH, 16M.
   *
   * PB5/PB6 carry SDCKE1/SDNE1 and fmc.c configures hsdram2, but the device
   * was never taken through its power-up sequence and no region ever named it,
   * so this half of the memory has been inert. Declared here so the linker can
   * place into it; MPU region 5 covers it. */
  SDRAM2 (rw)    : ORIGIN = 0xD0000000, LENGTH = 16M
  FLASH (rx)     : ORIGIN = 0x08000000, LENGTH = 2048K
  DTCMRAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1 (xrw)   : ORIGIN = 0x24000000, LENGTH = 512K
  /* 256K, not the 288K the device has at 0x30000000.
   *
   * The MPU region that marks this domain non-cacheable for the DMA
   * buffers must be a power of two and naturally aligned, so 256K is the
   * largest single region that fits. Declaring the same 256K here makes
   * the LINKER refuse to place anything the MPU does not cover, instead of
   * silently leaving a buffer cacheable while a DMA reads stale lines. */
  RAM_D2 (xrw)   : ORIGIN = 0x30000000, LENGTH = 256K
  RAM_D3 (xrw)   : ORIGIN = 0x38000000, LENGTH = 64K
  ITCMRAM (xrw)  : ORIGIN = 0x00000000, LENGTH = 64K
}

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH
  .ARM (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array (READONLY) : /* The READONLY keyword is only supported in GCC11 and later, remove it if using GCC10 or earlier. */
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */
    *(.RamFunc)        /* .RamFunc sections */
    *(.RamFunc*)       /* .RamFunc* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM_D1 AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM_D1

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM_D1
  
  /* Buffers a bus master reads or writes, in RAM_D2 and mapped
   * NON-CACHEABLE by the MPU - see MPU_Config in Init.c.
   *
   * NOLOAD, so nothing here is zeroed at reset. Every user must
   * initialise its own buffer; all of them already do, either by
   * memset or because a DMA fills the buffer before anything reads it.
   *
   * RAM_D2 is also DMA1/DMA2's own domain, so these transfers no
   * longer cross the D2-to-D1 bridge that RAM_D1 required. */
  .dma_buffers (NOLOAD) :
  {
    . = ALIGN(32);
    *(.dma_buffers)
    *(.dma_buffers*)
    . = ALIGN(32);
  } >RAM_D2
  /* Display. The framebuffer itself is placed by absolute address from
     generated code, so this section is for anything else the display side
     needs - it exists to bound that growth against the region. */
  .sdram_disp (NOLOAD) :
  {
    . = ALIGN(32);
	*(.sdram_disp)
	*(.sdram)          /* old name, kept so a stale tag still links here */
	. = ALIGN(32);
  } >SDRAM_DISP

  /* Recorder ring. Overflowing this is a link error naming SDRAM_REC, which
     is how REC_CHUNKS and the region length are kept in agreement. */
  .sdram_rec (NOLOAD) :
  {
    . = ALIGN(32);
	*(.sdram_rec)
	*(.sdrambuf)       /* old name, kept so a stale tag still links here */
	. = ALIGN(32);
  } >SDRAM_REC

  /* Loop staging, two slots. */
  .sdram_loop_a (NOLOAD) :
  {
    . = ALIGN(32);
	*(.sdram_loop_a)
	. = ALIGN(32);
  } >SDRAM_LOOP_A

  .sdram_loop_b (NOLOAD) :
  {
    . = ALIGN(32);
	*(.sdram_loop_b)
	. = ALIGN(32);
  } >SDRAM_LOOP_B

  /* Bounds of each WHOLE slot, not just what is placed in it - a loop is sized
     at run time against what the audio board actually sends, so the code that
     manages a slot wants the region, not a fixed array. Declare as
     "extern uint8_t _sloopslot_a[];" and take the difference for the size,
     rather than hardcoding 0xC0500000 anywhere. */
  _sloopslot_a = ORIGIN(SDRAM_LOOP_A);
  _eloopslot_a = ORIGIN(SDRAM_LOOP_A) + LENGTH(SDRAM_LOOP_A);
  _sloopslot_b = ORIGIN(SDRAM_LOOP_B);
  _eloopslot_b = ORIGIN(SDRAM_LOOP_B) + LENGTH(SDRAM_LOOP_B);

  /* Bank 2. Declared, but NOT populated on the test board - nothing is placed
     here and nothing should be until a device is fitted and answers a
     pattern write/read across 0xD0000000, because a configured bank with no
     chip on it returns plausible-looking garbage rather than faulting. */
  .sdram2 (NOLOAD) :
  {
    . = ALIGN(4);
	*(.sdram2)
	. = ALIGN(4);
  } >SDRAM2
  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
  
    
	  .freertos_heap (NOLOAD) :
	{
	    . = ALIGN(8);
	    *(.freertos_heap)
	    . = ALIGN(8);
	} >RAM_D1
}


