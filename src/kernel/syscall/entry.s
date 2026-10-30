.code16
.global syscall_interrupt_handler
.extern syscall_dispatch_frame

syscall_interrupt_handler:
    pushw %ax
    pushw %bx
    pushw %cx
    pushw %dx
    pushw %si
    pushw %di
    pushw %bp
    pushw %ds
    pushw %es

    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %sp, %ax
    pushw %ax
    call syscall_dispatch_frame
    addw $2, %sp

    movw %sp, %si
    movw %ax, 16(%si)
    popw %es
    popw %ds
    popw %bp
    popw %di
    popw %si
    popw %dx
    popw %cx
    popw %bx
    popw %ax
    iret
