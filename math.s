	.file	"math.c"
	.text
	.section	.rodata
.LC0:
	.string	"src/math.c"
.LC1:
	.string	"min <= max"
	.text
	.globl	clamp
	.type	clamp, @function
clamp:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	%esi, -8(%rbp)
	movl	%edx, -12(%rbp)
	movl	-8(%rbp), %eax
	cmpl	-12(%rbp), %eax
	jle	.L2
	leaq	__PRETTY_FUNCTION__.0(%rip), %rax
	movq	%rax, %rcx
	movl	$4, %edx
	leaq	.LC0(%rip), %rax
	movq	%rax, %rsi
	leaq	.LC1(%rip), %rax
	movq	%rax, %rdi
	call	__assert_fail@PLT
.L2:
	movl	-4(%rbp), %eax
	cmpl	-8(%rbp), %eax
	jge	.L3
	movl	-8(%rbp), %eax
	jmp	.L4
.L3:
	movl	-4(%rbp), %eax
	cmpl	-12(%rbp), %eax
	jle	.L5
	movl	-12(%rbp), %eax
	jmp	.L4
.L5:
	movl	-4(%rbp), %eax
.L4:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	clamp, .-clamp
	.section	.rodata
	.type	__PRETTY_FUNCTION__.0, @object
	.size	__PRETTY_FUNCTION__.0, 6
__PRETTY_FUNCTION__.0:
	.string	"clamp"
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
