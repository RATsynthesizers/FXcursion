/**
 * @file      irq_cfg.h
 *
 * @details   The interrupt priority plan, in one place.
 *
 *            ------------------------------------------------------------------
 *            WHY THIS FILE EXISTS
 *            ------------------------------------------------------------------
 *
 *            CubeMX generates every NVIC priority as 0. On a system with no
 *            RTOS and a hard 1333 us audio deadline that is not a neutral
 *            default: it means the debug UART's DMA completion can delay the
 *            audio block, and that the order in which two interrupts are
 *            serviced is decided by IRQ number rather than by importance.
 *
 *            Scattering the fix across each MX_*_Init would hide it. The whole
 *            plan is here so it can be read as a plan.
 *
 *            ------------------------------------------------------------------
 *            THE PLAN
 *            ------------------------------------------------------------------
 *
 *            HAL_Init selects NVIC_PRIORITYGROUP_4: four bits of preemption
 *            priority, no sub-priority. Lower number preempts higher.
 *
 *              0-3   left free, deliberately. Anything that must beat the audio
 *                    engine has to be added here on purpose.
 *
 *              4     SAI1_A receive DMA. THE audio interrupt: it runs the whole
 *                    engine. Nothing in the system may delay it.
 *
 *              5     the other four audio streams. Their callbacks do nothing -
 *                    they free-run in circular mode - but they must not be
 *                    starved, or a stream stalls.
 *
 *              6     MDMA, which carries loop audio to and from the PSRAM.
 *                    BELOW the audio interrupt on purpose: the loop store's
 *                    chain has 26x the time it needs, so it can afford to be
 *                    preempted, and the audio block cannot.
 *
 *              7     SAI and I2S peripheral errors. Diagnostics only.
 *
 *              8     SPI1, the recorder sample link.
 *
 *              9     USART1, the control link from the interface controller.
 *
 *              10    USART2, debug.
 *
 *              15    SysTick, from TICK_INT_PRIORITY in stm32h7xx_hal_conf.h.
 *                    Lowest, so HAL_Delay bookkeeping never touches audio.
 *
 *            ------------------------------------------------------------------
 *            THE ONE ORDERING THAT IS LOAD BEARING
 *            ------------------------------------------------------------------
 *
 *            IRQ_PRIO_AUDIO must be numerically lower than IRQ_PRIO_LOOP_MDMA.
 *
 *            The loop store's queue is lock free only because the audio context
 *            and the MDMA completion context never append and consume at the
 *            same time. That holds if audio preempts MDMA. Invert them and the
 *            queue needs a critical section.
 *
 * @version   1.0.0
 *
 * @authors   Claude (design draft)
 *
 * \date      01.09.2026 - First release
 *
 * @copyright RAT Synthesizers
 */

#ifndef IRQ_CFG_H
#define IRQ_CFG_H



/***************************************************************************************************
* Definitions of global (public) constants
***************************************************************************************************/

/** SAI1_A receive DMA - the audio block interrupt. */
#define IRQ_PRIO_AUDIO                  (4U)

/** The four free-running audio streams. */
#define IRQ_PRIO_AUDIO_STREAM           (5U)

/** Free since loop audio moved into SDRAM: the DTCM windows are filled by
    memcpy inside the block that asks, so no MDMA chain carries loop audio any
    more. Kept because the number is referenced by the priority table above. */
#define IRQ_PRIO_LOOP_MDMA              (6U)

/** SAI / I2S peripheral error interrupts. */
#define IRQ_PRIO_AUDIO_ERROR            (7U)

/** SPI1, recorder sample transfer. */
#define IRQ_PRIO_RECORDER               (8U)

/** USART1, control link. */
#define IRQ_PRIO_CTRL_LINK              (9U)

/** USART2, debug. */
#define IRQ_PRIO_DEBUG                  (10U)

/** Sub-priority. Priority group 4 leaves no sub-priority bits, so this is 0. */
#define IRQ_SUBPRIO                     (0U)



/***************************************************************************************************
* Configuration sanity checks
***************************************************************************************************/

/*
 * See "THE ONE ORDERING THAT IS LOAD BEARING" above. This is not a style rule -
 * inverting these two makes loop_store's queue racy.
 */
#if (IRQ_PRIO_AUDIO >= IRQ_PRIO_LOOP_MDMA)
#error "The audio interrupt must preempt MDMA, or the loop store queue needs a lock"
#endif

#if (IRQ_PRIO_AUDIO > IRQ_PRIO_AUDIO_STREAM)
#error "The audio block interrupt must not be lower priority than the streams it reads"
#endif

#if (IRQ_PRIO_DEBUG > 15U)
#error "Priority group 4 gives four bits of preemption priority: 0 to 15"
#endif



#endif // #ifndef IRQ_CFG_H

/****************************************** end of file *******************************************/
