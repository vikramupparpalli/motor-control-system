/*
 * startup.c — RA6T1 bare-metal startup
 *
 * Responsibilities:
 *   1. Place the Cortex-M4 vector table at Flash origin (0x00000000).
 *   2. Reset_Handler: copy .data to SRAM, zero .bss, then call main().
 *   3. Provide weak Default_Handler stubs for every exception and IRQ so
 *      that the linker is satisfied until real handlers are written.
 *
 * Vector table layout (RA6T1 Hardware Manual, Table 13.3):
 *   Offset 0x000        : Initial Main Stack Pointer
 *   Offsets 0x004–0x03C : 15 Cortex-M4 core exceptions
 *   Offsets 0x040–0x1BC : 96 configurable IRQs (ICU.IELSR0–IELSR95)
 *
 * Clock note:
 *   After POR the MCU runs on HOCO at 24 MHz.  Call a clock-init routine
 *   from main() (or add a SystemInit() call here) to switch to the
 *   PLL-derived 120 MHz system clock.
 *
 * Watchdog note:
 *   OFS0/OFS1 option bytes default to 0xFFFFFFFF which leaves both the
 *   WDT and IWDT stopped.  No watchdog kicking is required here.
 */

#include <stdint.h>

/* -------------------------------------------------------------------------
 * Linker-defined symbols (see ld/ra6t1.ld)
 * ---------------------------------------------------------------------- */
extern uint32_t _sidata;   /* Load address of .data image in Flash         */
extern uint32_t _sdata;    /* Start of .data region in SRAM                */
extern uint32_t _edata;    /* End   of .data region in SRAM                */
extern uint32_t _sbss;     /* Start of .bss  region                        */
extern uint32_t _ebss;     /* End   of .bss  region                        */
extern uint32_t _estack;   /* Top of stack (= SRAM base + SRAM length)     */

/* -------------------------------------------------------------------------
 * Application entry point
 * ---------------------------------------------------------------------- */
extern int main(void);

/* -------------------------------------------------------------------------
 * Handler forward declarations
 * ---------------------------------------------------------------------- */
void Reset_Handler(void);
void Default_Handler(void);

/* --- Cortex-M4 core exception handlers (override by defining in app) --- */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* --- RA6T1 external IRQs: ICU.IELSR0–IELSR95 (Table 13.3) ------------- */
void IRQ00_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ01_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ02_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ03_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ04_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ05_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ06_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ07_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ08_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ09_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ10_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ11_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ12_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ13_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ14_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ15_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ16_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ17_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ18_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ19_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ20_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ21_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ22_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ23_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ24_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ25_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ26_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ27_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ28_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ29_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ30_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ31_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ32_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ33_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ34_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ35_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ36_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ37_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ38_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ39_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ40_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ41_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ42_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ43_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ44_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ45_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ46_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ47_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ48_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ49_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ50_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ51_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ52_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ53_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ54_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ55_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ56_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ57_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ58_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ59_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ60_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ61_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ62_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ63_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ64_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ65_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ66_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ67_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ68_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ69_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ70_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ71_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ72_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ73_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ74_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ75_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ76_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ77_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ78_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ79_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ80_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ81_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ82_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ83_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ84_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ85_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ86_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ87_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ88_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ89_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ90_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ91_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ92_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ93_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ94_Handler(void) __attribute__((weak, alias("Default_Handler")));
void IRQ95_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* -------------------------------------------------------------------------
 * Interrupt vector table
 * 112 entries = 16 core (Arm exceptions) + 96 external (ICU slots)
 * Must be placed at 0x00000000 (Flash origin).
 * ---------------------------------------------------------------------- */
typedef void (*isr_t)(void);

__attribute__((section(".isr_vector"), used))
const isr_t g_vector_table[] = {
    /* ---- Cortex-M4 core exceptions ----------------------------------- */
    (isr_t)&_estack,       /* 0x000  Initial Main Stack Pointer           */
    Reset_Handler,          /* 0x004  Reset                                */
    NMI_Handler,            /* 0x008  Non-Maskable Interrupt               */
    HardFault_Handler,      /* 0x00C  Hard Fault                           */
    MemManage_Handler,      /* 0x010  MPU Fault                            */
    BusFault_Handler,       /* 0x014  Bus Fault                            */
    UsageFault_Handler,     /* 0x018  Usage Fault                          */
    (isr_t)0,               /* 0x01C  Reserved                             */
    (isr_t)0,               /* 0x020  Reserved                             */
    (isr_t)0,               /* 0x024  Reserved                             */
    (isr_t)0,               /* 0x028  Reserved                             */
    SVC_Handler,            /* 0x02C  Supervisor Call                      */
    DebugMon_Handler,       /* 0x030  Debug Monitor                        */
    (isr_t)0,               /* 0x034  Reserved                             */
    PendSV_Handler,         /* 0x038  Pendable Service Request             */
    SysTick_Handler,        /* 0x03C  System Tick Timer                    */

    /* ---- RA6T1 external IRQs: ICU.IELSR0–IELSR95 (Table 13.3) ------- */
    IRQ00_Handler,          /* 0x040  Exception 16  / IELSR0               */
    IRQ01_Handler,          /* 0x044  Exception 17  / IELSR1               */
    IRQ02_Handler,          /* 0x048  Exception 18  / IELSR2               */
    IRQ03_Handler,          /* 0x04C  Exception 19  / IELSR3               */
    IRQ04_Handler,          /* 0x050  Exception 20  / IELSR4               */
    IRQ05_Handler,          /* 0x054  Exception 21  / IELSR5               */
    IRQ06_Handler,          /* 0x058  Exception 22  / IELSR6               */
    IRQ07_Handler,          /* 0x05C  Exception 23  / IELSR7               */
    IRQ08_Handler,          /* 0x060  Exception 24  / IELSR8               */
    IRQ09_Handler,          /* 0x064  Exception 25  / IELSR9               */
    IRQ10_Handler,          /* 0x068  Exception 26  / IELSR10              */
    IRQ11_Handler,          /* 0x06C  Exception 27  / IELSR11              */
    IRQ12_Handler,          /* 0x070  Exception 28  / IELSR12              */
    IRQ13_Handler,          /* 0x074  Exception 29  / IELSR13              */
    IRQ14_Handler,          /* 0x078  Exception 30  / IELSR14              */
    IRQ15_Handler,          /* 0x07C  Exception 31  / IELSR15              */
    IRQ16_Handler,          /* 0x080  Exception 32  / IELSR16              */
    IRQ17_Handler,          /* 0x084  Exception 33  / IELSR17              */
    IRQ18_Handler,          /* 0x088  Exception 34  / IELSR18              */
    IRQ19_Handler,          /* 0x08C  Exception 35  / IELSR19              */
    IRQ20_Handler,          /* 0x090  Exception 36  / IELSR20              */
    IRQ21_Handler,          /* 0x094  Exception 37  / IELSR21              */
    IRQ22_Handler,          /* 0x098  Exception 38  / IELSR22              */
    IRQ23_Handler,          /* 0x09C  Exception 39  / IELSR23              */
    IRQ24_Handler,          /* 0x0A0  Exception 40  / IELSR24              */
    IRQ25_Handler,          /* 0x0A4  Exception 41  / IELSR25              */
    IRQ26_Handler,          /* 0x0A8  Exception 42  / IELSR26              */
    IRQ27_Handler,          /* 0x0AC  Exception 43  / IELSR27              */
    IRQ28_Handler,          /* 0x0B0  Exception 44  / IELSR28              */
    IRQ29_Handler,          /* 0x0B4  Exception 45  / IELSR29              */
    IRQ30_Handler,          /* 0x0B8  Exception 46  / IELSR30              */
    IRQ31_Handler,          /* 0x0BC  Exception 47  / IELSR31              */
    IRQ32_Handler,          /* 0x0C0  Exception 48  / IELSR32              */
    IRQ33_Handler,          /* 0x0C4  Exception 49  / IELSR33              */
    IRQ34_Handler,          /* 0x0C8  Exception 50  / IELSR34              */
    IRQ35_Handler,          /* 0x0CC  Exception 51  / IELSR35              */
    IRQ36_Handler,          /* 0x0D0  Exception 52  / IELSR36              */
    IRQ37_Handler,          /* 0x0D4  Exception 53  / IELSR37              */
    IRQ38_Handler,          /* 0x0D8  Exception 54  / IELSR38              */
    IRQ39_Handler,          /* 0x0DC  Exception 55  / IELSR39              */
    IRQ40_Handler,          /* 0x0E0  Exception 56  / IELSR40              */
    IRQ41_Handler,          /* 0x0E4  Exception 57  / IELSR41              */
    IRQ42_Handler,          /* 0x0E8  Exception 58  / IELSR42              */
    IRQ43_Handler,          /* 0x0EC  Exception 59  / IELSR43              */
    IRQ44_Handler,          /* 0x0F0  Exception 60  / IELSR44              */
    IRQ45_Handler,          /* 0x0F4  Exception 61  / IELSR45              */
    IRQ46_Handler,          /* 0x0F8  Exception 62  / IELSR46              */
    IRQ47_Handler,          /* 0x0FC  Exception 63  / IELSR47              */
    IRQ48_Handler,          /* 0x100  Exception 64  / IELSR48              */
    IRQ49_Handler,          /* 0x104  Exception 65  / IELSR49              */
    IRQ50_Handler,          /* 0x108  Exception 66  / IELSR50              */
    IRQ51_Handler,          /* 0x10C  Exception 67  / IELSR51              */
    IRQ52_Handler,          /* 0x110  Exception 68  / IELSR52              */
    IRQ53_Handler,          /* 0x114  Exception 69  / IELSR53              */
    IRQ54_Handler,          /* 0x118  Exception 70  / IELSR54              */
    IRQ55_Handler,          /* 0x11C  Exception 71  / IELSR55              */
    IRQ56_Handler,          /* 0x120  Exception 72  / IELSR56              */
    IRQ57_Handler,          /* 0x124  Exception 73  / IELSR57              */
    IRQ58_Handler,          /* 0x128  Exception 74  / IELSR58              */
    IRQ59_Handler,          /* 0x12C  Exception 75  / IELSR59              */
    IRQ60_Handler,          /* 0x130  Exception 76  / IELSR60              */
    IRQ61_Handler,          /* 0x134  Exception 77  / IELSR61              */
    IRQ62_Handler,          /* 0x138  Exception 78  / IELSR62              */
    IRQ63_Handler,          /* 0x13C  Exception 79  / IELSR63              */
    IRQ64_Handler,          /* 0x140  Exception 80  / IELSR64              */
    IRQ65_Handler,          /* 0x144  Exception 81  / IELSR65              */
    IRQ66_Handler,          /* 0x148  Exception 82  / IELSR66              */
    IRQ67_Handler,          /* 0x14C  Exception 83  / IELSR67              */
    IRQ68_Handler,          /* 0x150  Exception 84  / IELSR68              */
    IRQ69_Handler,          /* 0x154  Exception 85  / IELSR69              */
    IRQ70_Handler,          /* 0x158  Exception 86  / IELSR70              */
    IRQ71_Handler,          /* 0x15C  Exception 87  / IELSR71              */
    IRQ72_Handler,          /* 0x160  Exception 88  / IELSR72              */
    IRQ73_Handler,          /* 0x164  Exception 89  / IELSR73              */
    IRQ74_Handler,          /* 0x168  Exception 90  / IELSR74              */
    IRQ75_Handler,          /* 0x16C  Exception 91  / IELSR75              */
    IRQ76_Handler,          /* 0x170  Exception 92  / IELSR76              */
    IRQ77_Handler,          /* 0x174  Exception 93  / IELSR77              */
    IRQ78_Handler,          /* 0x178  Exception 94  / IELSR78              */
    IRQ79_Handler,          /* 0x17C  Exception 95  / IELSR79              */
    IRQ80_Handler,          /* 0x180  Exception 96  / IELSR80              */
    IRQ81_Handler,          /* 0x184  Exception 97  / IELSR81              */
    IRQ82_Handler,          /* 0x188  Exception 98  / IELSR82              */
    IRQ83_Handler,          /* 0x18C  Exception 99  / IELSR83              */
    IRQ84_Handler,          /* 0x190  Exception 100 / IELSR84              */
    IRQ85_Handler,          /* 0x194  Exception 101 / IELSR85              */
    IRQ86_Handler,          /* 0x198  Exception 102 / IELSR86              */
    IRQ87_Handler,          /* 0x19C  Exception 103 / IELSR87              */
    IRQ88_Handler,          /* 0x1A0  Exception 104 / IELSR88              */
    IRQ89_Handler,          /* 0x1A4  Exception 105 / IELSR89              */
    IRQ90_Handler,          /* 0x1A8  Exception 106 / IELSR90              */
    IRQ91_Handler,          /* 0x1AC  Exception 107 / IELSR91              */
    IRQ92_Handler,          /* 0x1B0  Exception 108 / IELSR92              */
    IRQ93_Handler,          /* 0x1B4  Exception 109 / IELSR93              */
    IRQ94_Handler,          /* 0x1B8  Exception 110 / IELSR94              */
    IRQ95_Handler,          /* 0x1BC  Exception 111 / IELSR95              */
};

/* -------------------------------------------------------------------------
 * Default_Handler
 * Spins on any unhandled exception so a debugger can identify the fault.
 * ---------------------------------------------------------------------- */
void Default_Handler(void)
{
    for (;;) {
    }
}

/* -------------------------------------------------------------------------
 * Reset_Handler
 * C-runtime initialisation executed immediately after reset.
 *
 * Sequence:
 *   1. Copy .data initialisation image from Flash to SRAM.
 *   2. Zero-fill .bss.
 *   3. Call main().
 *   4. Trap if main() returns (should never happen in embedded).
 * ---------------------------------------------------------------------- */
void Reset_Handler(void)
{
    uint32_t *src;
    uint32_t *dst;

    /* Copy initialised data from Flash load address to SRAM */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* Zero-fill BSS */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0U;
    }

    main();

    /* Trap — main() must not return in a bare-metal application */
    for (;;) {
    }
}
