how to run fifos-2:
    cd fifos-2
    make
    make test

how to run synchros:
    cd synchros
    make
    make test


reference:
    Intel Software Developer's Manual
    https://wiki.osdev.org/Interrupt_Descriptor_Table
    https://wiki.osdev.org/8259_PIC
    https://wiki.osdev.org/Programmable_Interval_Timer

流程：
    FIFOS-1（抢断式调度）
        1、初始化IDT(Interrupt Descriptor Table)，主要用于在中断发生，CPU找到对应的中断处理函数
        2、初始化PIC，只使用了master PIC，没有用slave pic
        3、初始化PIT，定时的发出中断信号

    SYNCHROS（线程间同步，只实现了cli/sti pairs,没有实现bonus）