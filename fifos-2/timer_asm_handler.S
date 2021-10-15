.global timer_interrupt_asm_handler

timer_interrupt_asm_handler:
    pushal
    pushw %es
    pushw %ds
    pushw %ss
    pushw %fs
    pushw %gs
    cld
    call timer_interrupt_handler
    popw %gs
    popw %fs
    popw %ss
    popw %ds
    popw %es
    popal
    iret
