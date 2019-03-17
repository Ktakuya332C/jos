.set PROT_MODE_CSEG, 0x8
.set PROT_MODE_DSEG, 0x10
.set CR0_PE_ON, 0x1

.globl start
start:
  .code16
  
  cli # Disable interrupts
  cld # String operations increment the counter
  
  # Switch from 16-bit real mode to 32-bit protected mode
  lgdt gdtdesc
  movl %cr0, %eax
  orl $CR0_PE_ON, %eax
  movl %eax, %cr0
  ljmp $PROT_MODE_CSEG, $protcseg

gdt:
	# Null segment
	.word 0x0, 0x0
	.byte 0x0, 0x0, 0x0, 0x0
	# Code segment
	.word 0xffff
	.word 0x0
	.byte 0x0
	.byte 0b10011010
	.byte 0b11001111
	.byte 0x0
	# Data segment
	.word 0xffff
	.word 0x0
	.byte 0x0
	.byte 0b10010010
	.byte 0b11001111
	.byte 0x0
gdtdesc:
	.word 0x17
	.long gdt
	
	.code32
protcseg:
	# Setup the 32-bit protected mode data segment registers
	movw $PROT_MODE_DSEG, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	
	# Setup the stack pointer and call into C
	movl $start, %esp
	call bootmain

spin:
  jmp spin # If bootmain returns, loop infinitely
