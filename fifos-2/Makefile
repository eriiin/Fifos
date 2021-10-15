CC = gcc
CFLAGS = -m32 -fno-builtin -fno-stack-protector -fno-strict-aliasing -fno-delete-null-pointer-checks -nostdinc -I. -g -Wall -std=c99
CPPFLAGS = -Wa,--32 -MMD
OBJS = boot.o init.o switch.o interrupt.o timer_asm_handler.o
PROGS = fifos-2
MNT_POINT=/mnt/

all: $(PROGS)

fifos-2: $(OBJS)
	$(LD) -m elf_i386 -T fifos.ld -o $@ $^

%: %.c

test:
	qemu-system-i386 -m 512 -kernel fifos-2

install: $(PROGS)
	cp $(PROGS) $(MNT_POINT)/boot
	sync

clean:
	-rm *.o *.d $(PROGS) *~

-include *.d
