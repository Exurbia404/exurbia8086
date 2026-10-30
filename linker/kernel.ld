OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)

SECTIONS
{
    . = 0x1000;
    .text :
    {
        KEEP(*(.text.start))
        *(.text)
        *(.text.*)
    }
    .data : { *(.data) }
    .bss  : { *(.bss) }
    __kernel_end = .;
    ASSERT(__kernel_end <= 0x4000, "kernel and BSS overlap the heap")
}
