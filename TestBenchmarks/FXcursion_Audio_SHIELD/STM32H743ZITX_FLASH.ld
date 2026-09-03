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
_Min_Heap_Size = 0x200;      /* required amount of heap  */
_Min_Stack_Size = 0x400; /* required amount of stack */

/* Specify the memory areas */
MEMORY
{
  /* Two SDRAM banks on separate chip selects, each carrying one looper plus
   * one class of effect memory. Delay and reverb are kept apart on purpose, and
   * the two loopers likewise - see Config/mem_map.h.
   *
   * The 11M looper regions match MEM_SDRAM_LOOP_REGION_BYTES and, through it,
   * the staging slots on the interface controller. All three move together or a
   * loop that one side can hold will not fit the other; mem_map.h asserts the
   * audio half of that, and overflowing a region here is a link error naming it.
   *
   *   bank 1   0xC0000000   11M looper 0 (planes 0,1 + undo)  |  5M delay
   *   bank 2   0xD0000000   11M looper 1 (planes 2,3 + undo)  |  5M reverb */
  SDRAM_LOOP_A (rw) : ORIGIN = 0xC0000000, LENGTH = 11M
  SDRAM_DLY    (rw) : ORIGIN = 0xC0B00000, LENGTH = 5M
  SDRAM_LOOP_B (rw) : ORIGIN = 0xD0000000, LENGTH = 11M
  SDRAM_REV    (rw) : ORIGIN = 0xD0B00000, LENGTH = 5M
  FLASH (rx)     : ORIGIN = 0x08000000, LENGTH = 2048K
  DTCMRAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_D1 (xrw)   : ORIGIN = 0x24000000, LENGTH = 512K
  /* 256K, not the 288K the device actually has at 0x30000000.
   *
   * MPU region 0 marks this domain NON-CACHEABLE so the audio DMA buffers need
   * no clean or invalidate - see MPU_Config in main.c. MPU regions must be a
   * power of two and naturally aligned, so the largest single region that fits
   * here is 256K, and the remaining 32K at 0x30040000 falls back to the default
   * write-back write-allocate mapping.
   *
   * Nothing is placed there today (.ramd2dma is a few KiB), so this is a trap
   * rather than a bug: add buffers, cross 256K, and one of them is silently
   * cached while the DMA reads stale lines. Shrinking the region here makes the
   * LINKER refuse to place anything the MPU does not cover, which is a build
   * error instead of a fault that only shows up as occasional clicks.
   *
   * To use the last 32K, add a second MPU region for it first. */
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

  /* ------------------------------------------------------------------------
   * Audio engine placement. The section names come from Config/mem_map.h.
   *
   * All of these are NOLOAD, so nothing here is copied from flash and nothing
   * is zeroed by the startup code: the startup fill only covers _sbss.._ebss.
   * Every object placed here is therefore GARBAGE at reset and must be
   * initialised explicitly by its module - AudioSys_Init calls each effect's
   * Reset and LoopStore_Init clears the staging windows for exactly this
   * reason. Do not place anything here that relies on static initialisation.
   * ------------------------------------------------------------------------ */

  /* Hot state: filter memory, LFO phase, parameters, meters, loop windows.
   * Zero wait state and never cached, so DMA into it needs no maintenance. */
  .dtcm (NOLOAD) :
  {
    . = ALIGN(32);
    *(.dtcm)
    *(.dtcm*)
    . = ALIGN(32);
  } >DTCMRAM

  /* Short modulation lines: chorus, flanger, vibrato. */
  .ramd1 (NOLOAD) :
  {
    . = ALIGN(32);
    *(.ramd1)
    *(.ramd1*)
    . = ALIGN(32);
  } >RAM_D1

  /* Audio DMA buffers.
   *
   * RAM_D2, because DMA1 and DMA2 are D2 masters: they cannot reach DTCM at
   * all, and reaching AXI SRAM costs a bridge. Nothing else is placed in this
   * region, which is what lets MPU_Config mark the whole of it non-cacheable
   * without having to reason about where the section landed. */
  .ramd2dma (NOLOAD) :
  {
    . = ALIGN(32);
    *(.ramd2dma)
    *(.ramd2dma*)
    . = ALIGN(32);
  } >RAM_D2

  /* Looper 0 - planes 0,1 and their undo snapshots. SDRAM bank 1. */
  .sdram_loop_a (NOLOAD) :
  {
    . = ALIGN(32);
    *(.sdram_loop_a)
    *(.sdram_loop_a*)
    . = ALIGN(32);
  } >SDRAM_LOOP_A

  /* Long delay lines, SDRAM bank 1. */
  .sdram_delay (NOLOAD) :
  {
    . = ALIGN(32);
    *(.sdram_delay)
    *(.sdram_delay*)
    . = ALIGN(32);
  } >SDRAM_DLY

  /* Looper 1 - planes 2,3 and their undo snapshots. SDRAM bank 2. */
  .sdram_loop_b (NOLOAD) :
  {
    . = ALIGN(32);
    *(.sdram_loop_b)
    *(.sdram_loop_b*)
    . = ALIGN(32);
  } >SDRAM_LOOP_B

  /* Reverb, SDRAM bank 2. */
  .sdram_reverb (NOLOAD) :
  {
    . = ALIGN(32);
    *(.sdram_reverb)
    *(.sdram_reverb*)
    . = ALIGN(32);
  } >SDRAM_REV

  /* Nothing uses IN_ITCM yet. Before anything does, this needs "AT> FLASH" and
   * a copy loop in Reset_Handler, exactly like .data - otherwise the code
   * placed here is never written to ITCM and the core executes garbage. */
  .itcm (NOLOAD) :
  {
    . = ALIGN(8);
    *(.itcm)
    *(.itcm*)
    . = ALIGN(8);
  } >ITCMRAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}


