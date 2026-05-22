/*
 * RA6T1 Linker Script
 *
 * Flash : 512 KB  @ 0x00000000  (program flash, read-only at run time)
 * SRAM  : 128 KB  @ 0x20000000  (main SRAM)
 *
 * Memory map reference: RA6T1 User's Manual Hardware, Rev.1.00, Figure 4.1
 */

ENTRY(Reset_Handler)

/* Stack placed at top of SRAM (grows downward) */
_stack_size = 0x800;   /* 2 KB — expand as needed */
_estack     = ORIGIN(RAM) + LENGTH(RAM);

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    /* Vector table must be at Flash origin (0x00000000) */
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    /* Executable code + read-only data */
    .text :
    {
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        _etext = .;
    } > FLASH

    /* C++ exception handling index tables */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* Load (Flash) address of the .data initialisation image */
    _sidata = LOADADDR(.data);

    /* Initialised data: lives in SRAM at run time, image stored in Flash */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* Zero-initialised data */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Reserve stack space at the top of SRAM */
    ._stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + _stack_size;
        . = ALIGN(8);
    } > RAM

    /* Discard C library helper sections not needed for bare-metal */
    /DISCARD/ :
    {
        *(.fini)
        *(.fini_array)
        *(.init_array)
        libc.a (*)
        libm.a (*)
        libgcc.a (*)
    }
}
