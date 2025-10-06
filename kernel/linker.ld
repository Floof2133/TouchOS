/* kernel/linker.ld */
/* Linker script for TouchOS kernel - Multiboot 1 */

ENTRY(_start)

SECTIONS
{
    /* Load address - where GRUB puts us in memory */
    . = 0x100000;

    /* Multiboot header must be early in the file */
    .multiboot : {
        *(.multiboot)
    }

    /* Text section right after multiboot */
    .text : {
        *(.text)
    }

    /* Read-only data */
    .rodata : {
        *(.rodata)
    }

    /* Data section */
    .data : {
        *(.data)
    }

    /* BSS section */
    .bss : {
        *(COMMON)
        *(.bss)
    }
	_kernel_end = .;
}
