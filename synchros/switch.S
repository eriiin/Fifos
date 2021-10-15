.global switch

switch:
    # check if it's the first time
    testl %esi, %esi
    jz 1f
    # save current tcb
    # save registers
    pushfl
    pushal
    pushw %es
    pushw %ds
    pushw %ss
    pushw %fs
    pushw %gs
    pushw %gs
    movl %esp, (%esi)
1:
    # switch to next thread
    movl (%edi), %esp
    # restore registers
    popw %gs
    popw %gs
    popw %fs
    popw %ss
    popw %ds
    popw %es
    popal
    popfl
    sti
    ret
