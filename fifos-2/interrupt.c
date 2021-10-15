#include "types.h"
#include "interrupt.h"


#define IDT_N 256
#define PIT_HZ 1193180
#define PIT_CMD_PORT 0x43
#define PIT_CHANNEL_0_PORT 0x40
#define PIC_MASTER_CMD_PORT 0x20
#define PIC_MASTER_DATA_PORT 0x21


static struct IDTDescr idt_descrs[IDT_N];


extern void thread_schedule();
extern void timer_interrupt_asm_handler();

static void outb (uint16_t port, uint8_t data)
{
  __asm__ volatile ("outb %0, %1" :: "a" (data), "Nd" (port));
}


void timer_interrupt_handler()
{
    /* end of interrupt */
    outb(PIC_MASTER_CMD_PORT , 0x20);
    thread_schedule();
}


void init_idt()
{
    int i;
    uint64_t lidt_info;
    uint16_t len = sizeof(idt_descrs) - 1;
    for (i=0; i<IDT_N; ++i) {
        idt_descrs[i].offset_1 = 0;
        idt_descrs[i].selector = 0x08;
        idt_descrs[i].zero = 0;
        idt_descrs[i].type_attr = 0x8e;
        idt_descrs[i].offset_2 = 0;
    }

    /* set timer interrupt handler */
    idt_descrs[32].offset_1 = (uint32_t)timer_interrupt_asm_handler & 0xFFFF;
    idt_descrs[32].offset_2 = (uint32_t)timer_interrupt_asm_handler >> 16;

    lidt_info = (uint64_t)((uint32_t)idt_descrs);
    lidt_info = (lidt_info << 16) | len;
    __asm__ volatile ("lidt %0"::"m" (lidt_info));
}


void init_pic()
{
    outb(PIC_MASTER_CMD_PORT, 0x11);
    outb(PIC_MASTER_DATA_PORT, 0x20);
    outb(PIC_MASTER_DATA_PORT, 0x0D);
}


void init_pit()
{
    outb(PIT_CMD_PORT, 0x34);
    outb(PIT_CHANNEL_0_PORT, ((PIT_HZ / 50) & 0xFF));
    outb(PIT_CHANNEL_0_PORT, ((PIT_HZ / 50) >> 8));
}