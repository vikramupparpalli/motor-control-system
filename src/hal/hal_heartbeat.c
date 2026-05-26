/*
 * hal_heartbeat.c — 1 kHz heartbeat LED via SysTick (RA6T1 bare-metal)
 *
 * Timing diagram:
 *
 *   SysTick  __|‾|__|‾|__|‾|__|‾|__   (2 kHz — fires every 500 µs)
 *   LED      __|‾‾‾‾|____|‾‾‾‾|____   (1 kHz — toggles on each tick)
 *              <-- 1 ms -->
 *
 * Hardware mapping (verify against CPU card schematic r12tu0091):
 *   LED_PORT  = 3  →  R_PORT3 base 0x40080060
 *   LED_PIN   = 4  →  P304
 *   Polarity     : active-low (LED ON when pin is driven LOW — LED# signals
 *                  on the 50-pin connector are all active-low per table 8.1)
 *
 * Register map used (RA6T1 Hardware Manual, §19 Port I/O / §13 ICU):
 *   PCNTR1.PDR   — Port Direction Register  (1 = output)
 *   PCNTR3.POSR  — Port Output Set Register  (write 1 → pin HIGH, atomic)
 *   PCNTR3.PORR  — Port Output Reset Register(write 1 → pin LOW,  atomic)
 *   PFS[n][p]    — Port Function Select      (PMR=0 → GPIO mode)
 *   PWPR         — PFS write-protect register
 *   SysTick CSR/RVR/CVR — Cortex-M4 core timer (ARMv7-M Architecture RM)
 *   SHPR3        — System Handler Priority Register 3 (SysTick priority)
 */

#include "hal_heartbeat.h"
#include <stdint.h>

/* =========================================================================
 * LED configuration
 * Adjust these three macros to match the CPU card schematic (r12tu0091).
 * ======================================================================= */
#define LED_PORT        3U   /* Port number: 3 → P3xx pins                  */
#define LED_PIN         5U   /* Pin  number: 4 → P304                       */
#define LED_ACTIVE_LOW  1    /* 1 = LED anode to VCC (pin LOW turns LED ON)  */

/* =========================================================================
 * Cortex-M4 SysTick registers  (ARMv7-M Architecture Reference Manual)
 * Base: 0xE000E010
 * ======================================================================= */
#define SYST_CSR   (*(volatile uint32_t *)0xE000E010U) /* Control & Status  */
#define SYST_RVR   (*(volatile uint32_t *)0xE000E014U) /* Reload Value      */
#define SYST_CVR   (*(volatile uint32_t *)0xE000E018U) /* Current Value     */

#define SYST_CSR_ENABLE     (1UL << 0) /* Counter enable                    */
#define SYST_CSR_TICKINT    (1UL << 1) /* Assert SysTick exception on zero  */
#define SYST_CSR_CLKSOURCE  (1UL << 2) /* 1 = processor clock (HCLK)        */

/* =========================================================================
 * Cortex-M4 System Handler Priority Register 3
 * SysTick (exception 15) priority is in bits [31:24] of SHPR3.
 * ======================================================================= */
#define SHPR3  (*(volatile uint32_t *)0xE000ED20U)

/*
 * Priority byte written to SHPR3[31:24].
 * RA6T1 implements 4 priority bits (values 0x00–0xF0 in steps of 0x10).
 * 0xC0 → priority level 6 of 8 — below the FOC current loop (0x00)
 *                                  and POEG fault handler (0x10).
 */
#define SYSTICK_IRQ_PRIORITY  0xC0U

/* =========================================================================
 * RA6T1 Port Control registers
 * Each port occupies 0x20 bytes; registers are half-word (16-bit) accessible.
 *
 * R_PORT0 = 0x40080000
 * R_PORT3 = 0x40080060   ← used here (LED_PORT = 3)
 * R_PORTn = 0x40080000 + n × 0x20
 *
 * Within each port (from PCNTR1/PCNTR3 layout, RA6T1 HW Manual §19.2):
 *   Offset 0x00  PCNTR1[15: 0]  PODR — Port Output Data Register  (R/W)
 *   Offset 0x02  PCNTR1[31:16]  PDR  — Port Direction Register     (R/W)
 *                                      bit = 1 → output
 *   Offset 0x08  PCNTR3[15: 0]  POSR — Port Output Set Register    (W-only)
 *                                      write 1 to bit n → pin n HIGH
 *   Offset 0x0A  PCNTR3[31:16]  PORR — Port Output Reset Register  (W-only)
 *                                      write 1 to bit n → pin n LOW
 * ======================================================================= */
#define PORT_BASE(n)  (0x40080000UL + (uint32_t)(n) * 0x20UL)
#define PORT_PDR(n)   (*(volatile uint16_t *)(PORT_BASE(n) + 0x02UL))
#define PORT_POSR(n)  (*(volatile uint16_t *)(PORT_BASE(n) + 0x08UL))
#define PORT_PORR(n)  (*(volatile uint16_t *)(PORT_BASE(n) + 0x0AUL))

/* =========================================================================
 * RA6T1 PFS (Port Function Select) registers  (HW Manual §19.6)
 *
 * PFS base: 0x40080800
 * PFS[port][pin] address = 0x40080800 + (port × 16 + pin) × 4
 *
 * Key bits of PmnPFS (32-bit):
 *   [16]  PMR  — 0 = GPIO mode  (default after POR)
 *   [15]  ASEL — 0 = analog input disabled
 *   [14]  ISEL — 0 = IRQ input disabled
 *   [28:24] PSEL — peripheral function select (ignored when PMR=0)
 *
 * After POR all pins default to PMR=0 (GPIO).  Writing PFS explicitly
 * here is defensive — ensures correct state after soft-reset or bootloader.
 * ======================================================================= */
#define PFS_BASE          0x40080800UL
#define PFS_REG(port, pin) \
    (*(volatile uint32_t *)(PFS_BASE + \
        ((uint32_t)(port) * 16UL + (uint32_t)(pin)) * 4UL))

/* =========================================================================
 * PWPR — PFS Write-Protect Register  (HW Manual §19.6.1)
 * Address: 0x40080D03
 *
 * Bit 7: BOWI  — write-protects the PFSWE bit (1 = locked, default)
 * Bit 6: PFSWE — enables writes to PFS registers (1 = writable)
 *
 * Unlock sequence:  BOWI ← 0,  then PFSWE ← 1
 * Lock   sequence:  PFSWE ← 0, then BOWI  ← 1
 * ======================================================================= */
#define PWPR       (*(volatile uint8_t *)0x40080D03U)
#define PWPR_BOWI   (1U << 7)
#define PWPR_PFSWE  (1U << 6)

/* =========================================================================
 * Static state
 * ======================================================================= */
static volatile uint8_t s_led_state = 0U; /* 0 = OFF, 1 = ON               */

/* =========================================================================
 * Private helpers
 * ======================================================================= */

/**
 * Drive the heartbeat LED ON via PORR/POSR (atomic, no read-modify-write).
 * Safe to call from an ISR.
 */
void led_on(void)
{
#if LED_ACTIVE_LOW
    /* Active-low: drive pin LOW to illuminate */
    PORT_PORR(LED_PORT) = (uint16_t)(1U << LED_PIN);
#else
    /* Active-high: drive pin HIGH to illuminate */
    PORT_POSR(LED_PORT) = (uint16_t)(1U << LED_PIN);
#endif
}

/**
 * Drive the heartbeat LED OFF via POSR/PORR (atomic, no read-modify-write).
 * Safe to call from an ISR.
 */
void led_off(void)
{
#if LED_ACTIVE_LOW
    /* Active-low: drive pin HIGH to extinguish */
    PORT_POSR(LED_PORT) = (uint16_t)(1U << LED_PIN);
#else
    /* Active-high: drive pin LOW to extinguish */
    PORT_PORR(LED_PORT) = (uint16_t)(1U << LED_PIN);
#endif
}

/**
 * Configure LED_PIN as a push-pull GPIO output, initial state = LED OFF.
 *
 * Steps:
 *   1. Unlock PFS write protection.
 *   2. Set PFS to GPIO mode (PMR = 0, all other function bits cleared).
 *   3. Re-lock PFS.
 *   4. Drive pin to idle (LED OFF) before enabling the output direction.
 *   5. Set PDR bit to make the pin an output.
 */
static void led_gpio_init(void)
{
    /* 1 & 2 — Unlock PFS, configure pin as GPIO --------------------- */
    PWPR = 0U;              /* Clear BOWI first (mandatory sequence)    */
    PWPR = PWPR_PFSWE;      /* Enable PFS register writes               */

    PFS_REG(LED_PORT, LED_PIN) = 0U; /* PMR=0, ASEL=0, ISEL=0 → GPIO  */

    /* 3 — Re-lock PFS ---------------------------------------------- */
    PWPR = 0U;              /* Clear PFSWE                              */
    PWPR = PWPR_BOWI;       /* Re-engage write protection               */

    /* 4 — Pre-drive the pin to LED OFF before enabling output -------- */
    /*     Prevents a spurious LED flash on the first half-tick.        */
    led_off();

    /* 5 — Set direction: pin → output (PDR bit = 1) ------------------ */
    PORT_PDR(LED_PORT) |= (uint16_t)(1U << LED_PIN);
}

/* =========================================================================
 * SysTick_Handler
 *
 * Overrides the weak Default_Handler alias declared in startup.c.
 * Fires every 500 µs (2 kHz).  Each invocation toggles the LED, producing
 * a 1 kHz blink (1 ms period, 50 % duty cycle).
 *
 * ISR budget:
 *   500 µs window at 120 MHz = 60 000 cycles.
 *   This handler executes in < 20 cycles — negligible CPU load.
 * ======================================================================= */
void SysTick_Handler(void)
{
    s_led_state ^= 1U;

    if (s_led_state) {
        led_on();
    } else {
        led_off();
    }
}

/* =========================================================================
 * Public API
 * ======================================================================= */

/**
 * hal_heartbeat_init — initialise heartbeat LED and SysTick timer.
 *
 * @param sysclk_hz  HCLK frequency in Hz.
 *                   Pass 24000000UL  when running on the default HOCO.
 *                   Pass 120000000UL after the PLL has been configured.
 */
void hal_heartbeat_init(uint32_t sysclk_hz)
{
    /*
     * SysTick reload for 500 µs period (2 kHz → 1 kHz LED toggle):
     *   LOAD = sysclk_hz / 2000 - 1
     *
     *   24 MHz  → LOAD =  11 999  (well within 24-bit max of 16 777 215)
     *  120 MHz  → LOAD =  59 999
     */
    const uint32_t load = (sysclk_hz / 2000U) - 1U;

    /* Step 1 — Configure LED GPIO (direction, PFS, idle state) --------- */
    led_gpio_init();

    /* Step 2 — Set SysTick interrupt priority -------------------------- */
    /*
     * SHPR3 bits [31:24] hold the SysTick priority byte.
     * 0xC0 places heartbeat well below the FOC current-loop ISR (0x00)
     * and the POEG hardware-fault handler (0x10), so a sustained burst
     * of high-priority interrupts may delay the LED toggle slightly but
     * will never be delayed by the heartbeat.
     */
    SHPR3 = (SHPR3 & ~(0xFFUL << 24U)) | ((uint32_t)SYSTICK_IRQ_PRIORITY << 24U);

    /* Step 3 — Program SysTick reload value ---------------------------- */
    SYST_RVR = load;

    /* Step 4 — Clear the current-value register to avoid a short first
     *           tick if the counter happens to be mid-way through a cycle */
    SYST_CVR = 0U;

    /* Step 5 — Enable SysTick:
     *   CLKSOURCE = 1 → use HCLK (processor clock, same domain as LOAD)
     *   TICKINT   = 1 → assert SysTick exception on countdown-to-zero
     *   ENABLE    = 1 → start the counter                               */
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}
