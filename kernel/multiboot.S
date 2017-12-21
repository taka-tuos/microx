#define __MULTIBOOT__S	1
#include "multiboot.h"

.extern _kernel_entry
.extern microx_api

.extern inthandler20, inthandler21, inthandler2c, inthandler07, inthandler0c, inthandler0d

.section .bss
.align 16
stack_bottom:
.skip 65536
stack_top:

.section .text
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
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 0
    .long 800
    .long 600
    .long 32

_multi_boot_entry:
    cli
    pushl   $0
    popf

    movl $stack_top, %esp
    
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
.global asm_inthandler21, asm_inthandler20, asm_inthandler2c, asm_inthandler0c, asm_inthandler0d
.global io_hlt, io_cli, io_sti, io_stihlt
.global io_load_eflags, io_store_eflags
.global load_tr, farjmp
.global load_gdtr, load_idtr
.global read_cs, read_ss
.global asm_microx_api, start_app, asm_end_app;
.global asm_inthandler07, clts, fnsave, frstor

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
	
asm_inthandler2c:
	pushw	%es
	pushw	%ds
	pusha
	
	movl	%esp, %eax
	pushl	%eax
	movw	%ss, %ax
	movw	%ax, %ds
	movw	%ax, %es
	call	inthandler2c
	popl	%eax

	popa
	popw	%ds
	popw	%es
	iret

asm_inthandler0c:
	sti
	push %es
	push %ds
	pusha
	mov %esp, %eax
	push %eax
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	call inthandler0c
	cmp $0, %eax
	jne asm_end_app
	pop %eax
	popa
	pop %ds
	pop %es
	add $4, %esp
	iret

asm_inthandler0d:
	sti
	push %es
	push %ds
	pusha
	mov %esp, %eax
	push %eax
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	call inthandler0d
	cmp $0, %eax
	jne asm_end_app
	pop %eax
	popa
	pop %ds
	pop %es
	add $4, %esp
	iret

asm_microx_api: 
	sti
	pushl	%ds
	pushl	%es
	pushal
	pushal 
	movw	%ss,%ax
	movw	%ax,%ds
	movw	%ax,%es
	call	microx_api
	cmpl	$0,%eax
	jne		asm_end_app
	addl	$32,%esp
	popal
	popl	%es
	popl	%ds
	iretl
asm_end_app: 
	movl            (%eax),%esp
	movl            $0,4(%eax)
	popal
	ret

start_app:
	pushal
	movl	36(%esp),%eax
	movl	40(%esp),%ecx
	movl	44(%esp),%edx
	movl	48(%esp),%ebx
	movl	52(%esp),%ebp
	movl	%esp,(%ebp)
	movw	%ss,4(%ebp)
	movw	%bx,%es
	movw	%bx,%ds
	movw	%bx,%fs
	movw	%bx,%gs
	
	orl		$3,%ecx
	orl		$3,%ebx
	pushl	%ebx
	pushl	%edx
	pushl	%ecx
	pushl	%eax
	lret

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

clts:	# void clts(void);
	clts
	ret

fnsave:	# void fnsave(int *addr);
	movl	4(%esp),%eax            # addr
	fnsave	(%eax)
	ret

frstor:	# void frstor(int *addr);
	movl	4(%esp),%eax            # addr
	frstor	(%eax)
	ret

asm_inthandler07: 
	sti
	pushl	%es
	pushl	%ds
	pushal
	movl	%esp,%eax
	pushl   %eax
	movw	%ss,%ax
	movw	%ax,%ds
	movw	%ax,%es
	call	inthandler07
	cmpl	$0,%eax
	jne		asm_end_app
	popl	%eax
	popal
	popl	%ds
	popl	%es
	iretl
