/*
 * main.c — RA6T1 application entry point
 *
 * At this point the C runtime is fully initialised:
 *   - .data is in SRAM (copied from Flash by Reset_Handler)
 *   - .bss  is zeroed
 *   - Stack pointer is set to the top of SRAM
 *
 * MCU state on entry:
 *   - Clock : HOCO, 24 MHz (default after POR — configure PLL here for 120 MHz)
 *   - Interrupts : disabled (PRIMASK = 1 until explicitly enabled)
 *   - All peripherals : module-stopped (MSTPCRB/C/D registers)
 */

#include <stdint.h>
#include "hal/hal_heartbeat.h"

/*
 * SYSCLK_HZ — current HCLK frequency fed to SysTick.
 *
 * After POR the RA6T1 runs on HOCO at 24 MHz.
 * Once a clock-init routine switches the PLL on (10 MHz XTAL × 12 = 120 MHz),
 * update this constant AND call hal_heartbeat_init() again so the SysTick
 * reload register is recalculated for the new clock rate.
 *
 * TODO: replace with a call to a hal_clk_get_sysclk() function once the
 *       PLL initialisation module (hal_clk.c) is implemented.
 */
#define SYSCLK_HZ   24000000UL   /* HOCO default — change to 120000000UL after PLL init */

void delay_ms(uint32_t ms)
{
    /*
     * Simple busy-wait loop for background tasks in main().
     * Not used for timing-critical work — all FOC loop and fault handling
     * runs in ISRs, so this can be as crude as needed.
     *
     * For a 1 ms delay at 24 MHz:
     *   24 000 cycles per ms → 24 000 iterations of an empty loop.
     *
     * Note: the exact number of cycles per iteration depends on the
     *       optimization level and compiler.  This is a rough estimate
     *       that produces approximately 1 ms delays when compiled with
     *       -O2 or -O3.  Adjust the loop count if you change optimization
     *       settings or need more precise timing.
     */
    volatile uint32_t count = (SYSCLK_HZ / 1000U) * ms / 4U;

    while (count--) {
        __asm volatile ("nop");
    }
}

static uint8_t state = 0U;

int main(void)
{
    /*
     * Initialise the 1 kHz heartbeat LED.
     *
     * Internally this:
     *   1. Sets P304 (LED_PORT=3, LED_PIN=4) as a GPIO output.
     *   2. Programs SysTick: LOAD = SYSCLK_HZ/2000 - 1 → fires every 500 µs.
     *   3. Assigns SysTick priority 0xC0 (below FOC loop at 0x00).
     *   4. Starts the SysTick counter.
     *
     * The SysTick_Handler defined in hal_heartbeat.c toggles the LED on
     * every interrupt, producing a 1 kHz blink (1 ms period, 50 % duty).
     */
    hal_heartbeat_init(SYSCLK_HZ);

    /*
     * Enable global interrupts.
     * PRIMASK is set to 1 by Reset_Handler; clear it here so SysTick
     * (and all future ISRs) can fire.
     */
    __asm volatile ("cpsie i" ::: "memory");

    for (;;) {
        /*
         * Background (main-loop) tasks go here — lowest priority work
         * such as parameter updates, diagnostics, and UART communication.
         * All time-critical work (FOC loop, fault handling) runs in ISRs.
         */
        delay_ms(100000);
         state ^= 1U;

        if (state) {
           led_on();
        } else {
           led_off();
        }
    }

    return 0;
}
