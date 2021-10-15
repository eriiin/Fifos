/*
 * boot.S for fifo-1
 *
 */
	.globl stack
	
	.bss
	.align 0x1000
	.comm stack, 0x1000 /* setup 4KB stack area in bss */

	.data
gdt:
	/* null */
	.long 0
	.long 0

	/* cs */
	.long 0x0000FFFF
	.long 0x00CF9A00

	/* ds */
	.long 0x0000FFFF
	.long 0x00CF9200

gdt_ptr:
	.short 0x18
	.long gdt
	
	.text
	.globl _start
_start:
	jmp real_start

	/* Multiboot header -- Safe to place this header in 1st page for GRUB */
	.align 4
	.long 0x1BADB002 /* Multiboot magic number */
	.long 0x00000003 /* Align modules to 4KB, req. mem size */
			 /* See 'info multiboot' for further info */
	.long 0xE4524FFB /* Checksum */

real_start:
	lgdt gdt_ptr
	ljmp $0x08, $1f
1:
	movw $0x10, %ax
	movw %ax, %es
	movw %ax, %ds
	movw %ax, %ss
	movw %ax, %fs
	movw %ax, %gs

	/* set up stack */
	movl $stack+0x1000, %esp /* setup 4Kbyte stack */
	
	/* save multiboot parameter, for eventual call to C code */
	pushl %ebx

	call init /* start of C code */

	/* In case we return from the call, we want to suspend the processor */
	cli
	hlt
loop:
	jmp loop
