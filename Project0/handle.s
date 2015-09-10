	.file	"handle.c"
	.section	.rodata
.LC0:
	.string	"PID = %d\n"
.LC1:
	.string	"Still here\n"
.LC2:
	.string	"signal error"
	.text
	.globl	main
	.type	main, @function
main:
.LFB2:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$80, %rsp
	movl	%edi, -68(%rbp)
	movq	%rsi, -80(%rbp)
	call	getpid
	movl	%eax, -48(%rbp)
	cmpl	$0, -48(%rbp)
	js	.L2
	movl	-48(%rbp), %eax
	movl	%eax, %esi
	movl	$.LC0, %edi
	movl	$0, %eax
	call	printf
	movl	$1, -44(%rbp)
	movq	$1, -32(%rbp)
	movq	$0, -24(%rbp)
	movl	$0, -52(%rbp)
.L9:
	cmpl	$-1, -52(%rbp)
	je	.L3
	movl	-44(%rbp), %eax
	movl	$11, %edx
	movl	$.LC1, %esi
	movl	%eax, %edi
	call	write
	movq	%rax, -40(%rbp)
	movq	$1, -32(%rbp)
	movq	$0, -24(%rbp)
.L3:
	cmpq	$11, -40(%rbp)
	je	.L4
	movl	$-999, %edi
	call	exit
.L4:
	movl	$handler, %esi
	movl	$2, %edi
	call	Signal
	cmpq	$-1, %rax
	jne	.L5
	movl	$.LC2, %edi
	call	unix_error
.L5:
	movl	$handler, %esi
	movl	$10, %edi
	call	Signal
	cmpq	$-1, %rax
	jne	.L6
	movl	$.LC2, %edi
	call	unix_error
.L6:
	leaq	-16(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	nanosleep
	movl	%eax, -52(%rbp)
	jmp	.L7
.L8:
	movq	-8(%rbp), %rax
	movq	%rax, -24(%rbp)
	movq	$0, -32(%rbp)
	leaq	-16(%rbp), %rdx
	leaq	-32(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	nanosleep
	movl	%eax, -52(%rbp)
.L7:
	cmpl	$-1, -52(%rbp)
	je	.L8
	jmp	.L9
.L2:
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	main, .-main
	.section	.rodata
.LC3:
	.string	"Nice try.\n"
.LC4:
	.string	"exiting\n"
	.text
	.globl	handler
	.type	handler, @function
handler:
.LFB3:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$32, %rsp
	movl	%edi, -20(%rbp)
	movl	$1, -12(%rbp)
	cmpl	$2, -20(%rbp)
	jne	.L12
	movl	-12(%rbp), %eax
	movl	$10, %edx
	movl	$.LC3, %esi
	movl	%eax, %edi
	call	write
	movq	%rax, -8(%rbp)
	cmpq	$10, -8(%rbp)
	je	.L11
	movl	$-999, %edi
	call	exit
.L12:
	cmpl	$10, -20(%rbp)
	jne	.L11
	movl	-12(%rbp), %eax
	movl	$8, %edx
	movl	$.LC4, %esi
	movl	%eax, %edi
	call	write
	movq	%rax, -8(%rbp)
	cmpq	$8, -8(%rbp)
	je	.L14
	movl	$-999, %edi
	call	exit
.L14:
	movl	$1, %edi
	call	exit
.L11:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	handler, .-handler
	.ident	"GCC: (Ubuntu 4.8.4-2ubuntu1~14.04) 4.8.4"
	.section	.note.GNU-stack,"",@progbits
