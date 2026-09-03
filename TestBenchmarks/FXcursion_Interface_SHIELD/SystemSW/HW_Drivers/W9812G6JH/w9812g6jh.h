/*
 * w9812g6jh.h
 *
 *  Created on: 30 ???. 2020 ?.
 *      Author: Predtech
 */

#ifndef INC_W9812G6JH_H_
#define INC_W9812G6JH_H_

/*
 * ======== SDRAM initialization ======================

The initialization sequence is managed by software. If the two banks are used, the
initialization sequence must be generated simultaneously to Bank 1and Bank 2 by setting
the Target Bank bits CTB1 and CTB2 in the FMC_SDCMR register:

1. Program the memory device features into the FMC_SDCRx register. The SDRAM
clock frequency, RBURST and RPIPE must be programmed in the FMC_SDCR1
register.

2. Program the memory device timing into the FMC_SDTRx register. The TRP and TRC
timings must be programmed in the FMC_SDTR1 register.

3. Set MODE bits to ‘001’ and configure the Target Bank bits (CTB1 and/or CTB2) in the
FMC_SDCMR register to start delivering the clock to the memory (SDCKE is driven
high).
4. Wait during the prescribed delay period. Typical delay is around 100 μs (refer to the
SDRAM datasheet for the required delay after power-up).

5. Set MODE bits to ‘010’ and configure the Target Bank bits (CTB1 and/or CTB2) in the
FMC_SDCMR register to issue a “Precharge All” command.

6. Set MODE bits to ‘011’, and configure the Target Bank bits (CTB1 and/or CTB2) as well
as the number of consecutive Auto-refresh commands (NRFS) in the FMC_SDCMR
register. Refer to the SDRAM datasheet for the number of Auto-refresh commands that
should be issued. Typical number is 8.

7. Configure the MRD field, set the MODE bits to ‘100’, and configure the Target Bank bits
(CTB1 and/or CTB2) in the FMC_SDCMR register to issue a “Load Mode Register”
command and program the SDRAM device. In particular the Burst Length (BL) has to
be set to ‘1’) and the CAS latency has to be selected. If the Mode Register is not the
same for both SDRAM banks, this step has to be repeated twice, once for each bank
and the Target Bank bits set accordingly. For mobile SDRAM devices, the MRD field is
also used to configure the extended mode register while issuing the Load Mode
Register”

8. Program the refresh rate in the FMC_SDRTR register
The refresh rate corresponds to the delay between refresh cycles. Its value must be
adapted to SDRAM devices.
At this stage the SDRAM device is ready to accept commands. If a system reset occurs
during an ongoing SDRAM access, the data bus might still be driven by the SDRAM device.
Therefore the SDRAM device must be first reinitialized after reset before issuing any new
access by the NOR Flash/PSRAM/SRAM or NAND Flash controller.

*Note: If two SDRAM devices are connected to the FMC, all the accesses performed at the same
time to both devices by the Command Mode register (Load Mode Register command) are
issued using the timing parameters configured for SDRAM Bank 1 (TMRD andTRAS
timings) in the FMC_SDTR1 register.
 */

#include "main.h"

#define SDRAM_TIMEOUT     ((uint16_t)0xFFFF)

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

#define SDRAM_BANK_ADDR                 ((uint32_t)0xC0000000)   /* FMC bank 1 */
#define SDRAM_BANK2_ADDR                ((uint32_t)0xD0000000)   /* FMC bank 2 */

/*
 * ======== REFRESH RATE ==============================
 *
 *   FMC_SDRTR COUNT = (refresh period / rows) * f_SDCLK - 20
 *
 * The -20 is the FMC's own margin for a refresh that collides with an access
 * already in flight. The FMC also requires COUNT >= 41.
 *
 * DERIVED, NOT A MAGIC NUMBER. A project sets SDRAM_SDCLK_MHZ for its own clock
 * tree and the count follows; it used to be a precomputed constant per project,
 * which is how the interface board came to be running 1350 at 60 MHz - one
 * refresh every 22.8 us against a part that needs one every 15.6, 46% too slow.
 *
 * GETTING IT WRONG IN THE SLOW DIRECTION DOES NOT FAIL ON THE BENCH. The 64 ms
 * retention figure is worst case over temperature; real cells at room
 * temperature hold far longer, so the only symptom is rare bit rot in memory
 * that is not being rewritten. A framebuffer is redrawn every frame and heals
 * itself, which is why nothing ever looked wrong. A recorder ring or a loop
 * buffer is NOT rewritten - it sits untouched for seconds - and a flipped bit
 * there is a click in the take.
 *
 *   interface   SDCLK  60 MHz -> 64000*60/4096  - 20 =  917
 *   audio       SDCLK 120 MHz -> 64000*120/4096 - 20 = 1855
 *
 * Integer division truncates DOWN, which shortens the interval - the safe
 * direction. A 512 Mbit device has 8192 rows, which halves the result.
 */
#ifndef SDRAM_ROW_QTY
#define SDRAM_ROW_QTY                   (4096UL)
#endif

#ifndef SDRAM_REFRESH_PERIOD_US
#define SDRAM_REFRESH_PERIOD_US         (64000UL)
#endif

/** Each project defines this for its own FMC clock tree. */
#ifndef SDRAM_SDCLK_MHZ
#define SDRAM_SDCLK_MHZ                 (60UL)
#endif

#ifndef SDRAM_RFR_COUNT
#define SDRAM_RFR_COUNT \
    (((SDRAM_REFRESH_PERIOD_US * SDRAM_SDCLK_MHZ) / SDRAM_ROW_QTY) - 20UL)
#endif

/** One device, bytes. Bounds the address-line walk in W9812G6JH_SelfTest. */
#define SDRAM_BANK_SIZE_BYTES           ((uint32_t)(16UL * 1024UL * 1024UL))

#define BUFFER_SIZE         ((uint32_t)0x1000000) // 16M bits
#define WRITE_READ_ADDR     ((uint32_t)0x0000000)


// write and read single u32 macro
#define _RAM_WRITE32(data, addr)   ( *(__IO uint32_t*) (SDRAM_BANK_ADDR + addr) = data )
#define _RAM_READ32(addr)          ( *(__IO uint32_t*) (SDRAM_BANK_ADDR + addr)        )
// examples:
// _RAM_WRITE32(u32 a = 0xFFFFFFFF, 0 )
// u32 a = _RAM_READ(0);
// calculation example:
// http://main.lv/writeup/stm32f4_sdram_configuration.md

/**
 * @brief Bring up one SDRAM device on the given FMC bank.
 *
 * @param hsdram   SDRAM handle for that bank
 * @param nTarget  FMC_SDRAM_CMD_TARGET_BANK1 or ..._BANK2
 *
 * Boards with two devices must call this once per bank. Note the caveat at the
 * top of this file: a Load Mode Register command issued to both banks at once
 * uses bank 1's timings, which is why each bank is initialised separately here.
 */
void W9812G6JH_InitBank(SDRAM_HandleTypeDef *hsdram, uint32_t nTarget);

/**
 * @brief Bring up the bank the handle names.
 *
 * Derives the target from hsdram->Init.SDBank, so a two-device board can call
 * this once per handle and get the right bank each time. It used to name bank 1
 * unconditionally, which meant a second call with the bank-2 handle sent the
 * whole power-up sequence to bank 1 again and left the second device without a
 * mode register - the reason nothing could ever be placed at 0xD0000000.
 */
void W9812G6JH_Init(SDRAM_HandleTypeDef *hsdram);

/**
 * @brief Prove a bank actually answers. DESTRUCTIVE.
 *
 * WHY THIS IS NEEDED AT ALL
 *
 * A bank that is configured in the FMC but has no device fitted, or has a
 * broken address line, does not fault. It returns plausible-looking values, and
 * loop audio written into it comes back as noise - or worse, as the wrong part
 * of the loop, which sounds like a bug in the looper.
 *
 * WHAT IT CHECKS, AND WHY NOT MORE
 *
 * Walking-ones on the data bus at the base address, then a unique value at
 * every power-of-two offset across the bank. Between them those catch the
 * failures that actually happen: nothing fitted, a data line shorted or open,
 * an address line swapped or stuck - which is the one that produces aliasing,
 * where two addresses are the same cell and a long loop overwrites its own
 * beginning.
 *
 * It does NOT write every cell. That is a production test, not a boot test:
 * 16 MiB at SDRAM speed is far too long to spend before the first audio block,
 * and cell-level faults are not what a bring-up check is looking for.
 *
 * Destructive by nature, so it must run BEFORE anything is placed in the bank.
 *
 * @param nBase  SDRAM_BANK_ADDR or SDRAM_BANK2_ADDR
 *
 * @return RESULT_OK when every pattern read back
 */
STD_RESULT W9812G6JH_SelfTest(uint32_t nBase);

#endif /* INC_W9812G6JH_H_ */
