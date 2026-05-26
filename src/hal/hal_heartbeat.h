/*
 * hal_heartbeat.h — 1 kHz heartbeat LED driver (SysTick-based)
 *
 * Produces a 1 kHz blink (1 ms period, 50 % duty cycle) on a GPIO-driven
 * status LED using the Cortex-M4 SysTick timer.  No RA6T1 GPT channel is
 * consumed, leaving all eight GPT units free for FOC PWM generation.
 *
 * Usage:
 *   1. Call hal_heartbeat_init(SYSCLK_HZ) once before enabling interrupts.
 *   2. Enable global interrupts with  __asm("cpsie i").
 *   3. The SysTick_Handler fires automatically every 500 µs and toggles the
 *      LED — no further application code is required.
 *
 * LED pin configuration:
 *   Edit the macros in hal_heartbeat.c:
 *     LED_PORT       — RA6T1 port number  (default: 3  → P3xx)
 *     LED_PIN        — pin within that port (default: 4  → P304)
 *     LED_ACTIVE_LOW — 1 if LED anode is pulled to VCC (typical eval board)
 *
 *   Verify the actual pin against the CPU card schematic (r12tu0091 / R12TU0091).
 */

#ifndef HAL_HEARTBEAT_H
#define HAL_HEARTBEAT_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * hal_heartbeat_init
 *
 * Configures the heartbeat LED GPIO and programs SysTick for a 500 µs
 * interrupt period.  The SysTick_Handler (defined in hal_heartbeat.c)
 * toggles the LED on every interrupt, producing a 1 kHz blink.
 *
 * Parameters:
 *   sysclk_hz — current HCLK frequency in Hz.
 *               24 000 000  for the default HOCO after POR.
 *              120 000 000  after PLL is configured.
 *
 * The SysTick reload value is computed as:
 *   LOAD = sysclk_hz / 2000 - 1
 *     24 MHz  →  11 999   (fits in the 24-bit LOAD register, max 16 777 215)
 *    120 MHz  →  59 999
 * ---------------------------------------------------------------------- */
void hal_heartbeat_init(uint32_t sysclk_hz);
void led_on(void);
void led_off(void);
#endif /* HAL_HEARTBEAT_H */
