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

int main(void)
{
    for (;;) {
    }

    return 0;
}
