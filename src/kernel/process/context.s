.code16
.global process_context_switch

process_context_switch:
    pushf
    cli
    pushw %ax
    pushw %bx
    pushw %cx
    pushw %dx
    pushw %si
    pushw %di
    pushw %bp
    pushw %ds
    pushw %es

    movw %sp, %bx
    movw 22(%bx), %si
    movw 24(%bx), %di
    movw %sp, (%si)
    movw %di, %sp

    popw %es
    popw %ds
    popw %bp
    popw %di
    popw %si
    popw %dx
    popw %cx
    popw %bx
    popw %ax
    popf
    ret
