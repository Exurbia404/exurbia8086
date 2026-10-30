.code16
.global _start
_start:
    cli
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %sp
    sti

    # Read kernel (sectors 2–17) to physical address 0x1000
    mov $0x02, %ah
    mov $16, %al
    mov $0x00, %ch
    mov $0x02, %cl
    mov $0x00, %dh
    mov $0x00, %dl          # floppy drive A:
    mov $0x1000, %bx
    int $0x13
    jc disk_error

    jmp $0x0000, $0x1000    # jump to kernel

disk_error:
    cli
1:
    hlt
    jmp 1b

    .fill 510-(.-_start), 1, 0
    .word 0xAA55
