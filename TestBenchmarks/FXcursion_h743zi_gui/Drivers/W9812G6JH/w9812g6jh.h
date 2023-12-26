/*
 * w9812g6jh.h
 *
 *  Created on: 30 ???. 2020 ?.
 *      Author: Predtech
 */

#ifndef INC_W9812G6JH_H_
#define INC_W9812G6JH_H_

#ifdef __cplusplus
extern "C" {
#endif
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

#define SDRAM_BANK_ADDR                 ((uint32_t)0xC0000000)
#define SDRAM_RFR_COUNT					3744 // clock cycles to refresh

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

void W9812G6JH_Init(SDRAM_HandleTypeDef *hsdram);

#ifdef __cplusplus
}
#endif

#endif /* INC_W9812G6JH_H_ */
