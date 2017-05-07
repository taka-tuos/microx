#define __MULTIBOOT__S	1
#include "multiboot.h"

.extern _kernel_entry

.text
.code32
.globl start, _start

start:
_start:
    jmp     _multi_boot_entry

/* align 32 bits boundary	*/
.align 4

multiboot_header:
    .long   DEF_MBH_MAGIC
    .long   DEF_MBH_FLAGS
    .long   DEF_MBH_CHECKSUM

_multi_boot_entry:
    cli
    pushl   $0
    popf

    .comm stack, 0x40000, 32
    pushl   %ebx    # 1st argument
    pushl   %eax    # 2nd argument

    call    _kernel_entry

loop_infinite:
    hlt
    jmp     loop_infinite

.global io_in8, io_in16, io_in32
.global io_out8, io_out16, io_out32
.global io_load_eflags, io_store_eflags
.global store_cr0, load_cr0
.global asm_inthandler21, asm_inthandler20
.global io_hlt, io_cli, io_sti, io_stihlt
.global io_load_eflags, io_store_eflags
.global load_tr, farjmp
.global load_gdtr, load_idtr
.global read_cs, read_ss

io_in8:		# void io_in8(int port)
	movl	4(%esp), %edx	#port
	movl	$0, %eax
	inb	%dx, %al
	ret
	
io_in16:	# void io_in16(int port)
	movl	4(%esp), %edx	#port
	movl	$0, %eax
	inw	%dx, %ax
	ret

io_in32:	# void io_in32(int port)
	movl	4(%esp), %edx	#port
	movl	$0, %eax
	inl	%dx, %eax
	ret

io_out8:	# void io_out8(int port, int data)
	movl	4(%esp), %edx	#port
	#movb	8(%esp), %al	#data
	movl	8(%esp), %eax
	outb	%al, %dx
	ret

io_out16:	# void io_out16(int port, int data)
	movl	4(%esp), %edx	#port
	movl	8(%esp), %eax	#data
	outw	%ax, %dx
	ret

io_out32:	# void io_out32(int port, int data)
	movl	4(%esp), %edx	#port
	movl	8(%esp), %eax	#data
	outl	%eax, %dx
	ret

asm_inthandler20:	
	pushw	%es
	pushw	%ds
	pusha

	movl	%esp, %eax
	pushl	%eax
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	call	inthandler20
	popl	%eax

	popa
	popw	%ds
	popw	%es
	iret
	
asm_inthandler21:
	pushw	%es
	pushw	%ds
	pusha
	
	movl	%esp, %eax
	pushl	%eax
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	call	inthandler21
	popl	%eax

	popa
	popw	%ds
	popw	%es
	iret

io_hlt:		# void io_hlt()
	hlt
	ret
	
io_cli:		# void io_cli()
	cli
	ret
		
io_sti:		# void io_sti()
	sti
	ret
	
io_stihlt:	# void io_stihlt()
	sti
	hlt
	ret

io_load_eflags:	#int io_load_eflags()
	pushf	# same as 'PUSHFD'(push %eflags)
	popl	%eax
	ret

io_store_eflags:	# void_store_eflags(int eflags)
	movl	4(%esp), %eax
	pushl	%eax
	popf	# same as 'POPFD' (push %eflags)
	ret

load_tr:	# void load_tr(int tr)
	ltr	4(%esp)		# tr
	ret

farjmp:		# void farjmp(int eip, int cs)
	ljmpl	*4(%esp)
	ret

load_gdtr:	# void load_gdtr(int limit, int addr)
	movw	4(%esp), %ax
	movw	%ax, 6(%esp)
	lgdt	6(%esp)
	ret

load_idtr:	# void load_idtr(int limit, int addr)
	movw	4(%esp), %ax
	movw	%ax, 6(%esp)
	lidt	6(%esp)
	ret

read_cs:	# int read_cs(void)
	movl	%cs, %eax
	ret

read_ss:	# int read_ss(void)
	movl	%ss, %eax
	ret
